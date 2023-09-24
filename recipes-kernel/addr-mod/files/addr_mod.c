#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

static unsigned long watch_address = 0x12345; // Default address

static struct task_struct *watch_thread;
static struct kobject *watch_kobj;
static int read_watch_enabled = 0;
static int write_watch_enabled = 0;

// Define callback function prototypes
static void read_callback(unsigned long address);
static void write_callback(unsigned long address);

// Function to print backtrace
static void print_backtrace(void)
{
    printk(KERN_INFO "Backtrace:\n");
    dump_stack();
}

// Function to check if the address matches the watch address
static int is_watch_address(unsigned long address)
{
    return (address == watch_address);
}

// Thread function to watch for read/write accesses
static int watch_thread_fn(void *data)
{
    while (!kthread_should_stop()) {
        if (read_watch_enabled) {
            unsigned long value;
            if (copy_from_user(&value, (void __user *)watch_address, sizeof(unsigned long)) == 0) {
                // Read access
                read_callback(watch_address);
                print_backtrace();
            }
        }

        if (write_watch_enabled) {
            unsigned long new_value = 0; // Replace with the desired new value
            unsigned long old_value;
            if (copy_to_user((void __user *)watch_address, &new_value, sizeof(unsigned long)) == 0 &&
                copy_from_user(&old_value, (void __user *)watch_address, sizeof(unsigned long)) == 0) {
                // Write access
                write_callback(watch_address);
                print_backtrace();
            }
        }

        schedule(); // Yield the CPU
    }
    return 0;
}

// Read callback function
static void read_callback(unsigned long address)
{
    printk(KERN_INFO "Read access at address 0x%lx\n", address);
    print_backtrace();
}

// Write callback function
static void write_callback(unsigned long address)
{
    printk(KERN_INFO "Write access at address 0x%lx\n", address);
    print_backtrace();
}

// Sysfs attributes to enable/disable read and write watch
static ssize_t read_watch_enabled_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", read_watch_enabled);
}

static ssize_t read_watch_enabled_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int val;
    if (sscanf(buf, "%d", &val) == 1) {
        read_watch_enabled = val;
        return count;
    }
    return -EINVAL;
}

static ssize_t write_watch_enabled_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", write_watch_enabled);
}

static ssize_t write_watch_enabled_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int val;
    if (sscanf(buf, "%d", &val) == 1) {
        write_watch_enabled = val;
        return count;
    }
    return -EINVAL;
}

static struct kobj_attribute read_watch_enabled_attribute = __ATTR_RW(read_watch_enabled);
static struct kobj_attribute write_watch_enabled_attribute = __ATTR_RW(write_watch_enabled);

// Sysfs attribute for setting the watch address
static ssize_t watch_address_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%lx\n", watch_address);
}

static ssize_t watch_address_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    unsigned long addr;
    if (kstrtoul(buf, 0, &addr) == 0) {
        watch_address = addr;
        return count;
    }
    return -EINVAL;
}

static struct kobj_attribute watch_address_attribute = __ATTR_RW(watch_address);

// Module initialization
static int __init watchpoint_module_init(void)
{
    // Create a sysfs kobject
    watch_kobj = kobject_create_and_add("watchpoint", kernel_kobj);

    if (!watch_kobj)
        return -ENOMEM;

    // Create sysfs files for enabling/disabling read and write watch
    if (sysfs_create_file(watch_kobj, &read_watch_enabled_attribute.attr) ||
        sysfs_create_file(watch_kobj, &write_watch_enabled_attribute.attr) ||
        sysfs_create_file(watch_kobj, &watch_address_attribute.attr)) {
        kobject_put(watch_kobj);
        return -ENOMEM;
    }

    // Create a kernel thread for watching
    watch_thread = kthread_run(watch_thread_fn, NULL, "watch_thread");

    if (IS_ERR(watch_thread)) {
        kobject_put(watch_kobj);
        return PTR_ERR(watch_thread);
    }

    printk(KERN_INFO "Watchpoint module loaded\n");
    return 0;
}

// Module cleanup
static void __exit watchpoint_module_exit(void)
{
    // Stop and cleanup the watch thread
    kthread_stop(watch_thread);

    // Remove sysfs files and kobject
    sysfs_remove_file(watch_kobj, &read_watch_enabled_attribute.attr);
    sysfs_remove_file(watch_kobj, &write_watch_enabled_attribute.attr);
    sysfs_remove_file(watch_kobj, &watch_address_attribute.attr);
    kobject_put(watch_kobj);

    printk(KERN_INFO "Watchpoint module unloaded\n");
}

module_init(watchpoint_module_init);
module_exit(watchpoint_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to watch a memory address");
MODULE_VERSION("1.0");

