#include "common.h"
#include "competitor.h"

#include <dirent.h>
#include <errno.h>
#include <string.h>

extern struct prob_link_t *prob;
extern const char *base_dir;

static int src_match(const char *file, const char *prob)
{
    return !strncmp(file, prob, strrchr(file, '.')-file);
}

/*
 * Get several source files from the user directory @srcent,
 * store results in @res.
 */
static struct comp_link_t *get_comp_info(struct dirent *srcent, struct comp_link_t *res)
{
    char *compdirpath=(char*)malloc(sizeof(char)*(strlen(srcent->d_name)+4+2));
    DIR *compdir;
    struct dirent *compent;
    struct srcfile_link_t *srcfile;

    strcpy(compdirpath, strconn("src/", srcent->d_name));
    compdir = opendir(compdirpath);

    if(test_error_noexit(!compdir, "Error opening src/%s, ignoring...", srcent->d_name)) return res;

    res->next = malloc_with_type(struct comp_link_t);
    res=res->next;
    srcfile=res->v.srcfile = malloc_with_type(struct srcfile_link_t);
    strcpy(res->v.name, srcent->d_name); /* competitor's name */
    res->next=NULL;
    srcfile->v.path[0]=0; /*header node.*/
    while((compent=readdir(compdir)))   /* srcfile is res->v.srcfile, the target. */
    {
        struct prob_link_t *tmp;
        if(!strcmp(compent->d_name, ".")) continue;
        if(!strcmp(compent->d_name, "..")) continue;

        for(tmp=prob->next; tmp; tmp=tmp->next)
        {
            if(src_match(compent->d_name, tmp->v.name)) /* compent->d_name is source file name */
            {
                srcfile->next=malloc_with_type(struct srcfile_link_t);
                srcfile=srcfile->next;
                memcpy(srcfile->v.path, absolute_path(compent->d_name, compdirpath), sizeof(srcfile->v.path));
                memcpy(srcfile->v.path, absolute_path(srcfile->v.path, base_dir), sizeof(srcfile->v.path));
                /*XXX: v.path can be updated to use pointer.*/
                srcfile->v.prob=tmp;
                srcfile->next=NULL;
            }
        }
    }

    closedir(compdir);
    return res;
}

int get_comp_list(struct comp_link_t *res)
{
    DIR *srcdir=opendir("src/");
    struct dirent *srcent;
    if(test_error_noexit(!srcdir, "Error opening src/")) return EXIT_ERROR;
    
    errno=0; /* XXX: Is this OK? */
    while((srcent=readdir(srcdir)))
    {
        if(!strcmp(srcent->d_name, ".")) continue;
        if(!strcmp(srcent->d_name, "..")) continue;

        res=get_comp_info(srcent, res);
    }
    if(errno)
    {
        perror("get_comp_list");
        return EXIT_ERROR;
    }

    closedir(srcdir);
    return EXIT_OK;
}

void dump_comp(struct comp_link_t *comp)
{
    struct comp_link_t *q;
    puts("+++++++++++++++++++++++++++++comp information++++++++++++++++++++++++");
    for(q=comp->next; q; q=q->next)
    {
        struct srcfile_link_t *p;
        printf("Competitor %s\n", q->v.name);
        for(p=q->v.srcfile->next; p; p=p->next)
        {
            printf("\tsrcfile %s for problem %s\n", p->v.path, p->v.prob->v.name);
        }
    }
    puts("-----------------------------end comp information--------------------");
}
