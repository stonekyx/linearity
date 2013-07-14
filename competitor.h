#ifndef COMPETITOR_H

#define COMPETITOR_H

#include "common.h"
#include "dataconf.h"

struct srcfile_t
{
    fpath_t path;
    struct prob_link_t *prob;
};

MAKE_LINKED_LIST (srcfile);

struct comp_t
{
    fname_t name;
    struct srcfile_link_t *srcfile;
};

MAKE_LINKED_LIST (comp);

int get_comp_list(struct comp_link_t *);

void dump_comp(struct comp_link_t *);

#endif
