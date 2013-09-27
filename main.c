#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <getopt.h>

#include "common.h"
#include "dataconf.h"
#include "competitor.h"

uid_t nobody_uid;
char *spec_comp, *spec_prob, *dataconf_path;
const char *help_str = "help";
const char *version_str = "version";
struct prob_link_t *prob;
struct comp_link_t *comp;
const char *base_dir;

uid_t get_file_uid(const fpath_t path)
{
    static struct stat x;
    stat(path, &x);
    return x.st_uid;
}

int init_prob(void)
{
    prob = (struct prob_link_t *)malloc(sizeof(struct prob_link_t));
    test_error(!prob, "Error allocating storage");
    prob->v.name[0]=0;
    prob->v.inp_data = prob->v.outp_data = NULL;
    prob->next=NULL;
    return EXIT_OK;
}

static int file_readable(const fpath_t x)
{
    FILE *tmp=fopen(x, "r");
    if(!tmp) return EXIT_FAIL;
    fclose(tmp);
    return EXIT_OK;
}

static int __relative_data_valid(struct data_link_t *dp)
{
    if(file_readable(dp->v.path)!=EXIT_OK)
    {
        test_error_noexit(1, "File %s not readable, ignoring...", dp->v.path);
        dp->v.path[0]=dp->v.score=0;
        return EXIT_FAIL;
    }
    return EXIT_OK;
}

int check_io_validity(void)
{
    struct prob_link_t *pp;
    struct data_link_t *dp;
    int failcnt=0;
    for(pp=prob; pp; pp=pp->next)
    {
        if(pp->v.name[0]==0) continue;
        for(dp=pp->v.inp_data; dp; dp=dp->next)
        {
            if(dp->v.path[0]==0) continue;
            failcnt+=(__relative_data_valid(dp)!=EXIT_OK);
        }
        for(dp=pp->v.outp_data; dp; dp=dp->next)
        {
            if(dp->v.path[0]==0) continue;
            failcnt+=(__relative_data_valid(dp)!=EXIT_OK);
        }
    }
    return failcnt?EXIT_FAIL:EXIT_OK;
}

struct prob_link_t *prob_present(const prob_name_t name)
{
    struct prob_link_t *pp;
    for(pp=prob; pp; pp=pp->next)
    {
        if(!strcmp(name, pp->v.name)) return pp;
    }
    return NULL;
}

int init_comp(void)
{
    comp=malloc_with_type(struct comp_link_t);
    test_error(!comp, "Error allocating storage");
    comp->v.name[0]=0;
    comp->v.srcfile=NULL;
    comp->next=NULL;
    return 0;
}

const char *get_base_dir(void)
{
    int cursize=2;
    char *res=(char*)malloc(sizeof(char)*cursize);
    while(!(getcwd(res, cursize)) && errno==ERANGE)
    {
        free(res);
        cursize<<=1;
        res=(char*)malloc(sizeof(char)*cursize);
    }
    res=realloc(res, sizeof(char)*(strlen(res)+1));
    return res;
}

void handle(int sig)
{
}

int main(int argc, char *argv[])
{
    int c, help=0, version=0;
    struct prob_link_t *sp_link;

    signal(SIGINT, handle);
    signal(SIGTERM, handle);
    signal(SIGQUIT, handle);
    signal(SIGKILL, handle);
    signal(SIGFPE, handle);
    signal(SIGUSR1, handle);
    signal(SIGUSR2, handle);

    nobody_uid = get_file_uid(argv[0]);

    test_error(geteuid()!=nobody_uid, "This program needs set-user-id bit turned on.");

    while((c=getopt(argc, argv, "c:p:hv"))!=-1)
    {
        switch(c)
        {
            case 'c':
                spec_comp=optarg;
                break;
            case 'p':
                spec_prob=optarg;
                break;
            case 'h':
                help=1;
                break;
            case 'v':
                version=1;
                break;
            default:
                test_error(1, "Unknown argument");
        }
    }

    if(help) puts(help_str);
    if(version) puts(version_str);
    if(help || version) return 0;

    dataconf_path = argv[optind];
    if(!dataconf_path)
    {
        test_error_noexit(1, "Notice: using default ./dataconf.lsc");
        dataconf_path = "./dataconf.lsc";
    }

    base_dir = get_base_dir();

    test_error(init_prob()!=EXIT_OK, "Error allocating space for problem information.");

    test_error(parse_problem(dataconf_path, prob)!=EXIT_OK, "Error reading dataconf.");

    test_error_noexit(check_io_validity()!=EXIT_OK, "Warning: test data not fully valid.");
    
    test_error(spec_prob && !(sp_link=prob_present(spec_prob)), "Specified problem %s doesn't exist.", spec_prob);

    test_error(init_comp()!=EXIT_OK, "Error allocating space for competitor information.");

    test_error(get_comp_list(comp)!=EXIT_OK, "Error getting competitor list.");

    dump_comp(comp);
    return 0;
}
