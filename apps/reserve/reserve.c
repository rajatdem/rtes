#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "reserve.h"

/* Signal handlers */
void sigexcess_handler(int sig)
{
    printf("Caught signal: SIGEXCESS\n");
}

void format_error()
{
    printf("Command format error -\n");
    printf("reserve <set/cancel> <pid> <C time> <T time> <cpuid>\n");
    exit(0);
}

/* Main function */
int main(int argc, char *argv[])
{   
    struct timespec C;
    struct timespec T;
    long c, t;
    unsigned long c_get_start_time;
    unsigned long c_get_end_time;
    pid_t tid;
    int set_command, cancel_command;
    int cpuid = 0;
    int ret = 0;

    if(SIG_ERR == signal(SIGEXCESS, sigexcess_handler))
    {
        printf("Could not register a signal handler\n");
    }

    /* Collect arguments */
    if(strcmp(argv[1], "set") == 0)
    {
        set_command = true;
        if(argc != 6)
        {
            format_error();
        }
    }
    else if(strcmp(argv[1], "cancel") == 0)
    {
        cancel_command = true;
        if(argc != 3)
        {
            format_error();
        }
    }

    tid = atoi(argv[2]);
    
    if(set_command)
    {
        c = strtol(argv[3], NULL, 0);
        C.tv_sec = (time_t)(c / ONE_SEC);
        C.tv_nsec = (long)((c % ONE_SEC) * ONE_SEC_NS);
        printf("C: sec = %d nsec = %d\n", C.tv_sec, C.tv_nsec);

        t = strtol(argv[4], NULL, 0);
        T.tv_sec = (time_t)(t / ONE_SEC);
        T.tv_nsec = (long)((t % ONE_SEC) * ONE_SEC_NS);
        printf("T: sec = %d nsec = %d\n", T.tv_sec, T.tv_nsec);
        
        cpuid = strtol(argv[5], NULL, 0);
        ret = set_reserve(tid, &C, &T, (int)cpuid);
        if(ret < 100)
        {
           printf("Unable to set reservation: %d\n", ret); 
        }
        else
        {
            printf("CPUID = %d\n", (ret-100));
        }
    }
    else if(cancel_command)
    {
        ret = cancel_reserve(tid);
        if(ret)
        {
           printf("Unable cancel reservation\n"); 
        }
    }
    else
    {
        format_error();
    }
    return 0;
}
