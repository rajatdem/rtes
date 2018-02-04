/*
 *  Copyright (C) 2016 Rajat Mathur
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/rtes.h>

#define ONE_SEC_MS 1000            
#define ONE_SEC_NS 1000000
#define MAX_CPU 4
#define SYSTEM_MAX_FREQ 1300000
#define SYSTEM_MIN_FREQ 340000

static bool is_sysclock_governor[4] = {false, false, false, false}; 
static struct cpufreq_policy *curr_policy[4];
static int apply_freq = SYSTEM_MAX_FREQ;
static int task_total = 0;
static int calc_sysclock_computation(util_store_t *start_list, int cpu_id, int task_on_cpu);

bool get_sysclock_governor(int cpu)
{
    return is_sysclock_governor[cpu];
}


static int calc_workload(int count, int Tj, long *C, long *T)
{
    int i, sum = 0;
    for(i = 0; i <= count; i++)
    {
        sum += (C[i]*10000)*(Tj/T[i] + ((Tj%T[i])?1:0));
    }
    return sum/Tj;
}

static int sysclock_per_cpu(util_store_t *start_list, int cpu_id)
{
    util_store_t *start;
    int max_freq = SYSTEM_MIN_FREQ;
    int task_on_cpu = 0;

    /* Find count of tasks on this CPU */
    for(start = start_list; start != NULL; start = start->next)
    {
        if(start->cpu == cpu_id)
        {
            task_on_cpu++;
        }
    }

    if(start_list != NULL)
    {
        max_freq = calc_sysclock_computation(start_list, cpu_id, task_on_cpu);
    }
    printk("SYSCLOCK: Attempt to Setup CORE: %d to freq: %d\n", cpu_id, max_freq);

    apply_freq = max_freq;
    return apply_freq;
}

static int calc_sysclock_computation(util_store_t *start_list, int cpu_id, int task_on_cpu)
{
    util_store_t *i;
    int tmp = 0;
    int j, k, l, Tl = 0, eta = 0, max_eta = 0;
    int count = 0;
    int *task_min = (int *) kmalloc(task_on_cpu*sizeof(int), GFP_ATOMIC);
    long *T = (long *) kmalloc(task_on_cpu*sizeof(long), GFP_ATOMIC);
    long *C = (long *) kmalloc(task_on_cpu*sizeof(long), GFP_ATOMIC);
  
    long *free_t = NULL, *free_c = NULL;
    int *free_task = NULL;

    free_t = T;
    free_c = C;
    free_task = task_min;

    printk("SYSCLOCK: Calculating sys clock for cpu = %d\n", cpu_id);

    if(task_on_cpu == 0)
    {
        apply_freq = SYSTEM_MIN_FREQ;
        return SYSTEM_MIN_FREQ;
    }

    for(i = start_list; i != NULL; i = i->next)
    {
        /* Segregate based on CPU */
        if(i->cpu == cpu_id)
        {
            T[count] = (long)((i->T.tv_sec * ONE_SEC_MS) + (i->T.tv_nsec/ONE_SEC_NS));
            C[count] = (long)((i->C.tv_sec * ONE_SEC_MS) + (i->C.tv_nsec/ONE_SEC_NS));
            task_min[count] = 10000;
            count++;           
        }
    }

    //Sort on the Basis of T
    for(k = 0; k < task_on_cpu; k++) 
    {       
        for(j = 1; j < (task_on_cpu-k); j++)
        {
            if(T[j] < T[j-1])
            {
                tmp = T[j-1];
                T[j-1] = T[j];
                T[j] = tmp;
                
                tmp = C[j-1];
                C[j-1] = C[j];
                C[j] = tmp;
            }
        }
    }

    //For All Tasks
    for(j = 0; j < task_on_cpu; j++)
    {
        for(l = 1; T[j]*l <= T[task_on_cpu-1]; l++)
        {
            Tl = T[j]*l;
            for(k = 0; k < task_on_cpu; k++)
            {
                eta = 0;
                if(T[k] >= Tl)
                {
                    eta = calc_workload(k, Tl, C, T);
                    if(eta < task_min[k])
                    {
                        task_min[k] = eta;
                    }
                }
            }
        }
    }

    for(k = 0; k < task_on_cpu; k++)
    {
        if(task_min[k] > max_eta)
        {
            max_eta = task_min[k];
        }
    }
    
    max_eta = 130*max_eta;
    
    if(free_c != NULL)
    {
        kfree(free_c); 
    }

    if(free_t != NULL)
    {
        kfree(free_t); 
    }

    if(free_task != NULL)
    {
        kfree(free_task); 
    }
    
    printk("SYSCLOCK: %d WAS CALCULATED\n", max_eta);
    return max_eta;
}

static int sysclock_governor(struct cpufreq_policy *policy,
					unsigned int event)
{
    util_store_t *start_list = NULL;
    
    switch (event) 
    {
        case CPUFREQ_GOV_START:
            curr_policy[policy->cpu] = policy;
            policy->max = SYSTEM_MAX_FREQ;
            policy->min = SYSTEM_MIN_FREQ;
            is_sysclock_governor[policy->cpu] = true;
            printk("############# Starting SysClock Governor for CPU %d #############\n", policy->cpu);
            break;
        case CPUFREQ_GOV_LIMITS:
            start_list = get_start_list();
            apply_freq = sysclock_per_cpu(start_list, policy->cpu);
            __cpufreq_driver_target(policy, apply_freq, CPUFREQ_RELATION_L);
            printk("SYSCLOCK: Setting CPU: %d to %u Hz - event %u\n", policy->cpu, apply_freq, event);
            break;
        case CPUFREQ_GOV_STOP:
            printk("SYSCLOCK: Stopping governor on CPU: %d\n", policy->cpu);
            curr_policy[policy->cpu] = NULL;
            is_sysclock_governor[policy->cpu] = false;
            break;
        default:
            is_sysclock_governor[policy->cpu] = false;
            printk("DEFAULT ACTION FOR SYSCLOCK");
            break;
	}
	return 0;
}

void calc_sysclock(int task_count, int cpu)
{
    task_total = task_count;
    sysclock_governor(cpufreq_cpu_get(cpu), CPUFREQ_GOV_LIMITS);
}

static struct cpufreq_governor cpufreq_gov_sysclock = {
	.name		= "sysclock",
	.governor	= sysclock_governor,
	.owner		= THIS_MODULE,
};

static int __init cpufreq_gov_sysclock_init(void)
{
	return cpufreq_register_governor(&cpufreq_gov_sysclock);
}


static void __exit cpufreq_gov_sysclock_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_sysclock);
}


MODULE_AUTHOR("Rajat Mathur <rajatdem@andrew.cmu.edu>");
MODULE_DESCRIPTION("CPUfreq policy governor 'sysclock'");
MODULE_LICENSE("GPL");

fs_initcall(cpufreq_gov_sysclock_init);
module_exit(cpufreq_gov_sysclock_exit);
