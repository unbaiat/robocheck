#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "black_list.h"

static char **BlackList = NULL;
static int BlackListCount = 0;

int
set_black_list()
{
    int str_size, inc = BL_SIZE_INC, ret_value = 1;
    char line[MAX_BUFF_SIZE];
    FILE *f_in = NULL;

    if (BlackList != NULL)
    {
        /* singleton */
        ret_value = 0;
        goto exit;
    }

    BlackListCount = 0;
    BlackList = malloc(BL_SIZE_INC * sizeof(**BlackList));
    if (BlackList == NULL)
    {
        perror("malloc");
        goto exit;
    }

    f_in = fopen("black_list", "r");
    if (f_in == NULL)
    {
        perror("fopen");
        goto exit;
    }

    while (fgets(line, MAX_BUFF_SIZE, f_in))
    {
        str_size = strlen(line);
        line[str_size - 1] = '\0';

        if (BlackListCount && (BlackListCount % BL_SIZE_INC) == 0)
        {
            inc += BL_SIZE_INC;
            BlackList = realloc(BlackList, sizeof(**BlackList) * inc);
            if (BlackList == NULL)
            {
                goto exit;
            }
        }

        BlackList[BlackListCount++] = strdup(line);
    }

    ret_value = 0;

exit:
    return ret_value;
}

int
is_black_listed(char *func_name)
{
    int is_listed = 0, i;

    if (func_name == NULL) { goto exit; }

    for (i = 0; i < BlackListCount; i++)
    {
        if (strcmp(BlackList[i], func_name) == 0)
        {
            is_listed = 1;
            goto exit;
        }
    }

exit:
    return is_listed;
}