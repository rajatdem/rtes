#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/syscall.h>
#include<fcntl.h>
#include<linux/unistd.h>
#include<linux/kernel.h>
#include<signal.h>

#define __NR_count_rt_threads 376
#define __NR_list_rt_threads 377

typedef struct
{
    pid_t tid;
    pid_t pid;
    unsigned int rt_prio;
    char name[20];
}rt_thread_details_t;

void handler(int sig)
{
}
void main()
{
    int fd = 0, i = 0;
    long count = 0;
    rt_thread_details_t *rt_data;
   
    signal(33, handler);

    fd = open("/data/temp", O_RDONLY);
    if(fd == -1)
    {
        printf("Its an error dude\n");
    }
    else
    {
        printf("YAyyyyyyyyyyyyyyyyyyyyyyyy\n");
    }
    //close(fd);
    printf("Checking the new syscall.... Fingers crossed\n");
    count = syscall(__NR_count_rt_threads);
    printf("Number of RT threads = %ld\n", syscall(__NR_count_rt_threads));
    
    rt_data = (rt_thread_details_t *)malloc(sizeof(rt_thread_details_t)*count); 
    if(syscall(__NR_list_rt_threads, rt_data, count) > 0)
    {
        printf("TID\tPID\tRT_PR\tName\n");
        for(i = 0; i < count; i++)
        {
            printf("%d\t%d\t%d\t%s\n",\
                    rt_data[i].tid,\
                    rt_data[i].pid,\
                    rt_data[i].rt_prio,\
                    rt_data[i].name);
        }
    }




    printf("Going into while 1\n");
    unsigned int count_test = 0;
    while(1)
    {
        count_test++;
        if(0 == count_test)
        {
            printf("Calling Endjob\n");
            printf("Endjob return = %d\n", syscall(381));
        }
    }
    exit(0);
}
