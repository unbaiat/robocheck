#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "black_list.h"

#include "../lib.h"
#include "../allocate.h"
#include "../token.h"
#include "../parse.h"
#include "../symbol.h"
#include "../expression.h"

#define MAX_NUM(a, b) \
            ((a > b) ? a : b)

enum Errors
{
    ERR_FUN_CALL = 1,
    ERR_FUN_LINES,
    ERR_TABS_SPACES,
    ERR_TRAYLING_WS,
    ERR_INCONSISTENT_INDENT,
    ERR_FUN_INDENT_LEVEL,

    ERR_MAX
};

/* CMD line configurable values */
static int MaxLineCount = -1;
static int MaxIndentLevel = 5;

#ifdef VEC_ACCESS
/* FIXME */
static char *IntVariables[1024];
static int IntValues[1024];
static int DeclaredIntValuesSize = 0;
#endif

/* feature variable declarations */
static int GlobalFlags = 0;
static int CurrentIndentLevel = -1;
static int LastInstructionLine = -1;

/* declarations */
static char *AnalyzedFunction = NULL;
static char *AssignmentVariable = NULL;
static char *CurrentFile = NULL;

static char *FileMapping = NULL;
static int MappingIndex = 0, FileSize = 0;
static int PrevLine = 0, CurrentLine = 0;

static struct string_list *FileList = NULL;
static struct symbol_list *FileFunctionList[1024] = { NULL };
static int FileFunctionListSize = -1;

static void
populate_symlist(struct symbol_list *list);

static void
parse_file_list (int argc, char **args, void * (* f_ptr)(struct statement *), int flags);

static void
parse_function_list (struct symbol_list *list, void * (* f_ptr)(struct statement *), int flags);

static int
examine_statement(struct statement *stmt, void * (* wrapper_func) (struct statement *), int set_relation);

static void *
examine_stmt_expression(struct statement *stmt);

static int
examine_expression(struct expression *expr);

static int
handle_call_simple(struct expression *expr);

static void
handle_assignment_expr(struct expression *expr);

static void
examine_preop(struct expression *expr);

static int
find_usages(struct statement *start_statement, struct expression *expr);

static struct statement *
get_expression_statement(struct expression *expr);

struct statement *
get_next_statement(struct statement *prt, int caller_line);

static void *
lookup_var (struct statement *stmt);

static int
validate_expr(struct expression *expr);

static int
lookup_expression(struct expression *expr);

static char *
get_caller(struct expression *expr);

static char *
get_assignment_var(struct expression *expr);

static struct expression *
get_last_expression(struct expression *expr);

static void
emit_error(char * current_file, char *func_name, int line, const char *err_msg);

static int
check_start_statement(struct statement *start_statement, struct expression *expr);

static int
handle_out_call_params(struct expression *expr);

static int
handle_declaration(struct expression *call_expr, struct statement *decl_stmt);

static int
is_declaration_call(struct expression *expr, struct statement ** ret_statement);

static char *
get_out_symbol (struct expression *expr);

static struct expression *
get_call_node (struct expression *expr);

static int
continue_parsing(int flags);

#ifdef VEC_ACCESS
static void
parse_init_statement(struct statement *stmt);
#endif

static char *
build_symbol(struct expression *expr);

static void
populate_symlist(struct symbol_list *list)
{
	struct symbol *sym;

	FOR_EACH_PTR(list, sym) {
		expand_symbol(sym);
	} END_FOR_EACH_PTR(sym);
}

static void
parse_file_list (int argc, char **args, void * (* f_ptr)(struct statement *), int flags)
{
    struct symbol_list * list = NULL;
    struct string_list *filelist = NULL, *tmp_filelist = NULL;
    char *file;
    int index = 0;

    off_t length = -1;
    int fd = -1, create_mapping = continue_parsing(flags);

    if (f_ptr == NULL) { return; }

    if (FileList == NULL)
    {
        list = sparse_initialize(argc, args, &filelist);
        tmp_filelist = filelist;
        
        populate_symlist(list);
        FileFunctionListSize = 0;
    }
    else
    {
        filelist = FileList;
    }

    FOR_EACH_PTR_NOTAG(filelist, file) {
        if (FileList == NULL)
        {
            list = sparse(file);
            populate_symlist(list);
            FileFunctionList[FileFunctionListSize++] = list;
        }

        CurrentFile = file;
        FileMapping = NULL;

        if (create_mapping)
        {
            fd = open(file, O_RDONLY, 0644);
            if (fd == -1)
            {
                perror("open");
                goto parse;
            }
            length = lseek(fd, 0, SEEK_END);
            if (length == -1)
            {
                perror("lseek");
                goto close_file;
            }

            FileMapping = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
            if (FileMapping != NULL)
            {
                FileSize = length;
                MappingIndex = PrevLine = CurrentLine = 0;

                goto parse;
            }
            else
            {
                perror("mmap");
            }

close_file:
            close(fd);
        }

parse:
        printf ("####From file: %s####\n", file);
        parse_function_list(FileFunctionList[index++], f_ptr, flags);
	printf ("####End parsing file####\n");

        if (create_mapping && FileMapping != NULL)
        {
            if (munmap(FileMapping, length) == -1)
            {
                perror("munmap");
            }

            close(fd);
        }
    } END_FOR_EACH_PTR_NOTAG(file);

    if (FileList == NULL)
    {
        FileList = tmp_filelist;
    }
}

static void
parse_function_list (struct symbol_list *list, void * (* f_ptr)(struct statement *), int flags)
{
    struct symbol *sym = NULL, *type = NULL;
    struct statement_list *instructions = NULL;
    struct statement *instruction = NULL;

#ifdef VEC_ACCESS
    DeclaredIntValuesSize = 0;
#endif

    if (list != NULL)
    {
        FOR_EACH_PTR(list, sym)
        {
            if (sym->ctype.base_type != NULL)
            {
                type = sym->ctype.base_type;
                if (type->type == SYM_FN)
                {
                    if (type->stmt && type->stmt->stmts)
                    {
                        printf ("\t#####parsing function: %s#####\n", sym->ident->name);

                        CurrentIndentLevel = LastInstructionLine = -1;
                        type->stmt->indent_level = 0;
                        instructions = type->stmt->stmts;
                        GlobalFlags = flags;

                        FOR_EACH_PTR(instructions, instruction)
                        {
                            instruction->prt_stmt = type->stmt;
                            examine_statement(instruction, f_ptr, 1);
                        } END_FOR_EACH_PTR(instruction);

                        if (flags & (1<<ERR_FUN_INDENT_LEVEL) && CurrentIndentLevel >= MaxIndentLevel)
                        {
                            printf ("\t\tIndent level: %d\n", CurrentIndentLevel);
                        }
                        if (flags & (1<<ERR_FUN_LINES))
                        {
                            int line_count = LastInstructionLine - type->pos.line + 1;
                            if (MaxLineCount <= 0 || (LastInstructionLine - type->pos.line + 1 >= MaxLineCount))
                            {
                                printf ("\t\tFunction line count: %d\n", line_count);
                            }
                        }

			printf ("\t#####end parsing function#####\n");
                    }
                }
            }
        } END_FOR_EACH_PTR(sym);
    }
}

static int
examine_statement(struct statement *stmt, void * (* wrapper_func) (struct statement *), int set_relation)
{
    struct statement *crt_statement = NULL;
    struct statement_list *list = NULL;
    int ret_value = 1, tmp_value;
#ifdef VEC_ACCESS
    struct symbol *sym = NULL;
#endif

    if (wrapper_func == NULL)
    {
        fprintf(stdout, "WRAPPER FUNCTION passed to examine_statement is NULL\n");
        return 1;
    }

    if (stmt != NULL)
    {
        void *continue_eval = wrapper_func(stmt);
        if (continue_eval == NULL)
        {
            return 0;
        }

        switch(stmt->type)
        {
            case STMT_ASM:
                fprintf (stdout, "statement ASM not handled");
                break;
            case STMT_CASE:
#ifdef RBC_STMT_DBG
                printf ("CASE");
#endif
                if (set_relation) { stmt->case_statement->prt_stmt = stmt; }

                return examine_statement(stmt->case_statement, wrapper_func, set_relation);
                break;
            case STMT_COMPOUND:
#ifdef RBC_STMT_DBG
                printf ("COMPOUND");
#endif
                list = stmt->stmts;
                ret_value = 1;
                FOR_EACH_PTR(list, crt_statement)
                {
                    if (set_relation) { crt_statement->prt_stmt = stmt; }

                    tmp_value = examine_statement(crt_statement, wrapper_func, set_relation);
                    if (tmp_value == 0)
                    {
                        ret_value = 0;
                    }
                } END_FOR_EACH_PTR(crt_statement);

                return ret_value;
                break;
            case STMT_CONTEXT:
                printf ("CONTEXT");
                break;
            case STMT_DECLARATION:
#ifdef RBC_STMT_DBG
                printf ("DECLARATION");
#endif
#ifdef VEC_ACCESS
                parse_init_statement(stmt);
#endif
                break;
            case STMT_GOTO:
#ifdef RBC_STMT_DBG
                printf ("GOTO");
#endif
                break;
            case STMT_EXPRESSION:
#ifdef RBC_STMT_DBG
                printf ("EXPRESSION");
#endif
                break;
            case STMT_IF:
#ifdef RBC_STMT_DBG
                printf ("IF");
#endif
                if (set_relation && stmt->if_true) { stmt->if_true->prt_stmt = stmt; }
                if (set_relation && stmt->if_false) { stmt->if_false->prt_stmt = stmt; }

                ret_value = 1;

                tmp_value = examine_statement(stmt->if_true, wrapper_func, set_relation);
                if (tmp_value == 0) { ret_value = 0; }

                tmp_value = examine_statement(stmt->if_false, wrapper_func, set_relation);
                if (tmp_value == 0) { ret_value = 0; }

                return ret_value;
                break;
            case STMT_ITERATOR:
#ifdef RBC_STMT_DBG
                printf ("ITERATOR");
#endif
                if (set_relation && stmt->iterator_pre_statement) { stmt->iterator_pre_statement->prt_stmt = stmt; }
                if (set_relation && stmt->iterator_post_statement) { stmt->iterator_post_statement->prt_stmt = stmt; }
                if (set_relation && stmt->iterator_statement) { stmt->iterator_statement->prt_stmt = stmt; }

                ret_value = 1;

                tmp_value = examine_statement(stmt->iterator_pre_statement, wrapper_func, set_relation);
                if (tmp_value == 0) { ret_value = 0; }

                tmp_value = examine_statement(stmt->iterator_statement, wrapper_func, set_relation);
                if (tmp_value == 0) { ret_value = 0; }

                tmp_value = examine_statement(stmt->iterator_post_statement, wrapper_func, set_relation);
                if (tmp_value == 0) { ret_value = 0; }

                return ret_value;
                break;
            case STMT_LABEL:
#ifdef RBC_STMT_DBG
                printf ("LABEL");
#endif
                if (set_relation) { stmt->label_statement->prt_stmt = stmt; }

                return examine_statement(stmt->label_statement, wrapper_func, set_relation);
                break;
            case STMT_NONE:
                fprintf (stdout, "statement NONE not handled\n");
                break;
            case STMT_RANGE:
                fprintf (stdout, "statement RANGE not handled\n");
                break;
            case STMT_RETURN:
#ifdef RBC_STMT_DBG
                printf ("RETURN");
#endif
                break;
            case STMT_SWITCH:
#ifdef RBC_STMT_DBG
                printf ("SWITCH");
#endif
                if (set_relation) { stmt->switch_statement->prt_stmt = stmt; }

                return examine_statement(stmt->switch_statement, wrapper_func, set_relation);
                break;
            default:
                fprintf (stdout, "statement NO STMT TYPE not handled\n");
        }
#ifdef RBC_STMT_DBG
        printf("%d", stmt->pos.line);
        getchar();
#endif
    }

    return 1;
}

static void *
examine_stmt_expression(struct statement *stmt)
{
    struct expression *ex = NULL;
    struct symbol *sym = NULL;

    static void *ret_ok = (void *)"OK";

    if (stmt != NULL)
    {
        switch(stmt->type)
        {
            case STMT_DECLARATION:
                FOR_EACH_PTR(stmt->declaration, sym)
                {
                    ex = sym->initializer;
                    if (ex != NULL)
                    {
                        ex->prt_stmt = stmt;
                        examine_expression(ex);
                    }
                }END_FOR_EACH_PTR(sym);
                break;

            case STMT_COMPOUND:
                break;

            case STMT_CONTEXT:
            case STMT_EXPRESSION:
                ex = stmt->expression;
                if (ex != NULL)
                {
                    ex->prt_stmt = stmt;
                    examine_expression(ex);
                }
                break;

            case STMT_CASE:
                ex = stmt->case_expression;
                if (ex) { ex->prt_stmt = stmt; }
                examine_expression(ex);

                ex = stmt->case_to;
                if (ex) { ex->prt_stmt = stmt; }
                examine_expression(ex);
                break;

            case STMT_IF:
                stmt->if_conditional->prt_stmt = stmt;
                examine_expression(stmt->if_conditional);
                break;

            case STMT_ITERATOR:
                ex = stmt->iterator_pre_condition;
                if (ex != NULL) { ex->prt_stmt = stmt; }
                examine_expression(ex);

                ex = stmt->iterator_post_condition;
                if (ex != NULL) { ex->prt_stmt = stmt; }
                examine_expression(ex);
                break;

            case STMT_SWITCH:
                stmt->switch_expression->prt_stmt = stmt;
                examine_expression(stmt->switch_expression);
                break;

            case STMT_RANGE:
                stmt->range_expression->prt_stmt = stmt;
                stmt->range_low->prt_stmt = stmt;
                stmt->range_high->prt_stmt = stmt;

		examine_expression(stmt->range_expression);
		examine_expression(stmt->range_low);
		examine_expression(stmt->range_high);
		break;

            case STMT_RETURN:
                stmt->ret_value->prt_stmt = stmt;
                examine_expression(stmt->ret_value);
                break;

            case STMT_GOTO:
                if (stmt->goto_expression) { stmt->goto_expression->prt_stmt = stmt; }
                examine_expression(stmt->goto_expression);
                break;

            case STMT_LABEL:
            case STMT_NONE:
            case STMT_ASM:
                break;
        }
    }

    return ret_ok;
}

static int
examine_expression(struct expression *expr)
{
    int temp_value = 0;
    struct statement *st;

    if (expr != NULL && (!expr->visited1 || !expr->visited2))
    {
        if (expr->visited1 == NULL)
        {
            expr->visited1 = malloc(2);
        }
        else
        {
            expr->visited2 = malloc(2);
        }

        switch (expr->type)
        {
            case EXPR_CALL:
                AnalyzedFunction = NULL;
                if (is_black_listed(get_caller(expr)))
                {
                    if (is_declaration_call(expr, &st))
                    {
                        temp_value = handle_declaration(expr, st);
                    }
                    else
                    {
                        temp_value = handle_call_simple(expr);
                    }

                    if (temp_value)
                    {
                        if (handle_out_call_params(expr))
                        {
                            emit_error(CurrentFile, get_caller(expr), expr->pos.line, "Unverified return value for function");
                        }
                    }
                }
                
                return EXPR_CALL;

            case EXPR_ASSIGNMENT:
                handle_assignment_expr(expr);
                return EXPR_ASSIGNMENT;

            case EXPR_COMMA:
                expr->left->prt_expr = expr->right->prt_expr = expr;
                examine_expression(expr->left);
                examine_expression(expr->right);
                return EXPR_COMMA;

            case EXPR_BINOP:
            case EXPR_COMPARE:
            case EXPR_LOGICAL:
                expr->left->prt_expr = expr->right->prt_expr = expr;
                examine_expression(expr->left);
                examine_expression(expr->right);
                return EXPR_LOGICAL;

            case EXPR_PREOP:
                examine_preop(expr);
                return EXPR_PREOP;

            case EXPR_POSTOP:
#ifdef RBC_DBG_EXAMINE
                printf("postop");
#endif
                return EXPR_POSTOP;

            case EXPR_SYMBOL:
                return EXPR_SYMBOL;

            case EXPR_CAST:
            case EXPR_FORCE_CAST:
            case EXPR_IMPLIED_CAST:
                expr->cast_expression->prt_expr = expr;
                examine_expression(expr->cast_expression);
                return EXPR_CAST;

            case EXPR_VALUE:
                return EXPR_VALUE;

            case EXPR_FVALUE:
                return EXPR_FVALUE;

            case EXPR_STRING:
                return EXPR_STRING;

            case EXPR_INITIALIZER:
                printf("\n\t\tinit");
                getchar();
                printf ("\n\t\t end init");
                return EXPR_INITIALIZER;

            case EXPR_SELECT:
            case EXPR_CONDITIONAL:
                expr->conditional->prt_expr = expr;
                expr->cond_false->prt_expr = expr; expr->cond_true->prt_expr = expr;

                examine_expression(expr->conditional);
                examine_expression(expr->cond_true);
                examine_expression(expr->cond_false);
                return EXPR_CONDITIONAL;

            case EXPR_STATEMENT:
                examine_statement(expr->statement, examine_stmt_expression, 1);
                return EXPR_STATEMENT;

            case EXPR_LABEL:
                return EXPR_LABEL;

            case EXPR_SLICE:
                expr->base->prt_expr = expr;
                examine_expression(expr->base);
                return EXPR_SLICE;

            default:
                printf("error - invalid expression");
        }
    }

    return -1;
}

static int
handle_call_simple(struct expression *expr)
{
    char *fun_name = NULL;
    struct expression *temp_expr = NULL;

    if (expr != NULL)
    {
        fun_name = get_caller(expr);
        AnalyzedFunction = NULL;

        if (fun_name != NULL)
        {
            AnalyzedFunction = fun_name;
        }
        temp_expr = get_last_expression(expr);

        if (expr->prt_expr == NULL && expr->prt_stmt)
        {
            if (expr->prt_stmt->type != STMT_SWITCH && expr->prt_stmt->type != STMT_CASE &&
                expr->prt_stmt->type != STMT_IF &&
                expr->prt_stmt->type != STMT_RETURN &&
                expr->prt_stmt->type != STMT_DECLARATION)
            {
                return 1;
            }
        }
        else if (temp_expr && temp_expr->prt_expr == NULL && temp_expr->prt_stmt)
        {
            if (temp_expr->prt_stmt->type != STMT_SWITCH && temp_expr->prt_stmt->type != STMT_CASE &&
                temp_expr->prt_stmt->type != STMT_IF &&
                temp_expr->prt_stmt->type != STMT_RETURN &&
                temp_expr->prt_stmt->type != STMT_DECLARATION)
            {
                return 1;
            }
        }
    }

    return 0;
}

static void
handle_assignment_expr(struct expression *expr)
{
    int ret_value = -1;
    struct statement *start_statement = NULL;

    if (expr != NULL)
    {
        AssignmentVariable = get_assignment_var(expr);

        expr->right->prt_expr = expr;
        ret_value = examine_expression(expr->right);

        if (ret_value == EXPR_CALL || ret_value == EXPR_CAST)
        {
            start_statement = get_expression_statement(expr);
            if (start_statement)
            {
                if (is_black_listed(AnalyzedFunction))
                {
                    ret_value = check_start_statement(start_statement, expr);
                    if (ret_value)
                    {
                        int line_pos = expr->pos.line;
                        expr = get_call_node(expr);
                        if (handle_out_call_params(expr))
                        {
                            emit_error(CurrentFile, AnalyzedFunction, line_pos, "Unverified return value of function call");
                        }
                    }
                }
            }
            else if (validate_expr(expr))
            {
                emit_error(CurrentFile, AnalyzedFunction, expr->pos.line, "Unverified return value of function call");
            }
        }
    }
}

static void
examine_preop(struct expression *expr)
{
    if (expr != NULL)
    {
        if (expr->op == SPECIAL_INCREMENT || expr->op == SPECIAL_DECREMENT)
        {
            /* TODO */
        }
        else
        {
            expr->unop->prt_expr = expr;
            examine_expression(expr->unop);
        }
    }
}

static int
find_usages(struct statement *start_statement, struct expression *expr)
{
    int show_err = 1;
    struct statement *crt_statement = NULL;
    struct statement *next_statement = get_next_statement(start_statement, expr->pos.line);

    while (next_statement != NULL && next_statement->type == STMT_COMPOUND)
    {
        if (next_statement->stmts != NULL)
        {
            //printf ("next statement at %d\n", next_statement->pos.line);

            FOR_EACH_PTR(next_statement->stmts, crt_statement)
            {
                if (crt_statement->pos.line > expr->pos.line)
                {
                    //printf ("crt statement at %d\n", crt_statement->pos.line);

                    int st_eval = examine_statement(crt_statement, lookup_var, 0);
                    if (st_eval == 0)
                    {
                        show_err = 0;
                        break;
                    }
                }
            }END_FOR_EACH_PTR(crt_statement);
        }

        if (show_err == 0) break;

        next_statement = get_next_statement(next_statement->prt_stmt, next_statement->pos.line);
    }

    return show_err;
}

static struct statement *
get_expression_statement(struct expression *expr)
{
    struct statement *stmt = NULL;
    struct expression *crs = expr;

    while (crs)
    {
        if (crs->prt_stmt != NULL)
        {
            stmt = crs->prt_stmt;
            break;
        }

        crs = crs->prt_expr;
    }

    return stmt;
}

static void
emit_error(char * current_file, char *func_name, int line, const char *err_msg)
{
    static char output_buffer[256];

    current_file = (current_file != NULL) ? current_file : (char *)"<NO FILE>";
    func_name = (func_name != NULL) ? func_name : (char *)"<NO FILE>";

    sprintf(output_buffer, "%s in file '%s' for function call '%s' at line '%d'",
                            err_msg, current_file, func_name, line);

    fprintf(stdout, "%s\n", output_buffer);
}

struct statement *
get_next_statement(struct statement *prt, int caller_line)
{
    if (prt != NULL)
    {
        switch(prt->type)
        {
            case STMT_ASM:
                fprintf (stdout, "statement ASM not handled");
                break;

            case STMT_CASE:
#ifdef RBC_NEXT
                printf ("CASE");
#endif
                if (prt->prt_stmt != NULL && prt->prt_stmt->prt_stmt != NULL)
                {
                    return get_next_statement(prt->prt_stmt->prt_stmt, caller_line);
                }
                else
                {
                    fprintf(stdout, "CASE NODE has parrent NULL");
                    getchar();
                }
                break;

            case STMT_COMPOUND:
#ifdef RBC_NEXT
                printf ("COMPOUND");
#endif
                return prt;

            case STMT_CONTEXT:
                printf ("CONTEXT");
                break;

            case STMT_DECLARATION:
#ifdef RBC_NEXT
                printf ("DECLARATION");
#endif
                if (prt->prt_stmt)
                {
                    return get_next_statement(prt->prt_stmt, caller_line);
                }
                break;

            case STMT_GOTO:
#ifdef RBC_NEXT
                printf ("GOTO");
#endif
                break;

            case STMT_EXPRESSION:
#ifdef RBC_NEXT
                printf ("EXPRESSION");
#endif
                if (prt->prt_stmt != NULL)
                {
                    return get_next_statement(prt->prt_stmt, caller_line);
                }
                else
                {
                    fprintf(stdout, "EXPR NODE has parrent NULL");
                    getchar();
                }
                break;

            case STMT_IF:
#ifdef RBC_NEXT
                printf ("IF");
#endif
                if (prt->prt_stmt != NULL)
                {
                    return get_next_statement(prt->prt_stmt, caller_line);
                }
                else
                {
                    fprintf(stdout, "IF NODE has parrent NULL");
                    getchar();
                }
                break;

            case STMT_ITERATOR:
#ifdef RBC_NEXT
                printf ("ITERATOR");
#endif
                if (prt->prt_stmt != NULL)
                {
                    return get_next_statement(prt->prt_stmt, caller_line);
                }
                else
                {
                    fprintf(stdout, "ITERATOR NODE has parrent NULL");
                    getchar();
                }
                break;

            case STMT_LABEL:
#ifdef RBC_NEXT
                printf ("LABEL");
#endif
                if (prt->prt_stmt != NULL)
                {
                    return get_next_statement(prt->prt_stmt, caller_line);
                }
                else
                {
                    fprintf(stdout, "LABEL NODE has parrent NULL");
                    getchar();
                }
                break;

            case STMT_NONE:
                fprintf (stdout, "statement NONE not handled\n");
                break;

            case STMT_RANGE:
                fprintf (stdout, "statement RANGE not handled\n");
                break;

            case STMT_RETURN:
#ifdef RBC_NEXT
                printf ("RETURN");
#endif
                break;

            case STMT_SWITCH:
#ifdef RBC_NEXT
                printf ("SWITCH");
#endif
                if (prt->prt_stmt != NULL)
                {
                    return get_next_statement(prt->prt_stmt, caller_line);
                }
                else
                {
                    fprintf(stdout, "SWITCH NODE has parrent NULL");
                    getchar();
                }
                break;

            default:
                fprintf (stdout, "statement NO STMT TYPE not handled\n");
        }

#ifdef RBC_NEXT
        printf ("#%d\n", prt->pos.line);
#endif
    }

    return NULL;
}

static void *
lookup_var (struct statement *stmt)
{
    int ret_eval_expr = -1;
    void *to_eval = (char *)"OK";

    if (stmt != NULL)
    {
        switch (stmt->type)
        {
            case STMT_IF:
                if (lookup_expression(stmt->if_conditional) == 0)
                {
                    to_eval = NULL;
                }
                break;
            case STMT_SWITCH:
                if (lookup_expression(stmt->switch_expression) == 0)
                {
                    to_eval = NULL;
                }
                break;
            case STMT_CASE:
                break;
            case STMT_RETURN:
                ret_eval_expr = lookup_expression(stmt->ret_value);
                if (ret_eval_expr == 0)
                {
                    to_eval = NULL;
                }
                break;
            case STMT_EXPRESSION:
                if (lookup_expression(stmt->expression) == 0)
                {
                    to_eval = NULL;
                }
                break;
#ifdef RBC_FIND_STMT
            default:
                printf("unhandled lookup statement");
                getchar();
#else
            default:
                return to_eval;
#endif
        }
    }

    return to_eval;
}

#define RBC_FIND_EXPR 1
#undef RBC_FIND_EXPR

static int
lookup_expression(struct expression *expr)
{
    struct expression *fn_arg = NULL;

    if (expr != NULL)
    {
        char *symbol = build_symbol(expr);
        if (symbol && AssignmentVariable && strcmp(symbol, AssignmentVariable) == 0)
        {
            return 0;
        }

        switch(expr->type)
        {
            case EXPR_CALL:
                FOR_EACH_PTR_REVERSE(expr->args, fn_arg)
                {
                    if (!lookup_expression(fn_arg))
                    {
                        return 0;
                    }
                } END_FOR_EACH_PTR_REVERSE(fn_arg);
                return -1;

	case EXPR_ASSIGNMENT:
            return lookup_expression(expr->right);

        case EXPR_BINOP:
	case EXPR_COMPARE:
	case EXPR_LOGICAL:
	case EXPR_COMMA:
            if (!lookup_expression(expr->left))
            {
                return 0;
            }
            else
            {
                return lookup_expression(expr->right);
            }

	case EXPR_PREOP:
            return lookup_expression(expr->unop);

	case EXPR_POSTOP:
#ifdef RBC_FIND_EXPR
            printf("postop");
#endif
            break;

	case EXPR_SYMBOL:
#ifdef RBC_FIND_EXPR
            printf ("symbol");
#endif
            if (expr->symbol && expr->symbol->ident && expr->symbol->ident->name && AssignmentVariable)
            {
                if (strcmp(expr->symbol->ident->name, AssignmentVariable) == 0)
                {
                    return 0;
                }
            }
            break;

	case EXPR_CAST:
	case EXPR_FORCE_CAST:
	case EXPR_IMPLIED_CAST:
#ifdef RBC_FIND_EXPR
            printf("cast");
#endif
            return lookup_expression(expr->cast_expression);
            break;

	case EXPR_VALUE:
#ifdef RBC_FIND_EXPR
            printf("value");
#endif
            break;

	case EXPR_FVALUE:
#ifdef RBC_FIND_EXPR
            printf("fvalue");
#endif
            break;

	case EXPR_STRING:
#ifdef RBC_FIND_EXPR
            printf("string");
#endif
            break;

	case EXPR_INITIALIZER:
#ifdef RBC_FIND_EXPR
            printf("init");
#endif
            break;

	case EXPR_SELECT:
	case EXPR_CONDITIONAL:
#ifndef RBC_FIND_EXPR
            printf("conditional");
#endif
            if (!lookup_expression(expr->conditional))
            {
                printf (" - ret 0");
                getchar();
                return 0;
            }
            else if (!lookup_expression(expr->cond_true))
            {
                printf (" - ret 0");
                getchar();
                return 0;
            }
            return lookup_expression(expr->cond_false);

	case EXPR_STATEMENT:
#ifdef RBC_FIND_EXPR
            printf("statement");
            getchar();
#endif
            break;

	case EXPR_LABEL:
#ifdef RBC_FIND_EXPR
            printf("label");
#endif
            break;

	case EXPR_SLICE:
#ifdef RBC_FIND_EXPR
            printf("slice");
#endif
            break;
#ifdef RBC_FIND_EXPR
        default:
                printf("unhandled lookup_expression\n");
                getchar();
#endif

        }
    }

    return -1;
}

static int
validate_expr(struct expression *expr)
{
    int ret_value = -1;
    struct expression *crs = expr->prt_expr;

    while (crs != NULL)
    {
        if (crs->prt_stmt)
        {
            if (crs->prt_stmt->type == STMT_IF ||
                crs->prt_stmt->type == STMT_SWITCH ||
                crs->prt_stmt->type == STMT_RETURN ||
                (crs->prt_stmt->prt_stmt &&
                 crs->prt_stmt->prt_stmt->type == STMT_RETURN &&
                 crs->prt_stmt->type == STMT_EXPRESSION))
            {
                ret_value = 0;
                break;
            }
            else if (crs->type == EXPR_ASSIGNMENT)
            {
                /* TODO check if the variable the value is assigned is used in IF/SWITCH statements */
                ret_value = 0;
                break;
            }
        }

        crs = crs->prt_expr;
    }

    return ret_value;
}

static char *
get_assignment_var(struct expression *expr)
{
/*
   char *ret_value = NULL;

    if (expr != NULL && expr->left && expr->left->symbol_name)
    {
        ret_value = expr->left->symbol_name->name;
    }
*/

    return build_symbol(expr);
}

static char *
get_caller(struct expression *expr)
{
    char *fn_name = NULL;

    if (expr && expr->fn &&
        expr->fn->symbol_name)
    {
        fn_name = expr->fn->symbol_name->name;
    }

    return fn_name;
}

static struct expression *
get_last_expression(struct expression *expr)
{
    struct expression *ret_expr = expr;

    while (ret_expr->prt_expr != NULL)
    {
        ret_expr = ret_expr->prt_expr;
        if (ret_expr->type == EXPR_ASSIGNMENT)
        {
            return NULL;
        }
    }

    return ret_expr;
}

static void *
examine_stmt_indent (struct statement *stmt)
{
    static void *ret_ok = (void *)"OK";

    if (stmt != NULL)
    {
        if (stmt->type == STMT_COMPOUND)
        {
            stmt->indent_level = (stmt->prt_stmt) ? stmt->prt_stmt->indent_level : 1;
        }
        else
        {
            stmt->indent_level = (stmt->prt_stmt) ? stmt->prt_stmt->indent_level + 1 : 1;
        }

        if (FileMapping)
        {
            int i;
            int start_index = MappingIndex, end_index = 0;
            int tab_count = 0, space_count = 0;

            for (i = CurrentLine; i < stmt->pos.line; i++)
            {
                PrevLine = CurrentLine;
                start_index = MappingIndex;

                while (FileMapping[MappingIndex] != '\n') MappingIndex++;
                CurrentLine++; MappingIndex++;
            }
            
            if (GlobalFlags & (1<<ERR_TABS_SPACES))
            {
                while (FileMapping[start_index] == ' ' || FileMapping[start_index] == '\t')
                {
                    if (FileMapping[start_index] == ' ') { space_count++; }
                    if (FileMapping[start_index] == '\t') { tab_count++; }
                    start_index++;
                }

                if (space_count && tab_count)
                {
                    printf ("\t\tMixed tabs with spaces. SPACES: %d TABS: %d at line %d\n",space_count, tab_count, stmt->pos.line);
                }
            }

            if (GlobalFlags & (1<<ERR_TRAYLING_WS))
            {
                end_index = MappingIndex - 2;
                while (FileMapping[end_index] == ' ' || FileMapping[end_index] == '\t')
                {
                    end_index--;
                }

                if (MappingIndex - end_index - 2 >= 2)
                {
                    printf ("\t\tTrailing white space: at line %d\n", stmt->pos.line);
                }
            }
        }

        CurrentIndentLevel = MAX_NUM(CurrentIndentLevel, stmt->indent_level);
        LastInstructionLine = MAX_NUM(LastInstructionLine, stmt->pos.line);
    }

    return ret_ok;
}

static int
handle_out_call_params(struct expression *expr)
{
    int show_err = 1;
    char *out_var_name = NULL;
    struct expression_list *list_args = NULL;
    struct expression *arg = NULL;
    struct statement *start_statement = NULL;

    if (expr != NULL && expr->type == EXPR_CALL && expr->args !=  NULL)
    {
        list_args = expr->args;

        FOR_EACH_PTR(list_args, arg)
        {
            if (arg != NULL)
            {
                out_var_name = get_out_symbol(arg);
                if (!out_var_name) { continue; }

                AssignmentVariable = out_var_name;

                start_statement = get_expression_statement(expr);
                if (start_statement)
                {
                    if (check_start_statement(start_statement, expr) == 0)
                    {
                        show_err = 0;
                        break;
                    }
                }
                else if (validate_expr(expr))
                {
                    emit_error(CurrentFile, AnalyzedFunction, expr->pos.line, "Unverified return value of function call handle out");
                }

                AssignmentVariable = NULL;
            }
        }END_FOR_EACH_PTR(arg);
    }

    return show_err;
}

static int
check_start_statement(struct statement *start_statement, struct expression *expr)
{
    if (start_statement->type == STMT_IF ||
        start_statement->type == STMT_SWITCH ||
        start_statement->type == STMT_RETURN ||
        start_statement->type == STMT_ITERATOR)
    {
        return 0;
    }

    return find_usages(start_statement, expr);
}

static int
handle_declaration(struct expression *call_expr, struct statement *decl_stmt)
{
    int found = 0, count = 0, ret_value = 0;
    char *var = NULL;
    struct symbol *sym = NULL;
    struct expression *e = call_expr;

    if (call_expr && call_expr->type == EXPR_CALL &&
        decl_stmt && decl_stmt->type == STMT_DECLARATION)
    {
        FOR_EACH_PTR(decl_stmt->declaration, sym)
        {
            e = call_expr;
            while (e != NULL)
            {
                if (e == sym->initializer)
                {
                    found = 1;
                    var = sym->ident->name;
                    break;
                }

                e = e->prt_expr;
            }

            count++;
        } END_FOR_EACH_PTR(sym);

        if (found)
        {
            AssignmentVariable = var;
            AnalyzedFunction = get_caller(call_expr);

            if (check_start_statement(decl_stmt, call_expr))
            {
                ret_value = 1;
            }

            AssignmentVariable = NULL;
        }
    }

    return ret_value;
}

static int
is_declaration_call(struct expression *expr, struct statement **ret_statement)
{
    struct statement *st = NULL;

    *ret_statement = NULL;

    if (expr && expr->type == EXPR_CALL)
    {
        st = get_expression_statement(expr);

        *ret_statement = st;

        return st && st->type == STMT_DECLARATION;
    }

    return 0;
}

static char *
get_out_symbol (struct expression *expr)
{
    if (expr)
    {
        switch (expr->type)
        {
            case EXPR_SYMBOL:
                if (expr->symbol && expr->symbol->ident)
                {
                    return expr->symbol->ident->name;
                }
                break;

            case EXPR_CAST:
            case EXPR_FORCE_CAST:
            case EXPR_IMPLIED_CAST:
                return get_out_symbol(expr->cast_expression);

            default:
#ifdef RBC_RBC_DEBUG
                fprintf (stdout, "Unhandled out parameter type: %d\n", expr->type);
#else
                ; /* NOP */
#endif
        }
    }

    return NULL;
}

static struct expression *
get_call_node (struct expression *expr)
{
    struct expression *temp_expr = NULL;

    if (expr)
    {
        switch (expr->type)
        {
            case EXPR_CALL:
                return expr;

            case EXPR_ASSIGNMENT:
                return get_call_node(expr->right);

            case EXPR_COMMA:
            case EXPR_BINOP:
            case EXPR_COMPARE:
            case EXPR_LOGICAL:
                temp_expr = get_call_node(expr->left);
                if (!temp_expr)
                {
                    temp_expr = get_call_node(expr->right);
                }
                return temp_expr;

            case EXPR_CAST:
            case EXPR_FORCE_CAST:
            case EXPR_IMPLIED_CAST:
                return get_call_node(expr->cast_expression);

            case EXPR_SELECT:
            case EXPR_CONDITIONAL:
                temp_expr = get_call_node(expr->conditional);
                if (!temp_expr)
                {
                    temp_expr = get_call_node(expr->cond_true);
                    if (!temp_expr)
                    {
                        temp_expr = get_call_node(expr->cond_false);
                    }
                }
                return temp_expr;

            case EXPR_SLICE:
                return get_call_node(expr->base);

            default:
                printf("error - invalid expression");
        }
    }

    return NULL;
}

static char *
build_symbol(struct expression *expr)
{
    char *ret_string = NULL, *tmp = NULL;
    static char *end_string = (char *)"vec_acc";

    if (expr)
    {
        switch (expr->type)
        {
            case EXPR_CALL:
                return build_symbol(expr->fn);

            case EXPR_ASSIGNMENT:
                return build_symbol(expr->left);

            case EXPR_PREOP:
                if (expr->op == '*')
                {
                    return build_symbol(expr->unop);
                }
                break;

            case EXPR_SYMBOL:
                if (expr->symbol && expr->symbol->ident)
                {
#ifdef VEC_ACCESS
                    int i = 0;
                    for (i = 0; i < DeclaredIntValuesSize; i++)
                    {
                        if (strcmp(IntVariables[i], expr->symbol->ident->name) == 0)
                        {
                            printf ("\nfound value = %d\n", IntValues[i]);
                            break;
                        }
                    }
#endif
                    return expr->symbol->ident->name;
                }
                break;

            case EXPR_CAST:
            case EXPR_FORCE_CAST:
            case EXPR_IMPLIED_CAST:
                return build_symbol(expr->cast_expression);

            case EXPR_SLICE:
                return build_symbol(expr->base);

            case EXPR_BINOP:
                tmp = build_symbol(expr->left);
#ifdef VEC_ACCESS
                build_symbol(expr->right);
#else
                if (tmp)
                {
                    ret_string = malloc(strlen(tmp) + strlen(end_string) + 1);
                    if (ret_string)
                    {
                        sprintf(ret_string, "%s_%s", tmp, end_string);
                        return ret_string;
                    }
                }
#endif
                return tmp;

            case EXPR_VALUE:
#ifdef VEC_ACCESS
                printf ("value: #%d#", expr->value);
#endif
                break;

            default:
#ifdef VEC_ACCESS
                printf("error - invalid expression in build_symbol: %d", expr->type);
#else
                ;
#endif
        }
    }

    return ret_string;
}

#ifdef VEC_ACCESS
static void
parse_init_statement(struct statement *stmt)
{
    struct symbol *sym = NULL;

    if (stmt == NULL || stmt->type != STMT_DECLARATION)
    {
        return;
    }

    FOR_EACH_PTR(stmt->declaration, sym)
    {
        char *str_type = get_type(sym->ctype.base_type);

        if (str_type && strchr(str_type, '*') == NULL)
        {
            if (sym->initializer && sym->initializer->type == EXPR_VALUE)
            {
                IntVariables[DeclaredIntValuesSize] = sym->ident->name;
                IntValues[DeclaredIntValuesSize] = sym->initializer->value;
                DeclaredIntValuesSize++;
#ifdef VAR_INIT
                printf ("\tvariable: #%s#", sym->ident->name);
                printf ("initizalization with: %lld\n", sym->initializer->value);
#endif
            }
        }
    } END_FOR_EACH_PTR(sym);
}
#endif

static int
continue_parsing(int flags)
{
   int i;

   for (i = ERR_FUN_CALL + 1; i < ERR_MAX; i++)
   {
       if (flags & (1<<i))
       {
           return 1;
       }
   }

    return 0;
}

int main(int argc, char **argv)
{
   int flags, opt, status = 0;

   int new_argc, i;
   char **new_argv = NULL;

   if (argc < 2)
   {
        fprintf(stderr, "Usage: %s [-c ][-l line count] [-i indent level] [-s trailing/mixed-begin] [file list]\n",
                   argv[0]);
        exit(EXIT_FAILURE);
   }

   flags = 0;
   int args = 1;
   while ((opt = getopt(argc, argv, "cl:i:s:")) != -1)
   {
       switch (opt)
       {
       case 'c':
           flags |= 1<<ERR_FUN_CALL;
           args++;
           break;
       case 'l':
           MaxLineCount = atoi(optarg);
           flags |= 1<<ERR_FUN_LINES;

           args += 2;
           break;
       case 'i':
           MaxIndentLevel = atoi(optarg);
           flags |= 1<<ERR_FUN_INDENT_LEVEL;

           args += 2;
           break;
       case 's':
           if (optarg != NULL)
           {
               if (strcmp(optarg, "trailing") == 0)
               {
                   flags |= 1<<ERR_TRAYLING_WS;
               }
               else if (strcmp(optarg, "mixed-begin") == 0)
               {
                   flags |= 1<<ERR_TABS_SPACES;
               }
               else
               {
                   fprintf(stderr, "Usage: %s [-s trailing/mixed-begin]\n",
                                    argv[0]);
                   exit(EXIT_FAILURE);
               }

               args += 2;
           }
           break;
       default: /* '?' */
           fprintf(stderr, "Usage: %s [-c ][-l line count] [-i indent level] [-s trailing/mixed-begin] [file list]\n",
                   argv[0]);
           exit(EXIT_FAILURE);
       }
   }

   status = set_black_list();
   if (status)
   {
       perror("black list creation");
       exit(EXIT_FAILURE);
   }

   new_argc = argc - args + 1;
   new_argv = malloc((new_argc + 1) * sizeof (char *));
   new_argv[0] = strdup(argv[0]);
   for (i = args; i < argc; i++)
   {
       new_argv[i - args + 1] = strdup(argv[i]);
   }

   if (flags & (1<<ERR_FUN_CALL))
   {
       printf ("\t\t\t\tCall Errors:\n");        
       parse_file_list(new_argc - 1, new_argv, examine_stmt_expression, 1<<ERR_FUN_CALL);

       flags &= ~(1<<ERR_FUN_CALL);
   }

   if (continue_parsing(flags))
   {
       printf ("\t\t\t\tOther errors:\n");
       parse_file_list(new_argc - 1, new_argv, examine_stmt_indent, flags);
   }

   return 0;
}

