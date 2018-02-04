/* Lab1
 * First char driver
 */
#include<linux/kernel.h>
#include<linux/device.h>
#include<linux/module.h>
#include<linux/ioctl.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/slab.h>
#include<linux/errno.h>
#include<linux/sched.h>
#include<linux/rcupdate.h>
#include<linux/string.h>
#include<linux/uaccess.h>
#include<linux/slab.h>

#define MODULE_NAME "psdev"
#define NO_OF_INSTANCES 255
#define DATA_LENGTH 50

static int psdev_open(struct inode *inode, struct file *file);
static int psdev_release(struct inode *inode, struct file *file);
static ssize_t psdev_read(struct file *file, __user char *buf, size_t count, loff_t *pos);
static ssize_t psdev_write(struct file *file, const __user char *buf, size_t count, loff_t *pos);
static long psdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static loff_t psdev_llseek(struct file *file, loff_t offset, int orig);

struct psdev_info 
{
    dev_t major;
    dev_t max_minor;
    struct cdev cdev;
    struct class *class;
};

static struct file_operations psdev_fops =
{
    .owner   = THIS_MODULE,
    .open    = psdev_open,
    .read    = psdev_read,
    .write   = psdev_write,
    .unlocked_ioctl = psdev_ioctl,
    .llseek  = psdev_llseek,
    .release = psdev_release,
};

struct psdev_info *psdev_gbl_info;
static unsigned int open_rec[NO_OF_INSTANCES] = {0};
static uint8_t dev_minor = 0;
typedef char psdev_data_t;
static int found_eof[NO_OF_INSTANCES] = {0};
static psdev_data_t* user_data[NO_OF_INSTANCES] = {NULL};


static int psdev_open(struct inode *inode, struct file *file)
{
    struct psdev_info *dev;
    unsigned int minor;

    /* If user wants to create a new device instance */
    if((file->f_flags & O_ACCMODE) == O_CREAT)
    {
        if(NO_OF_INSTANCES >= dev_minor)
        {
            /* Create new device node */
            if(IS_ERR(device_create(dev->class, NULL, dev->major, NULL, MODULE_NAME"%d", dev_minor)))
            {
                printk(MODULE_NAME " Unable to create node /dev/%s%d\n",MODULE_NAME, dev_minor);
                return -EBUSY;
            }
            dev_minor++;
        }
    }
    else if ((file->f_flags & O_ACCMODE) == O_RDONLY)
    {
        minor = iminor(inode);
        if(!open_rec[minor])
        {
            //dev = (struct psdev_info *) container_of(inode->i_cdev, struct psdev_info, cdev);
            //file->private_data = dev;
            file->private_data = (void *)minor;
            open_rec[minor] = true;
            printk("%s: Device open - Minor = %d\n", MODULE_NAME, minor);
        }
        else
        {
            printk("Trying to open again - BUSY\n");
            return -EBUSY;     
        }
    }
    else
    {
        return -ENOTSUPP;
    }
   return 0; 
}

static int psdev_release(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    open_rec[minor] = false;
    file->private_data = NULL;
    if(user_data[minor] != NULL)
    {
        kfree(user_data[minor]);
    }
    return 0; 
}

static ssize_t psdev_read(struct file *file, __user char *buf, size_t count, loff_t *pos)
{
    unsigned int count_alloc = 0;
    struct task_struct *tsk_details;
    char temp_buf[50];
    int ret = 0, i = 0;
    unsigned int minor = (unsigned int)file->private_data;

    if(!found_eof[minor] && (0 == *pos))
    {
        rcu_read_lock();
        for_each_process(tsk_details)
        {
            count_alloc++;
            if(NULL == user_data[minor])
            {
                user_data[minor] = (psdev_data_t *) kcalloc(2, DATA_LENGTH, GFP_KERNEL);
                strcat(user_data[minor], "TID\tPID\tRT PRIO\tNAME\n");
            }
            else
            {
                user_data[minor] = (psdev_data_t *) krealloc(user_data[minor],\
                                    DATA_LENGTH*count_alloc+1, GFP_KERNEL);
                if(user_data[minor] == NULL )
                {
                    goto mem_err;
                }
            }
            if(tsk_details->rt_priority < MAX_RT_PRIO)
            {
                sprintf(temp_buf, "%d\t", tsk_details->pid);    //tid
                strcat(user_data[minor], temp_buf);
                
                if(tsk_details->real_parent == NULL)
                {
                    /* This is a parent process */
                    sprintf(temp_buf, "%d\t", tsk_details->pid);
                    strcat(user_data[minor], temp_buf);
                }
                else
                {
                    /* This is a thread */
                    sprintf(temp_buf, "%d\t", tsk_details->real_parent->pid);
                    strcat(user_data[minor], temp_buf);
                }
                sprintf(temp_buf, "%d\t%s\n",\
                        (int)tsk_details->rt_priority,\
                        tsk_details->comm);
                strcat(user_data[minor], temp_buf);
            }
        };
        rcu_read_unlock();
    }
    for(i = *pos; i < count+(*pos); i++)
    {
        if(found_eof[minor])
        {
            found_eof[minor] = false;
            return 0;
        }
        if(user_data[minor][i] == '\0')
        {
            ret = copy_to_user(buf, &user_data[minor][*pos], (i+1-(*pos)));
            if(ret)
            {
                kfree(user_data[minor]);
                user_data[minor] = NULL;
                return -1;
            }
            else
            {
                kfree(user_data[minor]);
                user_data[minor] = NULL;
                if((i+1-(*pos)) == count)
                {
                    return 0;
                }
                else
                {
                    found_eof[minor] = true;
                    return (i+1-(*pos));
                }
            }
        }
    }
    ret = copy_to_user(buf, &user_data[minor][*pos], count);
    if(ret)
    {
        return -1;
    }
    *pos = count;
    return count;

mem_err:
    return -1;

}


static ssize_t psdev_write(struct file *file, const __user char *buf, size_t count, loff_t *pos)
{
   return -ENOTSUPP;
}

static long psdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
   return -ENOTSUPP;
}

static loff_t psdev_llseek(struct file *file, loff_t offset, int orig)
{
   return -ENOTSUPP;
}

/* Register character driver */
static int __devinit register_psdev_driver(struct psdev_info *info)
{
    int error = -ENODEV;

    /* Request device major number */
    if(alloc_chrdev_region(&info->major, 0, NO_OF_INSTANCES, MODULE_NAME) < 0)
    {
        printk("%s: Unable to allocate char device region\n", MODULE_NAME);
        return error;
    }

    /* Create sysfs entry */
    info->class = class_create(THIS_MODULE, MODULE_NAME);
    if(IS_ERR(info->class))
    {
        printk(MODULE_NAME " Unable to create class");
        unregister_chrdev_region(info->major, NO_OF_INSTANCES);
        return error;
    }

    /* Connect file ops */
    cdev_init(&info->cdev, &psdev_fops);
    info->cdev.owner = THIS_MODULE;
    info->cdev.ops = &psdev_fops;
    /* Connect Major/Minor number */
    if(cdev_add(&info->cdev, info->major, NO_OF_INSTANCES) < 0)
    {
        printk("%s: Unable to add char driver\n", MODULE_NAME);
        unregister_chrdev_region(info->major, NO_OF_INSTANCES);
        return error;
    }
    
    /* Create /dev/psdev node */
    if(IS_ERR(device_create(info->class, NULL, info->major, NULL, MODULE_NAME)))
    {
        printk(MODULE_NAME " Unable to create node in /dev\n");
        unregister_chrdev_region(info->major, NO_OF_INSTANCES);
        return error;
    }
    return 0;
}

static void unregister_psdev_driver(struct psdev_info *info)
{
    device_destroy(info->class, info->major);
    class_destroy(info->class);
    cdev_del(&info->cdev);
    unregister_chrdev_region(info->major, NO_OF_INSTANCES);
    kfree(info);
}

/* Driver init */
int __init psdev_init(void)
{
    int error = -ENOMEM;
    struct psdev_info *psdev_init_info;

    psdev_init_info = kmalloc(sizeof(*psdev_init_info), GFP_KERNEL);
    if(!psdev_init_info)
    {
        printk("%s: Unable to allocate memory\n", MODULE_NAME);
        return -ENOMEM;
    }
    psdev_gbl_info = psdev_init_info;

    /* Register the driver */
    error = register_psdev_driver(psdev_init_info);
    if(error < 0)
    {
        printk("%s: Unable to register driver interface errno: %d\n", MODULE_NAME, error);
        kfree(psdev_init_info);
        return error;
    }

    printk(KERN_INFO MODULE_NAME ": Initialized\n");
    return 0;
}

/* Driver exit */
void __exit psdev_exit(void)
{
    unregister_psdev_driver(psdev_gbl_info);
    return;
}


module_init(psdev_init);
module_exit(psdev_exit);
MODULE_LICENSE("GPL");
    
