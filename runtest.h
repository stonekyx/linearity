#ifndef RUNTEST_H

#define RUNTEST_H

typedef enum{STAT_UNKNOWN, STAT_TLE, STAT_MLE, STAT_AC, STAT_WA, STAT_RTE} user_stat_t;

ret_state runtest(void);

#endif
