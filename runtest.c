#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"
#include "runtest.h"
#include "dataconf.h"
#include "competitor.h"

extern struct prob_link_t *prob;
extern struct comp_link_t *comp;
extern const char *base_dir;
extern uid_t nobody_uid;

fpath_t tmpdir;

pid_t child_pid;

static ret_state prepare_temp(void);
static ret_state compile(struct srcfile_link_t *);
static void *_run(void *);
static user_stat_t verify(int, struct prob_link_t *, struct data_link_t *, struct data_link_t *);
static ret_state run(struct srcfile_link_t *);
static ret_state clean_up(void);
static ret_state run_competitor(struct comp_link_t *);
ret_state runtest(void);

static ret_state prepare_temp(void)
{
    strcpy(tmpdir, "/tmp/linear-XXXXXX");
    test_error(!mkdtemp(tmpdir), "prepare_temp: %s", strerror(errno));
    chmod(tmpdir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH | S_ISVTX); /* drwxrwxrwt */
    return EXIT_OK;
}

static ret_state compile(struct srcfile_link_t *src)
{
    printf("Compiling source file %s\n", strrchr(src->v.path, '/')+1);

    symlink(src->v.path, absolute_path(strrchr(src->v.path, '/')+1, tmpdir));
    /*source file*/
    chdir(tmpdir);

    if((child_pid=fork())<0)
    {
        test_error(1, "compile: %s", strerror(errno));
    }
    else if(child_pid==0)
    {/* child */
        if(setpgid(0,0)==-1)
        {
            perror("compile");
            exit(3);
        }
        close(0);
        open("/dev/null", O_RDONLY);
        seteuid(nobody_uid);
        execlp("gcc", "gcc", "-g", "-Wall", strrchr(src->v.path, '/')+1, "-o", "a.out", (char*)0);
    }
    else
    {/* parent */
        int statloc;
        if(setpgid(child_pid, child_pid)==-1)
        {
            clean_up();
            finish();
            perror("compile");
            exit(3);
        }
        wait(&statloc);
        if(!WIFEXITED(statloc) || WEXITSTATUS(statloc))
        {
            return EXIT_FAIL;
        }
    }

    chdir(base_dir);
    return EXIT_OK;
}

static void *_run(void *arg)
{
    if((child_pid=fork())<0)
    {
        test_error(1, "run: %s", strerror(errno));
        return NULL;
    }
    else if(child_pid==0)
    {/* child */
        if(setpgid(0,0)==-1)
        {
            perror("run child");
            exit(3);
        }
        close(0);
        open("/dev/null", O_RDONLY);
        setuid(nobody_uid);
        execl("./a.out", "a.out", (char*)0);
    }
    else
    {/* parent */
        int statloc;
        if(setpgid(child_pid, child_pid)==-1)
        {
            clean_up();
            finish();
            perror("run child");
            exit(3);
        }
        wait(&statloc);
        pthread_exit((void*)statloc);
    }
    pthread_exit(NULL);
    return NULL;
}

static user_stat_t diff(FILE *usr, FILE *std)
{
    int a, b;
    for(a=fgetc(usr), b=fgetc(std); a!=EOF && b!=EOF; a=fgetc(usr), b=fgetc(std)) {
        if(a!=b) {
            break;
        }
    }
    if(a!=b) return STAT_WA;
    return STAT_AC;
}

static user_stat_t diff_white(FILE *usr, FILE *std)
{
    int a, b;
    for(a=fgetc(usr),b=fgetc(std); a!=EOF && b!=EOF; a=fgetc(usr), b=fgetc(std)) {
        if(a==b) {
            continue;
        } else if (isspace(a)) {
            ungetc(b, std);
        } else if (isspace(b)) {
            ungetc(a, usr);
        } else {
            break;
        }
    }
    if(a!=b) return STAT_WA;
    return STAT_AC;
}

static user_stat_t diff_eoln(FILE *usr, FILE *std)
{
    int a, b;
    for(a=fgetc(usr), b=fgetc(std); a!=EOF && b!=EOF; a=fgetc(usr), b=fgetc(std)) {
        if(a==b) {
            continue;
        } else if (a=='\n' || a=='\r') {
            ungetc(b, std);
        } else if (b=='\n' || b=='\r') {
            ungetc(a, usr);
        } else {
            break;
        }
    }
    if(a!=b) return STAT_WA;
    return STAT_AC;
}

static user_stat_t
verify(
        int statloc, 
        struct prob_link_t *cprob, 
        struct data_link_t *pin, 
        struct data_link_t *pout
        )
{
    const char *usr, *std;
    if(WIFSIGNALED(statloc) || !WIFEXITED(statloc) || WEXITSTATUS(statloc)!=0) {
        return STAT_RTE;
    }
    usr=absolute_path(strrchr(pout->v.path, '/')+1, tmpdir);
    std=pout->v.path;
    if(cprob->v.jmode==IGN_NOT) {
        return diff(fopen(usr, "r"), fopen(std, "r"));
    } else if (cprob->v.jmode==IGN_WHITE) {
        return diff_white(fopen(usr, "r"), fopen(std, "r"));
    } else if (cprob->v.jmode==IGN_EOLN) {
        return diff_eoln(fopen(usr, "r"), fopen(std, "r"));
    } else if (cprob->v.jmode==SPEC_JUDGE) {
        /*FIXME not implemented.*/
    }
    return STAT_UNKNOWN;
}

static ret_state run(struct srcfile_link_t *src)
{
    struct data_link_t *pin, *pout;
    chdir(tmpdir);
    for(pin=src->v.prob->v.inp_data->next, pout=src->v.prob->v.outp_data->next;
            pin && pout;
            pin=pin->next, pout=pout->next)
    {
        symlink(pin->v.path, absolute_path(src->v.prob->v.inp, tmpdir));

        /***********************************************/

        {
            pthread_t runner_id;
            struct timespec nsintv, actual;
            long long acc;
            int statloc;
            nsintv.tv_sec=0;
            nsintv.tv_nsec=10000000; /* 0.01s */
            pthread_create(&runner_id, NULL, _run, NULL);
            for(acc=0; acc<pin->v.limits.time*1000000000ll; acc+=actual.tv_nsec+actual.tv_sec*1000000000ll)
            {
                nanosleep(&nsintv, &actual);
            }
            if(acc>=pin->v.limits.time*1000000000)
            {
                printf("Time limit exceeded.\n");
                kill(SIGKILL, -(int)child_pid);
            }
            pthread_join(runner_id, (void**)&statloc);

            verify(statloc, src->v.prob, pin, pout);
        }

        /***********************************************/

        unlink(absolute_path(strrchr(pin->v.path, '/')+1, tmpdir));
    }
    chdir(base_dir);
    return EXIT_OK;
}

static ret_state clean_up(void)
{
    DIR *dtmp=opendir(tmpdir);
    struct dirent *entry;
    while((entry=readdir(dtmp)))
    {
        if(!strcmp(entry->d_name, ".")) continue;
        if(!strcmp(entry->d_name, "..")) continue;
        unlink(entry->d_name);
    }
    closedir(dtmp);
    return EXIT_OK;
}

static ret_state run_competitor(struct comp_link_t *entry)
{
    struct srcfile_link_t *q;
    for(q=entry->v.srcfile->next; q; q=q->next)
    {
        if(compile(q)==EXIT_OK)
            run(q);
        else
            puts("Compile failed.");
        clean_up();
    }
    return EXIT_OK;
}

ret_state runtest(void)
{
    struct comp_link_t *q;

    prepare_temp();

    for(q=comp->next; q; q=q->next)
    {
        printf("++++++Processing competitor %s++++++\n", q->v.name);

        run_competitor(q);

        printf("------Finished  competitor  %s------\n", q->v.name);
    }

    if(rmdir(tmpdir)==-1)
    {
        test_error(1, "runtest clean tmpdir: %s", strerror(errno));
    }

    return EXIT_OK;
}
