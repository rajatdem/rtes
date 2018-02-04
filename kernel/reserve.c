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
#include <linux/cpufreq.h>

#include <asm/unistd.h>
#include <asm/current.h>

#include <linux/rtes.h>

#define FF 0
#define NF 1
#define BF 2
#define WF 3
#define LST 4
#define FAIL -1
#define CPU0 0x01
#define MIN_CPU 0
#define MAX_CPU 3
#define SIZE_STRING10 10
#define SIZE_STRING15 15
#define SIZE_STRING25 25
#define SIZE_STRING50 50
#define ONE_SEC_MS 1000
#define ONE_SEC_NS 1000000
#define MAX_UBOUND 20

static int heuristic = 0;
bool first = true;
bool calc_utilization = false;
static int is_reserve = 0;
static bool task_energy_mon = false;

unsigned long *original_exit;
unsigned long *original_exit_group;
extern void *sys_call_table[];

static const int upper_bound[] = { 1000,    828,    780,    757,    743,\
                                   735,     729,    724,    721,    718,\
                                   715,     713,    711,    710,    709,\
                                   708,     707,    706,    705,    705,\
                                   693 };


util_store_t *start_util = NULL;


util_store_t * get_start_list(void)
{
    return start_util;
}

asmlinkage unsigned long exit_hack(int error_code)
{
    cancel_reservation(current->pid);
    do_exit((error_code&0xff)<<8);
}

int hook_exit(void)
{
    /* Replacing Exit call with our exit */
    original_exit_group = (unsigned long *)sys_call_table[__NR_exit_group];
    original_exit = (unsigned long *)sys_call_table[__NR_exit];
    sys_call_table[__NR_exit_group] = (unsigned long *)exit_hack; 
    sys_call_table[__NR_exit] = (unsigned long *)exit_hack; 
    return 0;
}

enum hrtimer_restart c_task_callback(struct hrtimer *task_c_timer)
{
    struct rtes_acc *rtes;
    struct task_struct *task;
    struct timespec timespec_time;
	int cpu;
    unsigned int freq = 0;
    unsigned long energy = 0;
    unsigned long power = 0;
    unsigned long mask = 0;
 
    
    rtes = container_of(task_c_timer, struct rtes_acc, task_c_timer);
    if(rtes != NULL)
    {
        task = container_of(rtes, struct task_struct, rtes);
        if(task != NULL)
        {
            task->rtes.ctime_up = true;
            task->rtes.scheduled = false;
            task->rtes.c_accum_ktime = task->rtes.c_ktime;
            
            /* Account for the energy utilization for the 
             * last context switch */
            if(task->rtes.mon_energy)
            {
                sched_getaffinity(task->pid, (void *)&mask);
                switch(mask)
                {
                    case 1: cpu = 0;
                            break;
                    case 2: cpu = 1;
                            break;
                    case 4: cpu = 2;
                            break;
                    case 8: cpu = 3;
                            break;
                    default:
                        cpu = -1;
                }
                if(-1 != cpu)
                {
                    freq = cpufreq_get(cpu);
                }
                switch(freq)
                {
                    case FREQ1: power = POWER1;
                                break;
                    case FREQ2: power = POWER2;
                                break;
                    case FREQ3: power = POWER3;
                                break;
                    case FREQ4: power = POWER4;
                                break;
                    case FREQ5: power = POWER5;
                                break;
                    case FREQ6: power = POWER6;
                                break;
                    case FREQ7: power = POWER7;
                                break;
                    case FREQ8: power = POWER8;
                                break;
                    case FREQ9: power = POWER9;
                                break;
                    default: /* Not happening */
                                break;
                }
                timespec_time = ktime_to_timespec(task->rtes.c_ktime_rem);
                energy = power*(timespec_time.tv_sec * 1000 + timespec_time.tv_nsec/1000000);
                //printk("Time: %lu\tenergy = %lu\tpower = %lu\n", (timespec_time.tv_sec * 1000 + timespec_time.tv_nsec/1000000), energy, power);
                task->rtes.energy_accum += energy;
                set_global_energy_count(energy);
            }
            //task->state = TASK_UNINTERRUPTIBLE;
            task->rtes.c_ktime_rem = ktime_set(0,0);
            set_tsk_need_resched(task);
        }
    }
    return HRTIMER_NORESTART;
}

enum hrtimer_restart t_task_callback(struct hrtimer *task_timer)
{	
    struct rtes_acc *rtes;
    struct task_struct *task;
    struct timespec temp;
    struct timespec test; 
    long timestamp; 
    static long count = 0;
    ktime_t ktime;
    util_store_t *i;
    unsigned long flags;
    char *utilization;
    char param1[SIZE_STRING15], param2[SIZE_STRING15], temp_buf[SIZE_STRING25];

    rtes = container_of(task_timer, struct rtes_acc, task_timer);
    if(rtes != NULL)
    {
        task = container_of(rtes, struct task_struct, rtes);
        if(task != NULL)
        {
            if(hrtimer_active(&task->rtes.task_c_timer))
            {
                while(hrtimer_try_to_cancel(&task->rtes.task_c_timer) == -1)
                {
                    /* Make*/
                }
                task->rtes.c_accum_ktime = task->rtes.c_ktime;
            }
            test = ktime_to_timespec(task->rtes.c_accum_ktime);
            rcu_read_lock();
            
//printk("Time: %lu\tenergy = %lu\n", (test.tv_sec * 1000 + test.tv_nsec/1000000), task->rtes.energy_accum);
            if(calc_utilization)
            {
                count++;
                getrawmonotonic(&temp);
                sprintf(param1, "%ld", (long)((test.tv_sec * ONE_SEC_MS) + (test.tv_nsec/ONE_SEC_NS)));
                sprintf(param2, "%ld", (long)((task->rtes.T.tv_sec * ONE_SEC_MS) + (task->rtes.T.tv_nsec/ONE_SEC_NS)));
                utilization = calculator(param1, param2, '/');
                for(i = start_util; i != NULL; i = i->next)
                {
                    if(i->tid == task->pid)
                    {
                        temp = timespec_sub(temp, i->res_start_time);
                        timestamp = (temp.tv_sec * ONE_SEC_MS) + (temp.tv_nsec/ONE_SEC_NS);
                        sprintf(temp_buf, "\n%ld %s", timestamp, utilization);
                        if(i->buff == NULL)
                        {
                            i->buff = (char *)kcalloc(1, SIZE_STRING50, GFP_ATOMIC);
                            strcat(i->buff, temp_buf);
                        }
                        else
                        {
                            i->buff = (char *)krealloc(i->buff, SIZE_STRING50*(count+1), GFP_ATOMIC);
                            strcat(i->buff, temp_buf);
                        }
                        kfree(utilization);
                        break;
                    }
                }
            }
            rcu_read_unlock();
            
            spin_lock_irqsave(&task->rtes.lock, flags);
            task->rtes.ctime_up = false;
            task->rtes.scheduled = false;
            /* Reset the C timer values */
            task->rtes.c_accum_ktime = ktime_set(0, 0);
            task->rtes.c_ktime_rem = task->rtes.c_ktime;
            /* Reset the T timer */
            ktime = ktime_set(task->rtes.T.tv_sec, task->rtes.T.tv_nsec);
            hrtimer_forward_now(task_timer, ktime);
            spin_unlock_irqrestore(&task->rtes.lock, flags);
            
            /* Take care of the C timer and task */
            while(hrtimer_try_to_cancel(&task->rtes.task_c_timer) == FAIL)
            {
                /* Try until you actually cancel the timer*/
            }
            /* Request to reschedule the timer */
            wake_up_process(task);
#if 1 
            /* Send signal to app for marking T timer expiry */
            {
                struct siginfo sig_info;
                sig_info.si_signo = SIGEXCESS;
                sig_info.si_code = SI_KERNEL;
                /* send signal */
                send_sig_info(SIGEXCESS, &sig_info, task);

            }
#endif
            return HRTIMER_RESTART;
        }
    }
    else
    {
        printk("Could not find RTES\n");
    }
    return HRTIMER_NORESTART; 
}


static int get_cpu_heuristic(struct timespec *C, struct timespec *T, pid_t tid, long *util_curr)
{
    int cpu = 0;

    switch(heuristic)
    {
   	    case FF: cpu = first_fit(tid, C, T, util_curr);
		         break;
	    case NF: cpu = next_fit(tid, C, T, util_curr);
		          break;
	    case BF: cpu = best_fit(tid, C, T, util_curr);
		         break;
	    case WF: cpu = worst_fit(tid, C, T, util_curr);
		         break;
	    case LST: cpu = list_sched(tid, C, T, util_curr);
		         break;
        default: cpu = 0;
    } 
    printk("reserve: HEURISTIC = %d CPU = %d\n", heuristic, cpu); 
    return cpu;
}


long calc_ceiling(char *value)
{
    long value_int = 0;
    long decimal = 0;
    
    value = calculator(value, "1000", '*');
    value = strsep(&value, ".");
    if(kstrtol(value, 10, &value_int))
    {
        if(value != NULL)
        {
            kfree(value);
            return -1;
        }
    }
    if(value != NULL)
    {
        kfree(value);
    }

    decimal = value_int % 1000;
    value_int = value_int / 1000;
    if(decimal)
    {
        value_int++;
        return value_int;
    }
    else
    {
        return value_int;
    }
}

static int recursive_rt(long aik0, struct timespec *C, struct timespec *T, pid_t tid, int cpu_id, util_store_t *lowest_task)
{
    long aik1 = 0;
    long T_time = 0;
    long aik_sum = 0;
    long ceil =0;
    int ret = -1;
    util_store_t *task;
    char *div_tmp = NULL;
    char param[SIZE_STRING15] = {0};
    char string_aik0[SIZE_STRING15] = {0};
    char string_ceil[SIZE_STRING15] = {0};
	
    sprintf(string_aik0, "%ld", aik0);
    for(task = start_util; task != NULL; task = task->next)
    {
        if(task->tid != tid && task!=lowest_task && task->cpu == cpu_id)
        {
            
            sprintf(param, "%ld", (long)((task->T.tv_sec * ONE_SEC_MS) + (task->T.tv_nsec/ONE_SEC_NS)));
            div_tmp = calculator(string_aik0, param, '/');
            if((ceil = calc_ceiling(div_tmp)) == -1)
            {
                return -1;
            }
            sprintf(string_ceil, "%ld", ceil);
            sprintf(param, "%ld", (long)((task->C.tv_sec * ONE_SEC_MS) + (task->C.tv_nsec/ONE_SEC_NS)));
            
            /* Avoiding stupid behaviour */
            kfree(div_tmp);
            div_tmp = NULL;

            div_tmp = calculator(string_ceil, param, '*');
            div_tmp = strsep(&div_tmp, ".");
            if(kstrtol(div_tmp, 10, &aik1))
            {
                printk("Kstrtol failed\n");
                return -1;
            }
            kfree(div_tmp); //added to free div_tmp 
            div_tmp = NULL;
	        aik_sum += aik1;
        }
    }
	
   if(lowest_task!=NULL)
   {
       //add ceil[aik/T]*C for new task
            sprintf(param, "%ld", (long)((T->tv_sec * ONE_SEC_MS) + (T->tv_nsec/ONE_SEC_NS)));
            div_tmp = calculator(string_aik0, param, '/');
            if((ceil = calc_ceiling(div_tmp)) == -1)
            {
                return -1;
            }
            sprintf(string_ceil, "%ld", ceil);
            sprintf(param, "%ld", (long)((C->tv_sec * ONE_SEC_MS) + (C->tv_nsec/ONE_SEC_NS)));
            
            /* Avoiding stupid behaviour */
            kfree(div_tmp);
            div_tmp = NULL;

            div_tmp = calculator(string_ceil, param, '*');
            div_tmp = strsep(&div_tmp, ".");
            if(kstrtol(div_tmp, 10, &aik1))
            {
                printk("Kstrtol failed\n");
                return -1;
            }
            kfree(div_tmp); //added to free div_tmp 
            div_tmp = NULL;
	        aik_sum += aik1;
   	        aik_sum += (long)((lowest_task->C.tv_sec * ONE_SEC_MS) + (lowest_task->C.tv_nsec/ONE_SEC_NS));
   	        T_time = (long)((lowest_task->T.tv_sec * ONE_SEC_MS) + (lowest_task->T.tv_nsec/ONE_SEC_NS));
   }
   else
   {
       //new task has highest T
	    aik_sum += (long)((C->tv_sec * ONE_SEC_MS) + (C->tv_nsec/ONE_SEC_NS));
   	    T_time = (long)((T->tv_sec * ONE_SEC_MS) + (T->tv_nsec/ONE_SEC_NS));
   } 
    if(aik_sum > T_time)
    {
        printk("Task not schedule\n");
        ret = -1;
    }
    else if(aik_sum == aik0)
    {
        printk("reserve: Task Schedulable\n");
        ret = 0;
    }
    else
    {
        return recursive_rt(aik_sum, C, T, tid, cpu_id, lowest_task);
    }
    return ret;
}

static int try_rt_test(struct timespec *C, struct timespec *T, pid_t tid, int cpu_id, util_store_t *lowest_task)
{
    long ai0 = 0;
    util_store_t *task;
    
    for(task = start_util; task != NULL; task = task->next)
    {
        if(tid != task->tid && cpu_id == task->cpu)
        {
            ai0 += (long)((task->C.tv_sec * ONE_SEC_MS) + (task->C.tv_nsec/ONE_SEC_NS)); 
        }
    }
    ai0 += (long)((C->tv_sec * ONE_SEC_MS) + (C->tv_nsec/ONE_SEC_NS));

    return recursive_rt(ai0, C, T, tid, cpu_id, lowest_task);
    
}


int check_schedulability(pid_t tid, struct timespec *C, struct timespec *T, int cpu, long *util_curr)
{
    char *utilization = NULL;
    char param_C[SIZE_STRING15] = {0};
    char param_T[SIZE_STRING15] = {0};
    long cummul_util = 0;
    int count_task = 0;
    util_store_t *task = NULL;
    util_store_t *lowest_task = NULL;
    long T_max = 0;
	long tmp = 0;

    /* Latest Utilization of requested task */
    sprintf(param_C, "%ld", (long)((C->tv_sec * ONE_SEC_MS) + (C->tv_nsec/ONE_SEC_NS)));
    sprintf(param_T, "%ld", (long)((T->tv_sec * ONE_SEC_MS) + (T->tv_nsec/ONE_SEC_NS)));
    utilization = calculator(param_C, param_T, '/');
    sprintf(param_T, "%d", 1000);
    utilization = calculator(utilization, param_T, '*');
    utilization = strsep(&utilization, ".");

    if((kstrtol(utilization, 10, util_curr)) != 0)
    {
        //FIXME: Change return value
        if(utilization != NULL)
        {
            kfree(utilization);
        }
        return -EINVAL;
    }
    
    if(utilization != NULL)
    {
        kfree(utilization);
    }
    

    if(*util_curr > 1000)
    {
        return -EBUSY;
    }

    for(task = start_util; task != NULL; task = task->next)
    {
        if((task->tid != tid) && (task->cpu == cpu))
        {
            cummul_util += task->util;
            count_task++;
        }
    }
    
    cummul_util += *util_curr;
    count_task++;

    if(count_task > MAX_UBOUND)
    {
        if(cummul_util <= upper_bound[MAX_UBOUND+1])
        {
            //SCHEDULABLE
            return 0;
        } 
        else
        {  //get task with the highest T
           for(task = start_util; task!= NULL; task = task->next)
           {
            if(tid != task->tid && cpu == task->cpu)
            {
                tmp = (long)((task->T.tv_sec * ONE_SEC_MS) + (task->T.tv_nsec/ONE_SEC_NS));
              if(tmp > T_max)
              {
                 T_max = tmp;
                 lowest_task = task;            
                }   
              } 
            } 
           if((long)((lowest_task->T.tv_sec * ONE_SEC_MS) + (lowest_task->T.tv_nsec/ONE_SEC_NS)) < \
              (long)((T->tv_sec * ONE_SEC_MS) + (T->tv_nsec/ONE_SEC_NS)))
           {
                lowest_task = NULL;
           }
           return try_rt_test(C, T, tid, cpu, lowest_task);
        }
    } 
    else
    {
        if(cummul_util <= upper_bound[count_task-1])
        {
            //SCHEDULABLE
            return 0;
        } 
        else
        {  
		    //get task with the highest T
            for(task = start_util; task!= NULL; task = task->next)
            {
               if(tid != task->tid && cpu == task->cpu)
               {
                tmp = (long)((task->T.tv_sec * ONE_SEC_MS) + (task->T.tv_nsec/ONE_SEC_NS));
              if(tmp > T_max)
              {
                 T_max = tmp;
                 lowest_task = task;            
                }   
              } 
            } 
           if((long)((lowest_task->T.tv_sec * ONE_SEC_MS) + (lowest_task->T.tv_nsec/ONE_SEC_NS)) < \
              (long)((T->tv_sec * ONE_SEC_MS) + (T->tv_nsec/ONE_SEC_NS)))
           {
                lowest_task = NULL;
           }
           return try_rt_test(C, T, tid, cpu, lowest_task);
        }
    }
}

long cancel_reservation(pid_t tid)
{
    unsigned long mask = 1;
    unsigned long flags;
    //struct task_struct *th1;
    //struct task_struct *th2;
    struct task_struct *task = NULL;
    bool no_reservation = false;
    util_store_t *i;
    int cpu_id = 0;
    long util_curr = 0;
    int err = 0;

    if(tid == 0)
    {
        tid = current->pid;
    }
    
    rcu_read_lock();
#if 0
    do_each_thread(th1, th2)
    {
        if(th2->pid == tid)
        {
            if(th2->rtes.use_reservation)
            {
                th2->rtes.use_reservation = false;
	            task = th2;
            }
            else
            {
                no_reservation = true;
            }
	        goto break_this;
        }
    
    }while_each_thread(th1, th2);
break_this:
#endif
    task = find_task_by_vpid(tid);
    rcu_read_unlock();

    if(task->rtes.use_reservation)
    {
        spin_lock_irqsave(&task->rtes.lock, flags);
        task->rtes.use_reservation = false;
        spin_unlock_irqrestore(&task->rtes.lock, flags);
    }
    else
    {
        no_reservation = true;
    }

    if(!no_reservation)
    {
        while(hrtimer_try_to_cancel(&task->rtes.task_c_timer) == FAIL)
        {
            /* Try until you actually cancel the timer*/
        }
        while(hrtimer_try_to_cancel(&task->rtes.task_timer) == FAIL)
        {
            /* Try until you actually cancel the timer*/
        }
    }
   
    if(!no_reservation)
    {
        printk(" Cancel reserve: Pid  = %d\n", tid);
        is_reserve--;

        /* remove util data */
        for(i = start_util; i != NULL; i = i->next)
        {
            if(i->tid == tid)
            {
                printk("Migrating %d to CPU 0\n", (int)tid);

                mask = mask << 0;
                if((err = sched_setaffinity((pid_t)tid, (void *)&mask)) != 0)
                {
                    printk("Could not migrate task to CPU %d -- err = %d\n", 0, err);
                    //return -EPERM;
                }

                wake_up_process(task);
                printk("Done\n");
                
                util_curr = calc_util(&i->C, &i->T);
                remove_cpu_data(i->cpu, util_curr);
                cpu_id = i->cpu;
                
                if(i->prev==NULL && i->next==NULL)
                {
                    start_util = NULL;
                }
                else if(i->prev!=NULL && i->next==NULL)
                {
                    i->prev->next=NULL;
                }
                else if(i->prev==NULL && i->next!=NULL)
                {
                    i->next->prev=NULL;
                    start_util = i->next;
                }
                else
                {
                    i->prev->next = i->next;
                    i->next->prev = i->prev;
                }

                /* Deleting SYSFS nodes */
                delete_node(i->file_obj);
                delete_energy_node(i->kobj, i->tasks_obj);
                
                if(i->tasks_obj != NULL)
                {
                    kfree(i->tasks_obj);
                }
                if(i->buff != NULL)
                {
                    kfree(i->buff);
                }
                
                if(i->file_obj != NULL)
                {
                    kfree(i->file_obj);
                }
                kfree(i);
                break;
            }
        }
        
        printk("Checking if CPU is on %d\n", cpu_id);
        /* Perform sysclock frequency scaling */
        if(get_sysclock_governor(cpu_id))
        {
            printk("Calling governor in cancel\n");
            /* perform frequency scaling */
            calc_sysclock(is_reserve, cpu_id);
            printk("Frequency tuning done\n");
        }
    }
    return 0;
}

int set_reservation(pid_t tid, struct timespec *C, struct timespec *T, int cpuid)
{
    unsigned long mask = CPU0, flags;
    int err;
    long util_curr = 0;
    bool do_not_init = false;
    bool is_heuristic = false;
    util_store_t *util_data_store, *i;
    ktime_t ktime;
    struct kobj_attribute *vfile_obj;
    struct kobj_attribute *tasks_obj;
    struct kobject *kobj = NULL;
    char tid_para[SIZE_STRING10];
    //struct timespec C_user, T_user;
    //struct task_struct *th1 = NULL, *th2 = NULL;
    struct task_struct *task = NULL;
    
    if(first)
    {
        hook_exit();
        first = false;
    }
    if(tid == 0)
    {
        tid = current->pid;
    }

    /* Check CPU ID range */

    if(cpuid == -1)
    {
    	cpuid = get_cpu_heuristic(C, T, tid, &util_curr);
        is_heuristic = true;
    }

    if(cpuid == -2)
    {
        return additional_bonus(tid, C, T);
    }



    if(cpuid < MIN_CPU || cpuid > MAX_CPU)
    {
        return -EINVAL;
    }
    
    /* Identify if the cpu is online */
    if(!cpu_online(cpuid))
    {
        cpu_hotplug_driver_lock();
        printk("Turning on cpu %d\n", (int)cpuid);
        cpu_up(cpuid);
        cpu_hotplug_driver_unlock();
    }
    
    mask = mask << cpuid;
    if((err = sched_setaffinity((pid_t)tid, (void *)&mask)) != 0)
    {
        printk("CPU %d\n", cpuid);
        return -EPERM;
    }

    /* Store C and T values to the corresponding task structure */
    rcu_read_lock();
    task = find_task_by_vpid(tid);
    rcu_read_unlock();
    
    if(task != NULL)
    {
        if(!is_heuristic)
        {
            if(check_schedulability(tid, C, T, cpuid, &util_curr))
            {
                printk("Task not schedulable\n");
                return -EBUSY;
            }            
        }

        spin_lock_irqsave(&task->rtes.lock, flags);
        if(task->rtes.use_reservation)
        {
            do_not_init = true;
            /*Cancel the current timer*/
            while(hrtimer_try_to_cancel(&task->rtes.task_c_timer) == FAIL)
            {
                /* Try until you actually cancel the timer*/
            }
            while(hrtimer_try_to_cancel(&task->rtes.task_timer) == FAIL)
            {
                /* Try until you actually cancel the timer*/
            }
        }
        else
        {
            /* Create timer */
            do_not_init = false;
            hrtimer_init(&(task->rtes.task_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED); 
            hrtimer_init(&(task->rtes.task_c_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED); 
        }

        task->rtes.use_reservation = true;
        task->rtes.C = *C; 
        task->rtes.T = *T; 
        task->rtes.task_timer.function = t_task_callback;
        task->rtes.task_c_timer.function = c_task_callback;
        task->rtes.scheduled = false;
        task->rtes.ctime_up = false;
        
        task->rtes.c_ktime_rem = ktime_set(task->rtes.C.tv_sec, task->rtes.C.tv_nsec);
        task->rtes.c_ktime = task->rtes.c_ktime_rem;
        task->rtes.c_accum_ktime = ktime_set(0, 0);
        if(task_energy_mon)
        {
            task->rtes.mon_energy = true;
            task->rtes.energy_accum = 0;
        }
        else
        {
            task->rtes.mon_energy = false;
        }
        ktime = ktime_set(task->rtes.T.tv_sec, task->rtes.T.tv_nsec);
        hrtimer_start(&(task->rtes.task_timer), ktime, HRTIMER_MODE_REL_PINNED);
        spin_unlock_irqrestore(&task->rtes.lock, flags);

        /* Create a sysfs node for this thread */
        sprintf(tid_para, "%d", (int)tid);
        if(!do_not_init)
        {
            vfile_obj = create_node(tid_para);
            tasks_obj = create_energy_node(tid_para, &kobj);
            util_data_store = (util_store_t *)kmalloc(sizeof(util_store_t), GFP_ATOMIC);
            util_data_store->file_obj = vfile_obj;
            util_data_store->tasks_obj = tasks_obj;
            util_data_store->kobj = kobj;
            util_data_store->tid = tid;
            util_data_store->C = task->rtes.C;
            util_data_store->T = task->rtes.T;
            util_data_store->cpu = cpuid;
	        util_data_store->util = util_curr;
            
            getrawmonotonic(&util_data_store->res_start_time);
           
            util_data_store->prev = NULL;
            util_data_store->buff = NULL;
            util_data_store->next = start_util;
            if(start_util != NULL)
            {
                start_util->prev = util_data_store;
            }
            start_util = util_data_store;
        }
        else
        {
            for(i = start_util; i!=NULL; i=i->next)
            {
                if(i->tid == tid)
                {  
                    is_reserve--;
                    remove_cpu_data(i->cpu, i->util);
                    util_data_store->C = *C;
                    util_data_store->T = *T;
                    util_data_store = i;
                    util_data_store->util = util_curr;
                    util_data_store->cpu = cpuid;
                    getrawmonotonic(&util_data_store->res_start_time);
                    break;
                }
            }
        }
        is_reserve++;
        set_cpu_data(cpuid, util_curr);

        /* Perform sysclock frequency scaling */
        if(get_sysclock_governor(cpuid))
        {
            /* perform frequency scaling */
            calc_sysclock(is_reserve, cpuid);
        }
    }
    return (cpuid+100);
}


SYSCALL_DEFINE1(cancel_reserve, pid_t, tid)
{
    return cancel_reservation(tid);
}

SYSCALL_DEFINE4(set_reserve, pid_t, tid, struct timespec __user *, C, struct timespec __user *, T, int, cpuid)
{
    struct timespec C_user, T_user;
    if(copy_from_user(&C_user, C, sizeof(struct timespec)))
    {
        printk("Could not copy from user\n");
        return -EFAULT;
    }
    if(copy_from_user(&T_user, T, sizeof(struct timespec)))
    {
        printk("Could not copy from user\n");
        return -EFAULT;
    }
    return set_reservation(tid, &C_user, &T_user, cpuid);
}

void disable_reservation_monitor(void)
{
    calc_utilization = false;    
}

void enable_reservation_monitor(void)
{
    calc_utilization = true;
}

int set_heuristic(int select)
{
    //is_reserve is count of total reservations on the framework currently
    if(!is_reserve)
    {
        heuristic = select;
        heuristic_init_local();
        return 0;
    }
    else
    {   //EBUSY case
        return -1;
    }
}

void set_energy_mon(bool status)
{
    task_energy_mon = status;
}
