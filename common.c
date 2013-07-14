#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "common.h"

MAX_MIN_FUNC(int)

int test_error_noexit(int jud, const char *fmt, ...)
{
    va_list va;
    if(!jud) return jud;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");
    va_end(va);
    return jud;
}

/* Connects @a and @b, without modifying actual data.
 * Result string shouldn't exceed PATH_MAX, exceeding part will be truncated. */
const char *strconn(const char *a, const char *b)
{
    static fpath_t res;
    strncpy(res, a, int_min(PATH_MAX, strlen(a))+1);
    strncat(res, b, int_min(PATH_MAX-strlen(res), strlen(b)));
    return res;
}

char *string_tolower(char *s)
{
    int i;
    for(i=0; s[i]; i++) s[i]=tolower(s[i]);
    return s;
}

char *string_toupper(char *s)
{
    int i;
    for(i=0; s[i]; i++) s[i]=toupper(s[i]);
    return s;
}

/* Connects to a full path. @base can't be ended with '/'. */
const char *absolute_path(const char *rel, const char *base)
{
    static fpath_t res;
    if(rel[0]=='/') return rel;
    strncpy(res, base, int_min(PATH_MAX, strlen(base))+1);
    strncat(res, "/", int_min(PATH_MAX-strlen(res), 1));
    strncat(res, rel, int_min(PATH_MAX-strlen(res), strlen(rel)));
    return res;
}

extern pid_t child_pid;

void finish(void)
{
    kill(-(int)child_pid, SIGKILL);
}
