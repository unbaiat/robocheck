/* 
 * File:   black_list.h
 * Author: Socoteanu Cezar
 *
 * Created on June 24, 2011, 6:24 PM
 */

#ifndef BLACK_LIST_H
#define	BLACK_LIST_H

#define MAX_BUFF_SIZE       1024
#define BL_SIZE_INC         100

int
set_black_list(void);

int
is_black_listed(char *);

#endif	/* BLACK_LIST_H */

