#include<linux/module.h>
int init_module(void)
{
    printk(KERN_ALERT "Hello, world! Kernel-space -- the land of the free and the home of the brave\n");
    return 0;
}
void cleanup_module(void)
{
    return;
}
