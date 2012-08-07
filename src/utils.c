#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/utils.h"

static int 
my_tolower(int c)
{
	int x;

	if ('A' <= c && c >= 'Z')
		x = 'a' + c - 'A';

	return x;
}

int
my_strcasecmp(const char *s1, const char *s2)
{
	return my_strncasecmp(s1, s2, strlen(s1));
}

int
my_strncasecmp(const char *s1, const char *s2, int n)
{
	if (n == 0)
		return 0;

	while (n-- != 0 && my_tolower(*s1) == my_tolower(*s2)) {
		if (n == 0 || *s1 == '\0' || *s2 == '\0')
			break;
		s1++;
		s2++;
	}

	return my_tolower(*(unsigned char *) s1) - my_tolower(*(unsigned char *) s2);
}

char *
my_strcasestr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = my_tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)my_tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

char *
my_strdup(const char *s)
{
	char *x = calloc(strlen(s) + 1, sizeof(char));
	strcpy(x, s);

	return x;
}

int
cmp_msg_file (char *m1, char *m2)
{
	char *name1 = NULL, *name2 = NULL;
	char *n1 = strdup(m1);
	char *n2 = strdup(m2);
	char *p1 = strcasestr(n1, "in file");
	char *p2 = strcasestr(n2, "in file");
	int ret = 0, len1 = 0, len2 = 0, len_start = 0;

	if (p1 == NULL || p2 == NULL)
		return 0;

	p1 += 8;
	p2 += 8;

	trim_whitespace(p1);
	trim_whitespace(p2);

	name1 = strtok(p1, ", ");
	name2 = strtok(p2, ", ");

	len1 = strlen(name1);
	len2 = strlen(name2);
	len_start = name1 - n1;

	if (strcasecmp(name1, name2) == 0)
		ret = 1;
	if (strstr(name1, name2))
		ret = 1;
	if (strstr(name2, name1))
		ret = 1;

	/* Compare beginning. */
	if (strncasecmp(n1, n2, len_start))
		ret = 0;

	free(n1);
	free(n2);

	/* Compare end. */
	n1 = strdup(m1);
	n2 = strdup(m2);
	if (strcasecmp(n1 + len_start + len1, n2 + len_start + len2))
		ret = 0;

	free(n1);
	free(n2);

	return ret;
}


void
trim_whitespace (char *str)
{
	char *whitespace = " \t\n\r";
	int len = strlen(str), i = len - 1;
	
	/* Trailing whitespace */
	while(i >= 0)
	{
		if (strchr(whitespace, str[i]) == NULL)
			break;
		str[i] = 0;
		i--;
		len--;
	}

	/* Starting whitespace */
	i = 0;
	while(i < len)
	{
		if (strchr(whitespace, str[i]) == NULL)
			break;
		i++;
	}

	if (i)
		memmove(str, str + i, len - i + 1);
}
