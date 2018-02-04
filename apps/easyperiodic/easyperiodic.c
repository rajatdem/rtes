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
#include "easyperiodic.h"

#define SIXTY_SECS 60000
#define NO_OF_CPU 4
#define ONE_SEC 1000        // in ms (1000 ms = 1 sec)
#define ONE_SEC_NS 1000000
#define true  1
#define false 0

typedef int bool;

static struct itimerval c_timer;
static struct itimerval t_timer;



void c_timer_callback(int signal)
{
    /* Suspend task until next period */
    syscall(381);
}

void t_timer_callback(int signal)
{
    /* Restarting C timer in VIRTUAL mode */
    if(setitimer(ITIMER_VIRTUAL, &c_timer, NULL))
    {
        printf("Unable to restart C timer.");
    }
}

int set_affinity(int cpu_id)
{
    cpu_set_t mask;
    pid_t pid = gettid();

    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    return syscall(__NR_sched_setaffinity, pid, sizeof(cpu_set_t), &mask);
}

int start_engine(int c, int t, int cpu_id)
{
	struct timespec C;
	struct timespec T;
    int ret = 0;

    /* Register T timer callback */
    if(SIG_ERR == signal(33, t_timer_callback))
    {
        printf("Unable to register timer callback: Exiting\n");
        return -1;
    }
    
    /* Register C timer callback */
    if(SIG_ERR == signal(SIGVTALRM, c_timer_callback))
    {
        printf("Unable to register timer callback: Exiting\n");
        return -1;
    }
    c_timer.it_value.tv_sec = (time_t)(c / ONE_SEC);
    c_timer.it_value.tv_usec = (long)(((c * 1000) % 1000000) - 10000);

    c_timer.it_interval.tv_sec = 0;
    c_timer.it_interval.tv_usec = 0;


    if(setitimer(ITIMER_VIRTUAL, &c_timer, NULL))
    {
        printf("Unable to start C timer: Exiting\n");
        return -1;
    }
    
    /* Call Set reserve */
    C.tv_sec = (time_t)(t / ONE_SEC);
    C.tv_nsec = (long)((t % ONE_SEC) * ONE_SEC_NS);
    
    T.tv_sec = (time_t)(t / ONE_SEC);
    T.tv_nsec = (long)((t % ONE_SEC) * ONE_SEC_NS);
    
    ret = syscall(379, (pid_t)gettid(), &C, &T, cpu_id);
    if(ret)
    {
       printf("Reservation: %d\n", ret);
    }

    while(1);
    return 0;       /* Should not come here */

}

int main(int argc, char *argv[])
{
    int cpu_id, c, t;
    int check;
    unsigned int cpu_mask;

    if(argc != 4)
    {
        printf("Format - \neasyperiodic <C (ms)> T (ms) <CPU id>\n");
        return -1;
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
    
    check = start_engine(c, t, cpu_id);
    return check;
}
