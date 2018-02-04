/* Lab1
 * Cleanup kernel module
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/syscalls.h>

#include <asm/unistd.h>
#include <asm/current.h>

#define MODULE_NAME "cleanup"
#define MAX_SIZE 100

static char *comm = NULL;
unsigned long *original_exit;
extern void *sys_call_table[];

module_param(comm, charp, 0);
MODULE_PARM_DESC(comm, "Name of the process to monitor");

void check_open_files(struct task_struct *task)
{
    int first = true, ii;
    struct path files_path;
    char *path;
    char open_file[MAX_SIZE] = {0};
   
    if((comm != NULL) && (strstr(task->comm, comm) != NULL))
    {
        for(ii = 0; ii < NR_OPEN_DEFAULT; ii++)
        {
            if(task->files->fd_array[ii] != NULL)
            {
                if(first)
                {
                    first = false;
                    printk("%s: process '%s' (PID) %d did not close files:\n",\
                            MODULE_NAME,\
                            task->comm,\
                            task->pid);
                }
                files_path = task->files->fd_array[ii]->f_path;
                path = d_path(&files_path, open_file, MAX_SIZE);
                if(NULL != path)
                {
                    printk("%s: %s\n", MODULE_NAME, path);
                }
                else
                {
                    printk("%s: Unable to get file path\n", MODULE_NAME);
                }
            }
        }
    }
}

/* exit_hack: would perform the required job and then call exit */
asmlinkage unsigned long exit_hack(int error_code)
{
    check_open_files(current);
    do_exit((error_code&0xff)<<8);
}

/* Driver init */
int __init cleanup_init(void)
{
    /* Replacing Exit call with our exit */
    original_exit = (unsigned long *)sys_call_table[__NR_exit_group];
    sys_call_table[__NR_exit_group] = (unsigned long *)exit_hack; 
    printk("%s: Initialized\n", MODULE_NAME);
    return 0;
}

void cleanup_exit(void)
{
    /* Inserting the original exit call */
    sys_call_table[__NR_exit_group] = original_exit; 
    printk("%s: Exiting cleanup\n", MODULE_NAME);
}

module_init(cleanup_init);
module_exit(cleanup_exit);
MODULE_LICENSE("GPL");
