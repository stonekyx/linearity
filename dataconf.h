#ifndef DATACONF_H

#define DATACONF_H

#include "common.h"

struct res_limit_t
{
    double time, memory;
};

struct data_t
{
    fpath_t path;
    struct res_limit_t limits;
    double score;
};

MAKE_LINKED_LIST (data);

enum jmode_t
{
    IGN_NOT,
    IGN_WHITE,
    IGN_EOLN,    /* Consider performing end-of-line convert */
    SPEC_JUDGE   /* FIXME not implemented. */
};

struct prob_t
{
    prob_name_t name;
    enum jmode_t jmode;
    fname_t inp, outp;
    struct data_link_t *inp_data, *outp_data;
    /* NOTICE: only inp_data stores valid limits and score. */
};

MAKE_LINKED_LIST (prob);

int parse_problem(const fpath_t dc, struct prob_link_t *res);

#endif
