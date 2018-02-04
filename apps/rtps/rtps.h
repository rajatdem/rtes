#ifndef __RTPS_H_
#define __RTPS_H_

#define __NR_SYSCALL_COUNT_RT 376
#define __NR_SYSCALL_LIST_RT  377

typedef struct
{
    pid_t tid;
    pid_t pid;
    unsigned int rt_prio;
    char name[20];
} rt_thread_details_t;

#define count_rt_threads() syscall(__NR_SYSCALL_COUNT_RT)
#define list_rt_threads(a, b) syscall(__NR_SYSCALL_LIST_RT, a, b)

#endif
