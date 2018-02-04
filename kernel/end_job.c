#include <linux/linkage.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/current.h>

SYSCALL_DEFINE0(end_job)
{
    unsigned long flags;
    if(current->rtes.use_reservation)
    {
        if(current->rtes.scheduled)
        {
            spin_lock_irqsave(&current->rtes.lock, flags);
            current->rtes.c_ktime_rem = hrtimer_get_remaining(&(current->rtes.task_c_timer));
            current->rtes.c_accum_ktime = ktime_sub(current->rtes.c_ktime, current->rtes.c_ktime_rem);
            current->rtes.scheduled = false;
            while(hrtimer_try_to_cancel(&current->rtes.task_c_timer) == -1)
            {
                /* Try until you actually cancel the timer*/
            }
        }
        current->rtes.ctime_up = true;
        spin_unlock_irqrestore(&current->rtes.lock, flags);
        set_tsk_need_resched(current);
        return 0;
    }
    return -1;
}
