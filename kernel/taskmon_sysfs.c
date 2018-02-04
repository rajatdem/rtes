#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/rtes.h>
#include<linux/cpufreq.h>

#define SIZE_NAME 10
#define PERMISSION 0666
#define MODULE_NAME "reserves"
#define PERMISSION 0666
#define SIZE_STRING50 50
#define CPU0 1<<0
#define CPU1 1<<1
#define CPU2 1<<2
#define CPU3 1<<3
#define MAX_NO_CPU 4

//extern struct mutex util_data_mutex;
static int enabled;
static int policy;
static int energy_enable;
static unsigned long system_energy = 0;

static struct kobject *taskmon;
static struct kobject *rtes;
static struct kobject *util;
static struct kobject *config;
static struct kobject *tasks;

static ssize_t reserve_show(struct kobject *kobj, \
        struct kobj_attribute *attr,\
        char *buf);
static ssize_t reserve_store(struct kobject *kobj, \
        struct kobj_attribute *attr, \
        const char *buf,\
        size_t count);
static ssize_t tid_show(struct kobject *kobj, \
        struct kobj_attribute *attr,\
        char *buf);
static ssize_t tid_store(struct kobject *kobj, \
        struct kobj_attribute *attr,\
        const char *buf,\
        size_t count);
static ssize_t enabled_show(struct kobject *kobj, \
        struct kobj_attribute *attr,\
        char *buf);
static ssize_t enabled_store(struct kobject *kobj, \
        struct kobj_attribute *attr,\
        const char *buf, size_t count);
static ssize_t  partition_show(struct kobject *kobj, \
        struct kobj_attribute *attr,\
        char *buf);
static ssize_t partition_store(struct kobject *kobj, \
        struct kobj_attribute *attr,\
        const char *buf, size_t count);

static ssize_t conf_energy_store(struct kobject *kobj, struct kobj_attribute *attr,
             const char *buf, size_t count);
static ssize_t conf_energy_show(struct kobject *kobj, struct kobj_attribute *attr,
            char *buf);

    static struct kobj_attribute enabled_attribute =
    __ATTR(enabled, PERMISSION, enabled_show, enabled_store);
static struct kobj_attribute partition_attribute =
    __ATTR(partition_policy, PERMISSION, partition_show, partition_store);
static struct kobj_attribute reserve_attribute =
    __ATTR(reserves, PERMISSION, reserve_show, reserve_store);
static ssize_t energy_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf);
static ssize_t energy_store(struct kobject *kobj, struct kobj_attribute *attr,
        const char *buf, size_t count);
static ssize_t freq_store(struct kobject *kobj, struct kobj_attribute *attr,
       const char *buf, size_t count);
static ssize_t freq_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf);
static ssize_t power_store(struct kobject *kobj, struct kobj_attribute *attr,
       const char *buf, size_t count);
static ssize_t power_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf);
static ssize_t tid_energy_store(struct kobject *kobj, struct kobj_attribute *attr,
       const char *buf, size_t count);
static ssize_t tid_energy_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf);

static struct kobj_attribute energy_attribute =
    __ATTR(energy, PERMISSION, energy_show, energy_store);
static struct kobj_attribute freq_attribute =
    __ATTR(freq, PERMISSION, freq_show, freq_store);
static struct kobj_attribute power_attribute =
    __ATTR(power, PERMISSION, power_show, power_store);
static struct kobj_attribute config_energy_attribute =
    __ATTR(energy, PERMISSION, conf_energy_show, conf_energy_store);


static ssize_t enabled_show(struct kobject *kobj, struct kobj_attribute *attr,
            char *buf)
{
    return sprintf(buf, "%d\n", enabled);
}
static ssize_t partition_show(struct kobject *kobj, struct kobj_attribute *attr,
            char *buf)
{
    int count;
    switch(policy)
    {
	case 0: count = sprintf(buf, "%s", "FF\n");
		    strcat(buf, "\n");
		    break;
	case 1: count = sprintf(buf, "%s", "NF\n");
		    strcat(buf, "\n");
		    break;
	case 2: count = sprintf(buf, "%s", "BF\n");
		    strcat(buf, "\n");
		    break;
	case 3: count = sprintf(buf, "%s", "WF\n");
		    strcat(buf, "\n");
		    break;
	case 4: count = sprintf(buf, "%s", "LST\n");
		    strcat(buf, "\n");
		    break;
	default:count = sprintf(buf, "%d", policy);
    }
    return count;
}

static ssize_t enabled_store(struct kobject *kobj, struct kobj_attribute *attr,
             const char *buf, size_t count)
{
    sscanf(buf, "%du", &enabled);
    if(enabled == 1)
    {
        enable_reservation_monitor();    
    }
    else
    {
        disable_reservation_monitor();
    }
    return count;
}
static ssize_t partition_store(struct kobject *kobj, struct kobj_attribute *attr,
             const char *buf, size_t count)
{
    int old_policy;
    char partition_policy[10] = {0};
    
    if(sizeof(buf) > 10)
    {
	    return -1;
    }
    
    sscanf(buf, "%su", partition_policy);
    old_policy = policy;
    if(strcmp(partition_policy, "FF")==0)
    {
	   policy = 0;
    }
    else if(strcmp(partition_policy, "NF")==0)
    {
    	policy = 1;
    }
    else if(strcmp(partition_policy, "BF")==0)
    {
	   policy = 2;    
    }
    else if(strcmp(partition_policy, "WF")==0)
    {
	   policy = 3;
    }
    else if(strcmp(partition_policy, "LST")==0)
    {
	   policy = 4;
    }
    else 
    {  
	   printk("USE OLD BIN PACKING FORMAT\n");
    }
    if(set_heuristic(policy))
    {
        policy = old_policy;
        return -EBUSY;
    }
    return count;
}

/*
 *  taskmon_init: Create nodes for task monitoring
 */
static int __init taskmon_init(void)
{
    int retval;

    rtes    = kobject_create_and_add("rtes", NULL);
    taskmon = kobject_create_and_add("taskmon", rtes);
    config  = kobject_create_and_add("config", rtes);
    tasks   = kobject_create_and_add("tasks", rtes);
    util    = kobject_create_and_add("util", taskmon); 
    
    if ((!taskmon) && (!rtes) && (!util) && (!tasks) && (!config))
    {
        return -ENOMEM;
    }

    /* Create the files associated with this kobject */
    retval = sysfs_create_file(rtes, &(reserve_attribute.attr));
    if (retval)
    {
        kobject_put(rtes);
    }
    
    retval = sysfs_create_file(rtes, &(partition_attribute.attr));
    if (retval)
    {
        kobject_put(rtes);
    }	
 
    retval = sysfs_create_file(rtes, &(energy_attribute.attr));
    if (retval)
    {
        kobject_put(rtes);
    }
   
    retval = sysfs_create_file(rtes, &(freq_attribute.attr));
    if (retval)
    {
        kobject_put(rtes);
    }

    retval = sysfs_create_file(rtes, &(power_attribute.attr));
    if (retval)
    {
        kobject_put(rtes);
    }
    retval = sysfs_create_file(taskmon, &(enabled_attribute.attr));
    if (retval)
    {
        kobject_put(taskmon);
    }
   
    retval = sysfs_create_file(config, &(config_energy_attribute.attr));
    if (retval)
    {
        kobject_put(config);
    }
 

    return retval;
}

/*
 * Create a file for each thread
 *
 */
struct kobj_attribute * create_node(char *tid_para)
{
    int retval;
    struct kobj_attribute *tid_attribute;
    
    tid_attribute = (struct kobj_attribute *)kmalloc(sizeof(struct kobj_attribute *), GFP_ATOMIC);
    tid_attribute->attr.name = (char *)kmalloc(SIZE_NAME, GFP_ATOMIC);
    strcpy(tid_attribute->attr.name, tid_para);
    tid_attribute->attr.mode = PERMISSION;
    tid_attribute->show = tid_show;
    tid_attribute->store = tid_store;
    
    if((retval = sysfs_create_file(util, &((tid_attribute)->attr))) == 0) 
    {
        return tid_attribute;
    } 
    else 
    {
        return NULL;
    }
}

/*
 * Create tasks/#pid#/energy node 
 */
struct kobj_attribute *create_energy_node(char *tid_para, struct kobject **task_tid_return)
{
    int retval;
    struct kobj_attribute *pid_energy_attribute;
    struct kobject *task_tid;

    pid_energy_attribute = (struct kobj_attribute *)kmalloc(sizeof(struct kobj_attribute *), GFP_ATOMIC);
    pid_energy_attribute->attr.name = (char *)kmalloc(SIZE_NAME, GFP_ATOMIC);
    strcpy(pid_energy_attribute->attr.name, "energy");
    pid_energy_attribute->attr.mode = PERMISSION;
    pid_energy_attribute->show = tid_energy_show;
    pid_energy_attribute->store = tid_energy_store;


    task_tid  = kobject_create_and_add(tid_para, tasks);

    *task_tid_return = task_tid;

    if((retval = sysfs_create_file(task_tid, &((pid_energy_attribute)->attr))) == 0) 
    {
        return pid_energy_attribute;
    } 
    else 
    {
        return NULL;
    }

}
/*
 * Delete a file for each thread
 */
void delete_node(struct kobj_attribute *kobj)
{
    sysfs_remove_file(util, &(kobj->attr));
    if(kobj->attr.name != NULL)
    {
        kfree(kobj->attr.name);
    }
}

/*
 * Delete a node
 */
void delete_energy_node(struct kobject *task_tid, struct kobj_attribute *kobj)
{

    sysfs_remove_file(task_tid, &(kobj->attr));
    if(kobj->attr.name != NULL)
    {
        kfree(kobj->attr.name);
    }
    kobject_put(task_tid);
    //kobject_del(task_tid);

}

/*
 * Create a file for each thread
 */
static ssize_t tid_show(struct kobject *kobj, struct kobj_attribute *attr,
            char *buf)
{
    int err;
    long pid;
    util_store_t *i, *start_util;
   
    err = kstrtol(attr->attr.name, 10, &pid);
    if(err)
    {
        return 0;
    }
    
    start_util = get_start_list();
   
    //err = mutex_lock_interruptible(&util_data_mutex);
    for(i = start_util; i!=NULL; i= i->next)
    {
        if(pid == i->tid)
        {
            if(i->buff != NULL)
            {
                err = sprintf(buf, "%s\n", i->buff);
                kfree(i->buff);
                i->buff = NULL;
                //mutex_unlock(&util_data_mutex);
                return err;
            }
        }
    }
    //mutex_unlock(&util_data_mutex);
    return 0;
}

static ssize_t tid_store(struct kobject *kobj, struct kobj_attribute *attr,
             const char *buf, size_t count)
{
    return 0;
}


/*
 * reserve_show: Display thread properties
 */
static ssize_t reserve_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{
    int ret, cpu;
    struct task_struct *task = NULL, *p, *t;
    char *print_buf;
    char thread_buf[SIZE_STRING50];
    char name[TASK_COMM_LEN];
    unsigned int rt_priority = 0;
    unsigned long mask, count = 0;
    pid_t tid = 0, pid = 0;
    util_store_t *i, *start_util;

    start_util = get_start_list();

    print_buf = (char *)kcalloc(SIZE_STRING50, 1, GFP_KERNEL);
    if(print_buf == NULL)
    {
        printk("%s: Could not allocate buffer\n", MODULE_NAME);
        return 0;
    }
    ret = sprintf(print_buf, "TID\tPID\tPRIO\tCPU\tNAME\n");

    //mutex_lock(&util_data_mutex);
    for(i = start_util; i!=NULL; i= i->next)
    {
        rcu_read_lock();
        do_each_thread(p, t)
        {
            if(i->tid == t->pid)
            {
                task = t;
                pid = t->tgid;
                tid = t->pid;
                rt_priority = t->rt_priority;
                strcpy(name, t->comm);
                goto got_task;
            }

        }while_each_thread(p, t);

got_task:
        
        if(task != NULL)
        { 
            ret = sched_getaffinity(tid, (void *)&mask);
            rcu_read_unlock();
            switch(mask)
            {
                case CPU0: cpu = 0;
                           break;
                case CPU1: cpu = 1;
                           break;
                case CPU2: cpu = 2;
                           break;
                case CPU3: cpu = 3;
                           break;
                default:
                    cpu = -1;
            }
            if(-1 != cpu)
            {
                sprintf(thread_buf, "%d\t%d\t%d\t%d\t%s\n", tid, \
                        pid, \
                        rt_priority, \
                        cpu,\
                        name);
                print_buf = (char*)krealloc(print_buf, SIZE_STRING50*(count+1), GFP_ATOMIC);
                if(print_buf == NULL)
                {
                    //mutex_unlock(&util_data_mutex);
                    printk("%s: Could not allocate buffer\n", MODULE_NAME);
                    return 0;
                }
                strcat(print_buf, thread_buf);
            }
        }
        else
        {
            rcu_read_unlock();
        }
    }
    //mutex_unlock(&util_data_mutex);

    return sprintf(buf, "%s", print_buf);
}

/*
 * reserve_store: Do nothing
 */
static ssize_t reserve_store(struct kobject *kobj, struct kobj_attribute *attr,
       const char *buf, size_t count)
{
    return 0;
}

static ssize_t conf_energy_store(struct kobject *kobj, struct kobj_attribute *attr,
             const char *buf, size_t count)
{
    sscanf(buf, "%du", &energy_enable);
    set_energy_mon(energy_enable);
    return count;
}

static ssize_t conf_energy_show(struct kobject *kobj, struct kobj_attribute *attr,
            char *buf)
{
    return sprintf(buf, "%d\n", energy_enable);
}

static ssize_t energy_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{ 
    return sprintf(buf, "%lu\n", (system_energy/1000));
}

static ssize_t energy_store(struct kobject *kobj, struct kobj_attribute *attr,
             const char *buf, size_t count)
{
    system_energy = 0;
    return count;
}

static ssize_t freq_store(struct kobject *kobj, struct kobj_attribute *attr,
       const char *buf, size_t count)
{
    return 0;
}

static ssize_t freq_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{
    unsigned long freq[MAX_NO_CPU], max = 0;
    int i;

    for(i=0; i<MAX_NO_CPU; i++){
        freq[i] = cpufreq_get(i);
        if(max < freq[i])
            max = freq[i];
    }
    return sprintf(buf, "%lu\n", (max/1000));
}

static ssize_t power_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{
    unsigned long power = 0, freq = cpufreq_get(0);
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
        default: break;
        
    }
    return sprintf(buf, "%lu\n", power);
}

static ssize_t power_store(struct kobject *kobj, struct kobj_attribute *attr,
       const char *buf, size_t count)
{
    return 0;
}

static ssize_t tid_energy_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    struct task_struct *p = NULL, *t = NULL;
    pid_t tid;
    int err = 0;
    unsigned long energy_tid = 0;

    err = kstrtol(kobj->name, 10, (long *)&tid);
    if(err)
    {
        return 0;
    }
    
    rcu_read_lock();
    do_each_thread(p, t)
    {
        if(tid == t->pid)
        {
            energy_tid = t->rtes.energy_accum;
            goto got_task;
        }
    }while_each_thread(p, t);

got_task:
    rcu_read_unlock();

    return sprintf(buf, "%lu\n", (energy_tid/1000));
}

static ssize_t tid_energy_store(struct kobject *kobj, struct kobj_attribute *attr,
       const char *buf, size_t count)
{
    return 0;
}
    
void set_global_energy_count(unsigned long energy)
{
    system_energy += energy;
}
module_init(taskmon_init);
MODULE_LICENSE("GPL");

