#include <linux/kernel.h>
#include <linux/cpuidle.h>

static int deep_sleep_select(struct cpuidle_device *dev)
{
    printk("DEEP SLEEP: Going into Deep Sleep\n");
    return dev->state_count -1;
}
static struct cpuidle_governor deep_sleep_governor = 
{
    .name = "deepsleep",
    .rating  = 1,
    .select = deep_sleep_select,
    .owner  = THIS_MODULE,
};

static int __init init_deep_sleep(void)
{
    return cpuidle_register_governor(&deep_sleep_governor);
}

static void __exit exit_deep_sleep(void)
{
    return cpuidle_unregister_governor(&deep_sleep_governor);
}

MODULE_LICENSE("GPL");
module_init(init_deep_sleep);
module_exit(exit_deep_sleep);
