#include <linux/linkage.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cpumask.h>
#include <linux/hrtimer.h>
#include <linux/signal.h>
#include <linux/mutex.h>

#include <asm/unistd.h>
#include <asm/current.h>

SYSCALL_DEFINE2(set_rtprio, pid_t, pid, int, prio)
{
    struct sched_param param;
    struct task_struct *task  = NULL;
    struct task_struct *a = NULL, *b = NULL;
    
    rcu_read_lock();
    do_each_thread(a, b)
    {
        if(b->pid == pid)
        {
            task = b;
            goto found;
        }
    }while_each_thread(a, b);

found:
    rcu_read_unlock();
    {
        param.sched_priority = prio;
        if(sched_setscheduler(task, SCHED_FIFO, &param))
        {
            printk("Unable to set SCHED_FIFO\n");
            return -1;
        }
    }
    return 0; 
}
