#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>
#define LINE_BUFFER_LENGTH 512
#define MAX_PROBLEMS 20
#define MAX_CASES 50

struct problem_t
{
    char filename[NAME_MAX], checker[LINE_BUFFER_LENGTH];
    short comp_type;
    char infile[NAME_MAX], outfile[NAME_MAX];
    struct case_t
    {
        char infile[LINE_BUFFER_LENGTH], outfile[LINE_BUFFER_LENGTH];
        float tl, ml, sc;
    }cases[MAX_CASES];
    short tot_cases;
}problems[MAX_PROBLEMS];
char tmpbuf[LINE_BUFFER_LENGTH];
short tot_problems;

char *get_from_line(char *cur, char *target)
{
    char *next;
    if(!cur)
    {
        printf("Unexpected end of line.\n");
        exit(1);
    }
    if(!target)
    {
        printf("get_from_line: target does not exist.\n");
        exit(1);
    }
    next=strchr(cur, ';');
    if(!next) return NULL;
    strncpy(target, cur, next-cur);
    return next+1;
}

FILE *parse_conf(FILE *dataconf)
{
    char linebuf[LINE_BUFFER_LENGTH];
    char *cur=linebuf;
    short *cur_problems=&tot_problems, *cur_cases=NULL;
    char *cwd=getcwd(NULL, 0);
    chdir("data");
    while(!feof(dataconf))
    {
        char *next;
        fgets(linebuf, LINE_BUFFER_LENGTH, dataconf);
        next=strchr(cur, ';');
        if(!next) continue;

        if(!strncmp(cur, "problem", next-cur))
        {
            if(*cur_problems>=MAX_PROBLEMS)
            {
                printf("More than %d problems found.\n", MAX_PROBLEMS);
                exit(1);
            }
            cur=next+1;
            cur=get_from_line(cur, problems[*cur_problems].filename);
            cur=get_from_line(cur, tmpbuf);
            sscanf(tmpbuf, "%hd", &problems[*cur_problems].comp_type);
            cur=get_from_line(cur, problems[*cur_problems].infile);
            cur=get_from_line(cur, problems[*cur_problems].outfile);

            (*cur_problems)++;
            cur_cases=&problems[*cur_problems].tot_cases;
        }
        else if(!strncmp(cur, "case", next-cur))
        {
            if(*cur_cases>=MAX_CASES)
            {
                printf("More than %d cases found.\n", MAX_CASES);
                exit(1);
            }
            cur=next+1;
            cur=get_from_line(cur, problems[*cur_problems].cases[*cur_cases].infile);
            cur=get_from_line(cur, problems[*cur_problems].cases[*cur_cases].outfile);

            if(access(problems[*cur_problems].cases[*cur_cases].infile, R_OK)
                    || access(problems[*cur_problems].cases[*cur_cases].outfile, R_OK))
            {
                printf("One or more of the input/output files are inaccessible.\n");
                exit(1);
            }

            cur=get_from_line(cur, tmpbuf);
            sscanf(tmpbuf, "%f", &problems[*cur_problems].cases[*cur_cases].tl);
            cur=get_from_line(cur, tmpbuf);
            sscanf(tmpbuf, "%f", &problems[*cur_problems].cases[*cur_cases].ml);
            cur=get_from_line(cur, tmpbuf);
            sscanf(tmpbuf, "%f", &problems[*cur_problems].cases[*cur_cases].sc);
            (*cur_cases)++;
        }
    }
    chdir(cwd);
    free(cwd);
    return dataconf;
}

void copy_file(const char *dest, const char *src)
{
    FILE *fdest=fopen(dest, "w");
    FILE *fsrc=fopen(src, "r");
    while(!feof(fsrc))
        fputc(fgetc(fsrc), fdest);
    fclose(fdest);
    fclose(fsrc);
}

void symlink_to_infile(int i, int j, const char *cwd,
        char *dataOriginal)
{
    strcpy(dataOriginal, cwd);
    strcat(strcat(dataOriginal, "/data/"),
            problems[i].cases[j].infile);
    strcpy(tmpbuf, "/tmp/linearity/");
    strcat(tmpbuf, problems[i].infile);
    symlink(dataOriginal, tmpbuf);
}

int getc_custom(FILE **src)
{
    int tmp=fgetc(*src);
    while(isspace(tmp))
        tmp=fgetc(*src);
    return tmp;
}

short diff(short comp_type, const char *a, const char *b)
{
    FILE *fa=fopen(a,"r");
    FILE *fb=fopen(b,"r");
    if(comp_type == 1)
    {
        while(!feof(fa) && !feof(fb))
        {
            if(getc_custom(&fa) != getc_custom(&fb))
            {
                fclose(fa); fclose(fb);
                return 1;
            }
        }
    }
    else if(comp_type == 0)
    {
        while(!feof(fa) && !feof(fb))
        {
            if(fgetc(fa) != fgetc(fb))
            {
                fclose(fa); fclose(fb);
                return 1;
            }
        }
    }
    else
    {//custom checker.
    }
    fclose(fa); fclose(fb);
    return 0;
}

void start_judge(char *src)
{
    int i=0, j=0;
    static char srcOriginal[PATH_MAX];
    static char dataOriginal[PATH_MAX];
    static char stdOriginal[PATH_MAX];
    static struct stat oldstat;
    int child_process_stat;
    short right=0;
    char *cwd=getcwd(NULL, 0);

    chdir("/tmp/linear");
    for(i=0; i<tot_problems; i++)
    {
        short type_c=0, type_cpp=0, type_pas=0;
        int user_program_pid, guard_process_pid;

        printf("Problem %s:\n", problems[i].filename);
        srcOriginal[0]=0;
        strcat(strcat(strcat(strcat(srcOriginal, cwd), "/src/"), src), problems[i].filename);

        if(access(strcat(srcOriginal, ".c"), R_OK))
            *strrchr(srcOriginal, '.')=0;
        else
        {
            type_c=1;
            copy_file("/tmp/linear/a.c", srcOriginal);
        }

        if(!type_c && access(strcat(srcOriginal, ".cpp"), R_OK))
            *strrchr(srcOriginal, '.')=0;
        else
        {
            type_cpp=1;
            copy_file("/tmp/linear/a.cpp", srcOriginal);
        }

        if(!type_c && !type_cpp &&
                access(strcat(srcOriginal, ".pas"), R_OK))
            *strrchr(srcOriginal, '.')=0;
        else
        {
            type_pas=1;
            copy_file("/tmp/linear/a.pas", srcOriginal);
        }

        //compile & judge
        printf("Compiling %s...\n", problems[i].filename);
        if(fork())
            wait(&child_process_stat);
        else
        {
            if(type_c)
                execlp("gcc", "gcc", "a.c", NULL);
            else if(type_cpp)
                execlp("g++", "g++", "a.cpp", NULL);
            else if(type_pas)
                execlp("fpc", "fpc", "a.pas", "-oa.out", NULL);
        }
        if(child_process_stat)
            printf("Compile error.\n");
        else
        {//compile success.
            for(j=0; j<problems[i].tot_cases; j++)
            {
                symlink_to_infile(i, j, cwd, dataOriginal);
                stat(dataOriginal, &oldstat);
                chmod(dataOriginal, S_IRUSR | S_IRGRP | S_IROTH);

                user_program_pid=fork();
                if(user_program_pid)
                {//parent process
                    guard_process_pid=fork();
                    if(guard_process_pid)
                    {//parent process again
                        //GET TIME HERE.
                        int return_pid=wait(&child_process_stat);
                        //GET TIME HERE AGAIN.
                        if(return_pid == user_program_pid)
                            kill(guard_process_pid, SIGTERM);
                        else
                            kill(user_program_pid, SIGTERM);
                        if(return_pid == user_program_pid && !child_process_stat)
                        {
                            //PRINT TIME HERE.
                            strcpy(tmpbuf, "/tmp/linear/");
                            strcat(tmpbuf, problems[i].outfile);
                            strcpy(stdOriginal, cwd);
                            strcat(stdOriginal, problems[i].cases[j].outfile);
                            if(diff(tmpbuf, stdOriginal))
                                printf("Wrong answer.\n");
                            else
                            {
                                printf("Accepted.\n");
                                right=1;
                            }
                        }
                        else if(return_pid == user_program_pid)
                            printf("Runtime error.\n");
                        else if(return_pid == guard_process_pid)
                            printf("Time limit exceeded.\n");
                    }
                    else
                    {//guard process
                        nanosleep(problems[i].cases[j].tl);
                    }
                }
                else
                {//user program
                    execl("a.out","a.out");
                    printf("Fatal: unable to excute user program.\n");
                    exit(1);
                }

                chmod(dataOriginal, oldstat.st_mode);
                problem_score +=
                    right * problems[i].cases[j].sc;
            }//for cases
        }//compile success
    }//for problems
    chdir(cwd);
    free(cwd);
}

int main(int argc, char **argv)
{
    FILE *dataconf=NULL;
    DIR *srcdir=NULL;
    struct dirent *srcdirent=NULL;
    if(argc<=1)
    {
        printf("Missing arguments.\n");
        return 1;
    }

    if(access("data", R_OK | X_OK) || access("src", R_OK | X_OK))
    {
        printf("Directory data or src inaccessible.\n");
        return 1;
    }

    dataconf=fopen(argv[1], "r");
    if(!dataconf)
    {
        printf("Specified file %s inaccessible.\n", argv[1]);
        return 1;
    }
    dataconf=parse_conf(dataconf);
    fclose(dataconf);

    if(mkdir("/tmp/linearity", S_IRWXU | S_IRWXG | S_IRWXO))
    {
        printf("/tmp/linearity cannot be created, which is not supposed to happen.\n");
        return 1;
    }

    srcdir=opendir("src");
    do
    {
        srcdirent=readdir(srcdir);
        if(!srcdirent) break;
        if(!strcmp(srcdirent->d_name, ".") ||
            !strcmp(srcdirent->d_name, "..") ||
            srcdirent->d_type != DT_DIR) continue;
        printf("Judging %s...\n", srcdirent->d_name);
        start_judge(srcdirent->d_name);
    }while(srcdirent);

    closedir(srcdir);
    return 0;
}
