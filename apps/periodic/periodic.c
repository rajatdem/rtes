#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "periodic.h"

#define SIXTY_SECS 60000
#define NO_OF_CPU 4
#define ONE_MS 1000        // in ms (1000 ms = 1 sec)
#define true  1
#define false 0


void handler(int signal)
{
    /* Do nothing */
}

int set_affinity(int cpu_id)
{
    cpu_set_t mask;
    pid_t pid = gettid();

    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    return syscall(__NR_sched_setaffinity, pid, sizeof(cpu_set_t), &mask);
}

int start_engine(int c, int t)
{
    unsigned long c_get_start_time;
    unsigned long c_get_end_time;

    while(1)
    {
        c_get_start_time = ((clock()/CLOCKS_PER_SEC) * ONE_MS);
        c_get_end_time = ((clock()/CLOCKS_PER_SEC) * ONE_MS);
        
        while((c_get_end_time - c_get_start_time) < c)
        {
            c_get_end_time = ((clock()/CLOCKS_PER_SEC) * ONE_MS);
        }
        usleep((t - c)*1000);
    }
    return 0;       /* Should not come here */

}

int main(int argc, char *argv[])
{
    int cpu_id, c, t;
    int check;
    unsigned int cpu_mask;

    if(argc != 4)
    {
        printf("Format - \nperiodic <C (ms)> T (ms) <CPU id>\n");
        return -1;
    }
   
    if(SIG_ERR == signal(33, handler))
    {
        printf("Could not register SIGEXCESS\n");
    }

    c = strtol(argv[1], NULL, 0);
    t = strtol(argv[2], NULL, 0);
    cpu_id = strtol(argv[3], NULL, 0);
    
    if((c > SIXTY_SECS)||(t > SIXTY_SECS))
    {
        printf("Value of c and t should be under %dms\n", SIXTY_SECS);
        return -1;
    }
    if((cpu_id < 0 )||(cpu_id >= NO_OF_CPU))
    {
        printf("CPU id should be between 0 - 3\nYou entered - %d\n", cpu_id);
        return -1;
    }
   
    printf("c = %d, t = %d\n", c, t);
    /* Bind this thread to one CPU */
    check = set_affinity(cpu_id);
    if(check)
    {
        printf("Unable to lock to one CPU ret = %d\n", check);
        return -1;
    }
    else
    {
        printf("Locked to CPU = %d\n", cpu_id);
    }
    
    check = start_engine(c, t);
    return check;
}
