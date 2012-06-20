/*
 * rbc_drmemory.c: drmemory module
 *
 * General description:
 * 	Checks for invalid free/memory access, memory leaks, usage of
 * uninitialized variables and unclosed file descriptors. Parses the
 * output of drmemory according to the bit set of tracked errors
 * (user can choose to track a part, none or all types of errors).
 *
 * (C) 2012, Laura Vasilescu <laura@rosedu.org>
 *              * last review 20.06.2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/dynamic_tool.h"

