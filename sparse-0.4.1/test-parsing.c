/*
 * Example trivial client program that uses the sparse library
 * to tokenize, preprocess and parse a C file, and prints out
 * the results.
 *
 * Copyright (C) 2003 Transmeta Corp.
 *               2003 Linus Torvalds
 *
 *  Licensed under the Open Software License version 1.1
 */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"
#include "allocate.h"
#include "token.h"
#include "parse.h"
#include "symbol.h"
#include "expression.h"

static void clean_up_symbols(struct symbol_list *list)
{
	struct symbol *sym;

	FOR_EACH_PTR(list, sym) {
		expand_symbol(sym);
	} END_FOR_EACH_PTR(sym);
}

void
examine_list(struct symbol_list *list)
{
    struct symbol *sym = NULL;

    if (!list)
    {
        return;
    }
	FOR_EACH_PTR(list, sym)
        {
            printf ("#####analyzing: %s\n", sym->ident->name);
            if (sym->ctype.base_type != NULL)
            {
                struct symbol *type = sym->ctype.base_type;
                if (type->type == SYM_FN)
                {
                    printf ("\tfunction\n");
                    struct statement *stmt = type->stmt;
                    if (stmt != NULL && stmt->stmts != NULL)
                    {
                        #if 1
                            printf ("\t\t has statements\n");
                        #endif

                        int end_line = 0;
                        struct statement_list *st_list = stmt->stmts;
                        struct statement *crt_statement = NULL;

                        #if 0
                            printf("fst statement: %d - %d\n", stmt->type, stmt->pos.line);
                        #endif

                        FOR_EACH_PTR(st_list, crt_statement)
                        {
                            #if 0
                                printf ("%d - %d\n", crt_statement->type, crt_statement->pos.line);
                            #endif

                            end_line = crt_statement->pos.line;
                        }END_FOR_EACH_PTR(crt_statement);


                        printf ("\tlast statement line: %d\n", end_line);
                    }
                }
            }
	} END_FOR_EACH_PTR(sym);
}

int main(int argc, char **argv)
{
	struct symbol_list * list;
	struct string_list * filelist = NULL;
	char *file;

	list = sparse_initialize(argc, argv, &filelist);

	// Simplification
	clean_up_symbols(list);

#if 1
	show_symbol_list(list, "\n\n");
	printf("\n\n");

        printf ("------------------------\n");
#endif

	FOR_EACH_PTR_NOTAG(filelist, file) {
		list = sparse(file);

		// Simplification
		clean_up_symbols(list);

#if 1
		// Show the end result.
		show_symbol_list(list, "\n\n");
		printf("\n\n");
#endif
                //examine_list(list);
	} END_FOR_EACH_PTR_NOTAG(file);

#if 0
	// And show the allocation statistics
	show_ident_alloc();
	show_token_alloc();
	show_symbol_alloc();
	show_expression_alloc();
	show_statement_alloc();
	show_string_alloc();
	show_bytes_alloc();
#endif

	return 0;
}
