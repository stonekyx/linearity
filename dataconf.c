#include "dataconf.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

extern const char *base_dir;
static int jump_space(FILE *);
static int read_to_next(FILE *, int);
static int read_literal(FILE *, const char *);
static int test_next(FILE *);
int parse_problem(const fpath_t, struct prob_link_t *);
static int parse_data(FILE *, struct prob_t *);
static int init_data(struct prob_t *);

/* Jump through space, returning the next non-space character. */
static int jump_space(FILE *fin)
{
    int t;
    while(isspace(t=fgetc(fin)));
    return t;
}

static int read_to_next(FILE *fin, int x)
{
    int t;
    t=jump_space(fin);
    if(t==x) return EXIT_OK;
    ungetc(t, fin);
    return EXIT_FAIL;
}

/* Reads literal string from FILE *fin, trailing spaces will be ignored.
 * On cases that given literal doesn't match, read parts will be *put back*.
 */
static int read_literal(FILE *fin, const char *x)
{
    const char *i;
    for(i=x; *i; i++)
    {
        if(isspace(*i)) continue;
        if(read_to_next(fin, *i)==EXIT_FAIL)
        {
            while(--i>=x)
            {
                ungetc(*i, fin);
            }
            return EXIT_FAIL;
        }
    }
    return EXIT_OK;
}

/* Gets next non-space character in FILE *fin, puts it back, 
 * and returns it. 
 * Return -1 on error.
 */
static int test_next(FILE *fin)
{
    int res=jump_space(fin);
    if(ungetc(res, fin)==EOF) return -1;
    return res;
}

static char *fgets_strip(char *s, int size, FILE *stream)
{
    char *res;
    int len;
    if(test_next(stream)==-1) return NULL;
    if(!(res=fgets(s, size, stream))) return res;
    len=strlen(res);
    while(len && isspace(res[len-1])) res[--len]=0;
    return res;
}

static const
char *str_fmterr="Data configuration is not correctly formatted",
     *str_allocerr="Error allocating for storage",
     *str_parseerr="Error parsing data configuration";

/* The only exported function.
 * Parses one problem, returning -1 on parse error, 0 on success, 1 on end.
 */
int parse_problem(const fpath_t dc, struct prob_link_t *res)
{
    FILE *fdc = fopen(dc, "r");
    int exit_status=EXIT_OK;

    test_error(!fdc, "Can't open %s for read", dc);

    while(1)
    {
        static struct prob_t new;

        if(read_literal(fdc, "problem")==EXIT_FAIL)
        {
            exit_status = EXIT_OK;
            break;
        }
        fgets_strip(new.name, PROB_NAME_MAX, fdc);

        string_tolower(new.name);

        test_error(read_literal(fdc, "judge ignore")!=EXIT_OK, str_fmterr);
        switch(test_next(fdc))
        {
            case 'e':
                test_error(read_literal(fdc, "emptyline white")!=EXIT_OK, str_fmterr);
                new.jmode = IGN_WHITE;
                break;
            case 'n':
                test_error(read_literal(fdc, "nothing")!=EXIT_OK, str_fmterr);
                new.jmode = IGN_NOT;
                break;
            default:
                test_error(1, str_fmterr);
        }

        test_error(read_literal(fdc, "input")!=EXIT_OK, str_fmterr);
        fgets_strip(new.inp, NAME_MAX, fdc);

        test_error(read_literal(fdc, "output")!=EXIT_OK, str_fmterr);
        fgets_strip(new.outp, NAME_MAX, fdc);

        test_error(init_data(&new)!=EXIT_OK, str_allocerr);

        test_error(parse_data(fdc, &new)!=EXIT_OK, str_parseerr);

        INSERT_LINKED_LIST (prob, res, new);

        res=res->next;
    }

    fclose(fdc);
    return exit_status;
}

/* Initializes dataset of problem new. */
static int init_data(struct prob_t *new)
{
    new->inp_data = (struct data_link_t *)malloc(sizeof(struct data_link_t));
    new->outp_data = (struct data_link_t *)malloc(sizeof(struct data_link_t));
    if(!new->inp_data || !new->outp_data) return EXIT_ERROR;
    new->inp_data->v.path[0] = new->outp_data->v.path[0] = 0;
    new->inp_data->next = new->outp_data->next = NULL;
    return EXIT_OK;
}

static int parse_data(FILE *fdc, struct prob_t *new)
{
    struct data_link_t *itail = new->inp_data,
                       *otail = new->outp_data;
    while(itail->next) itail=itail->next;
    while(otail->next) otail=otail->next;

    while(read_literal(fdc, "case")==EXIT_OK)
    {
        static struct data_t inew, onew;
        fscanf(fdc, "%*d");  /* case number ignored. */

        test_error(read_literal(fdc, "input")!=EXIT_OK, str_fmterr);
        fgets_strip(inew.path, PATH_MAX, fdc);
        memcpy(inew.path, absolute_path(inew.path, "data"), sizeof(inew.path));
        memcpy(inew.path, absolute_path(inew.path, base_dir), sizeof(inew.path));

        test_error(read_literal(fdc, "output")!=EXIT_OK, str_fmterr);
        fgets_strip(onew.path, PATH_MAX, fdc);
        memcpy(onew.path, absolute_path(onew.path, "data"), sizeof(onew.path));
        memcpy(onew.path, absolute_path(onew.path, base_dir), sizeof(onew.path));

        test_error(read_literal(fdc, "timelimit")!=EXIT_OK, str_fmterr);
        fscanf(fdc, "%lf", &inew.limits.time);

        test_error(read_literal(fdc, "memory")!=EXIT_OK, str_fmterr);
        fscanf(fdc, "%lf", &inew.limits.memory);

        test_error(read_literal(fdc, "score")!=EXIT_OK, str_fmterr);
        fscanf(fdc, "%lf", &inew.score);

        INSERT_LINKED_LIST (data, itail, inew);
        INSERT_LINKED_LIST (data, otail, onew);

        itail=itail->next;
        otail=otail->next;
    }
    return EXIT_OK;
}
