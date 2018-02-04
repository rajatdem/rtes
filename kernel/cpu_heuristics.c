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
#include <linux/cpu.h>

#include <asm/unistd.h>
#include <asm/current.h>

#include <linux/rtes.h>

#define ONE_SEC_MS 1000
#define ONE_SEC_NS 1000000
#define SIZE_STRING15 15

static cpu_t c[4];
static int curr_cpu;

long calc_util(struct timespec *C, struct timespec *T)
{

	char *utilization = NULL;
    char param_C[SIZE_STRING15] = {0};
    char param_T[SIZE_STRING15] = {0};
    long util_curr = 0;

	sprintf(param_C, "%ld", (long)((C->tv_sec * ONE_SEC_MS) + (C->tv_nsec/ONE_SEC_NS)));
    sprintf(param_T, "%ld", (long)((T->tv_sec * ONE_SEC_MS) + (T->tv_nsec/ONE_SEC_NS)));
    utilization = calculator(param_C, param_T, '/');
    sprintf(param_T, "%d", 1000);
    utilization = calculator(utilization, param_T, '*');
    utilization = strsep(&utilization, ".");

    if((kstrtol(utilization, 10, &util_curr)) != 0)
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
    return util_curr;
}
	

int first_fit(pid_t tid, struct timespec *C, struct timespec *T, long *util_curr)
{
	//check from CPU0 to CPU3
	int i; 
	for(i = 0; i<4; i++)
	{
		if(c[i].state)
		{
			if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
			{
				printk(">>>>>>>>>SELECTING CPU=%d>>>>", c[i].cpu_id);
				return c[i].cpu_id;
			}
		}	
	}
	for(i = 0; i<4 ;i++)
	{
		if(!c[i].state)
		{
			if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
			{
				printk(">>>>>OPEN:%d>>>>>at position:%d\n", c[i].cpu_id, i);
				return c[i].cpu_id;
			}
		}
	}	
	return -1;
}

int next_fit(pid_t tid, struct timespec *C, struct timespec *T, long *util_curr)
{
	int i;
	int flag = 0;

	if(!check_schedulability(tid, C, T, curr_cpu, util_curr))
	{	 
		flag = 1;
		return curr_cpu;
	}
#if 0
	if(curr_cpu == 3)
	{
		curr_cpu = 0;
	}
	else
	{
		curr_cpu++;
	}	
	x = curr_cpu;
#endif
	if(curr_cpu!=3)
	{
		for(i = curr_cpu; i<4; i++)
		{
			if(c[i].state)
			{
				if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
				{	 
					flag = 1;
					curr_cpu = c[i].cpu_id;
					return c[i].cpu_id;
	    		}
			}
		}
	}
	if(!flag)
	{
		if(curr_cpu!=0)
		{
			for(i =0; i < (curr_cpu-1); i++)
			{
				if(c[i].state)
				{
					if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
					{	 
						flag = 1;
						curr_cpu = c[i].cpu_id;
						return c[i].cpu_id;
	    			}
				}
			}
		}
	}       
	//OPEN BINS
	if(curr_cpu!=3)
	{
		for(i = curr_cpu; i<4; i++)
		{
			if(!c[i].state)
			{
				if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
				{	 
					flag = 1;
					curr_cpu = c[i].cpu_id;
					printk(">>>>>OPEN:%d>>>>>at position:%d\n", c[i].cpu_id, i);
					return c[i].cpu_id;
	    		}
			}
		}
	}
	if(!flag)
	{
		if(curr_cpu!=0)
		{
			for(i =0; i < (curr_cpu-1); i++)
			{
				if(!c[i].state)
				{
					if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
					{	 
						flag = 1;
						curr_cpu = c[i].cpu_id;
						printk(">>>>>OPEN:%d>>>>>at position:%d\n", c[i].cpu_id, i);
						return c[i].cpu_id;
	    			}
				}
			}
		}
	}       

#if 0
	for(i = curr_cpu; i<4; i++)
	{
		if(!c[i].state)
		{
			if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
			{	 
				c[i].state = 1;
				curr_cpu = c[i].cpu_id;
				printk("OPEN: %d\n", c[i].cpu_id);
				printk("The ith posititon is %d\n", i);
				return c[i].cpu_id;
	    	}
		}
	}
	if(!flag)
	{
		for(i =0; i < (x-1); i++)
		{
			if(!c[i].state)
			{
				if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
				{	
					c[i].state = 1; 
					curr_cpu = c[i].cpu_id;
					printk("OPEN: %d\n", c[i].cpu_id);
					printk("The ith posititon is %d\n", i);
					return c[i].cpu_id;
	    		}
			}
		}
	}
#endif
	return -1;
}

//descending order arranged CPUs according to utilization
int best_fit(pid_t tid, struct timespec *C, struct timespec *T, long *util_curr)
{	//sort by utilization
	int i, j; 
	//long utilization[4];//
	cpu_t tmp;
	for(i = 0; i < 3; i++)
	{
		for(j = 1; j < (4-i); j++)
		{
			if(c[j].util > c[j-1].util)
			{
				tmp = c[j-1];
				c[j-1] = c[j];
				c[j] = tmp;
			}
		}
	}
		
	printk("%d:%d:%d:%d\n", c[0].cpu_id, c[1].cpu_id, c[2].cpu_id, c[3].cpu_id);

	// sorted 
	for(i = 0; i < 4; i++)
	{
		if(c[i].state)
		{
			if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
			{
				printk("The ith posititon is %d\n", i);
				return c[i].cpu_id;
			}
		}	
	}
	// nothing returned from above so open bins
	for(i = 0; i<4 ;i++)
	{
		if(!c[i].state)
		{
			if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
			{
				printk(">>>>>OPEN:%d>>>>>at position:%d\n", c[i].cpu_id, i);
				return c[i].cpu_id;
			}
		}
	}
	//not possible on bins also
	return -1;
}

//ascending order of util
int worst_fit(pid_t tid, struct timespec *C, struct timespec *T, long *util_curr)
{	//sort by utilization
	int i, j; 
	//long utilization[4];//
	cpu_t tmp;
	for(i = 0; i < 3; i++)
	{
		for(j = 1; j < (4-i); j++)
		{
			if(c[j].util < c[j-1].util)
			{
				tmp = c[j-1];
				c[j-1] = c[j];
				c[j] = tmp;
			}
		}
	}
		
	printk("%d:%d:%d:%d\n", c[0].cpu_id, c[1].cpu_id, c[2].cpu_id, c[3].cpu_id);

	// sorted 
	for(i = 0; i < 4; i++)
	{
		if(c[i].state)
		{
			if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
			{
				printk("The ith posititon is %d\n", i);
				return c[i].cpu_id;
			}
		}	
	}
	// nothing returned from above so open bins
	for(i = 0; i<4 ;i++)
	{
		if(!c[i].state)
		{
			if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
			{
				printk(">>>>>OPEN:%d>>>>>at position:%d\n", c[i].cpu_id, i);
				return c[i].cpu_id;
			}
		}
	}
	//not possible on bins also
	return -1;
}

int list_sched(pid_t tid, struct timespec *C, struct timespec *T, long *util_curr)
{
	int i, j; 
	cpu_t tmp;
	for(i = 0; i < 3; i++)
	{
		for(j = 1; j < (4-i); j++)
		{
			if(c[j].util < c[j-1].util)
			{
				tmp = c[j-1];
				c[j-1] = c[j];
				c[j] = tmp;
			}
		}
	}

	for(i = 0; i<4; i++)
	{
		if(!check_schedulability(tid, C, T, c[i].cpu_id, util_curr))
		{
			printk(">>>>>>>>>SELECTING CPU=%d>>>>", c[i].cpu_id);
			return c[i].cpu_id;
		}	
	}

	return -1;
}

void set_cpu_data(int cpu, long util)
{
	int i =0;
	for(i = 0; i < 4; i++)
	{
		if(c[i].cpu_id == cpu)
		{
			/* Make state += 1 */
			c[i].state++;
			c[i].util += util;
			curr_cpu = i;

	        printk("reserve: CPU = %d\n", c[i].cpu_id);
			//break;
		}

        /* Turn off unused cores */
        if(!c[i].state && c[i].cpu_id != 0)
        {
            /* Turn off this cpu */
            if(!!cpu_online(c[i].cpu_id))
            {
                cpu_hotplug_driver_lock();
                printk("Turning off cpu %d\n", (int)c[i].cpu_id);
                cpu_down(c[i].cpu_id);
                cpu_hotplug_driver_unlock();
            }
        }
	}
}

void remove_cpu_data(int cpu, long util)
{
	int i = 0;
	int is_closing = 0;
    for(i = 0; i < 4; i++)
	{
		if(c[i].cpu_id == cpu)
		{
			/*Make state -= 1*/
			c[i].state--;
			c[i].util -= util;
		    is_closing = 1;
            printk("~~~~~~REMOVE A TASK FROM CPU:%d:STATE:%d:UTIL:%ld~~~~~~\n", c[i].cpu_id, c[i].state, c[i].util);
			break;
		}  
	}
    if(!c[i].state)
    {
		printk("~~~~~~~CLOSING CPU:%d~STATE:%d~UTIL:%ld~~~~~\n", c[i].cpu_id, c[i].state, c[i].util);
	}
    //only for NF
    if(!c[i].state && is_closing==1 && c[i].cpu_id == curr_cpu)
    {
        if(c[i].cpu_id!=3)
        {
		    for(i = curr_cpu; i<4; i++)
		    {
			    if(c[i].state)
			    {
				     is_closing = 0;
				     curr_cpu = i;
                     break;
			    }
		    }
	    }
    }
	
    if(is_closing && c[i].cpu_id == curr_cpu)
	{
		if(c[i].cpu_id!=0)
		{
			for(i =0; i < (curr_cpu-1); i++)
			{
				if(c[i].state)
				{
                    is_closing = 0;
                    curr_cpu = i;
                    break;
				}
			}
		}
	}

    /* Turn off unused cores 
     * Make sure you don't attempt to turn off CPU 0 */
    for(i = 0; i < 4; i++)
    {
        if(!c[i].state)
        {
            if((c[i].cpu_id) && (!!cpu_online(c[i].cpu_id)))
            {
                /* Turn off this core */
                //change_governor(c[i].cpu_id);
                cpu_hotplug_driver_lock();
                printk("Turning off cpu %d\n", (int)c[i].cpu_id);
                cpu_down(c[i].cpu_id);
                printk("CPU IS DOWN\n");
                cpu_hotplug_driver_unlock();
            }
        }
    }
}

int additional_bonus(pid_t tid, struct timespec *C, struct timespec *T)
{
	util_store_t *i;
	int new_cpu = 0;
	//mutex_lock(&util_store_mutex);
	for(i = get_start_list(); i != NULL; i = i->next)
	{
		new_cpu = set_reservation(i->tid, &(i->C), &(i->T), -1);
		printk("reserve: TID = %d : CPU = %d", tid, (new_cpu - 100));
	}
	//mutex_unlock(&util_store_mutex);
	new_cpu = set_reservation(tid, C, T, -1);
	if(new_cpu < 100)
    {
        printk("Unable to set reservation: %d\n", new_cpu); 
    }
    else
    {
		printk("reserve: CPU = %d\n", (new_cpu - 100));
    }
    return new_cpu;
}

void heuristic_init_local(void)
{
	int i = 0;
	curr_cpu = 0;
	
    for(i = 0; i < 4; i++)
	{
		c[i].cpu_id = i;
		c[i].state = 0;
		c[i].util = 0;
	}
}
static int __init heuristic_init(void)
{	
    heuristic_init_local();
    return 0;
}

module_init(heuristic_init);
MODULE_LICENSE("GPL");
