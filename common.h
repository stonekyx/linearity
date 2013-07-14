#ifndef LINEARITY_COMMON_H

#define LINEARITY_COMMON_H

#include "limits.h"
#include <malloc.h>
#include <stdlib.h>

typedef char fname_t[NAME_MAX+1];
typedef char fpath_t[PATH_MAX+1];
typedef char prob_name_t[PROB_NAME_MAX+1];

#define MAX_MIN_FUNC(type)   \
    type type ## _max(type a, type b) \
{   \
    return a>b?a:b;   \
}   \
type type ## _min(type a, type b)  \
{   \
    return a<b?a:b;   \
}

#define malloc_with_type(type)  \
    ((type *)malloc(sizeof(type)))

#define MAKE_LINKED_LIST(type)   \
    struct type ## _link_t     \
{                               \
    struct type ## _t v;           \
    struct type ## _link_t *next;        \
}

#define INSERT_LINKED_LIST(type, tar, val) \
    do {  \
        struct type ## _link_t *_new = \
        (struct type ## _link_t *)malloc(sizeof(struct type ## _link_t));  \
        _new->v = val; \
        _new->next = tar->next; \
        tar->next = _new; \
    } while(0)

int test_error_noexit(int, const char *, ...);

#define test_error(...) \
    do \
{ \
    if(test_error_noexit(__VA_ARGS__)) exit(1); \
} while (0)

const char *strconn(const char *, const char *);

char *string_toupper(char *);
char *string_tolower(char *);

const char *absolute_path(const char *, const char *);

typedef enum { EXIT_OK, EXIT_ERROR, EXIT_FAIL } ret_state;

void finish(void);

#endif
