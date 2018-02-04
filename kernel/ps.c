#include<linux/linkage.h>
#include<linux/sched.h>
#include<linux/syscalls.h>
#include<linux/errno.h>
#include<linux/string.h>
#include<linux/uaccess.h>
#include<linux/slab.h>

#define GET_COUNT 0

typedef struct
{
    pid_t tid;
    pid_t pid;
    unsigned int rt_prio;
    char name[20];
}rt_thread_details_t;

long find_real_time_threads(rt_thread_details_t *data_buf, int no_of_threads)
{
    struct task_struct *tsk_details1;
    unsigned int count = 0;
    
    rcu_read_lock();
    for_each_process(tsk_details1)
    {
        if((GET_COUNT != no_of_threads) && (count == no_of_threads))
        {
            break;
        }
        if(tsk_details1->rt_priority < MAX_RT_PRIO)
        {
            if(GET_COUNT != no_of_threads)
            {
                data_buf[count].tid = tsk_details1->pid;
                if(tsk_details1->real_parent != NULL)
                {
                    data_buf[count].pid = tsk_details1->real_parent->pid;
                }
                else
                {
                    data_buf[count].pid = tsk_details1->tgid;
                }
                data_buf[count].rt_prio = tsk_details1->rt_priority;
                strcpy(data_buf[count].name, tsk_details1->comm);
            }
            count++; 
        }
    };
    rcu_read_unlock();
    return count;
}

/* returns the number of real time threads */
SYSCALL_DEFINE0(count_rt_threads)
{
    rt_thread_details_t *thread_data = NULL;
    return find_real_time_threads(thread_data, GET_COUNT);
}

/* Returns number of bytes copied 
 * User must call the count_rt_threads first to get the number 
 * of real time threads and allocate memory for those 
 * many threads.
 */
SYSCALL_DEFINE2(list_rt_threads, char __user *, array_of_thread_data, int, count)
{
    int ret = -1;
    rt_thread_details_t *thread_data = (rt_thread_details_t *)\
                                       kmalloc(sizeof(rt_thread_details_t)*count,\
                                               GFP_KERNEL);

    ret = find_real_time_threads(thread_data, count);
    if(ret < 0)
    {
        kfree(thread_data);
        return -1;
    }

    ret = copy_to_user(array_of_thread_data, thread_data, count*sizeof(rt_thread_details_t));
    if(ret)
    {
        kfree(thread_data);
        return -1;
    }

    kfree(thread_data);
    return sizeof(thread_data);
}
