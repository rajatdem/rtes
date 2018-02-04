#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <asm/unistd.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int prio = 0;
    struct sched_param param;

    if(argc != 3)
    {
        printf("Format error\n");
    }
    pid = atoi(argv[1]);
    prio = atoi(argv[2]);
    if(kill(pid, 0))
    {
        printf("Invalid PID\n");
        return -1;
    }
    param.sched_priority = prio;
    printf("Status: %d\n", sched_setscheduler(pid, 1, &param));
    return 0;
}
