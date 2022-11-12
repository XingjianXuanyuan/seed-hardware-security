/* MeltdownKernel.c */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h> /* for vmalloc() */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>

static char secret[10] = {'L', 'o', 'c', 'h', 'L', 'o', 'm', 'o', 'n', 'd'};
static struct proc_dir_entry *secret_entry;
static char* secret_buffer;

static int test_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, NULL, PDE_DATA(inode));
}

/* read_proc() does not return the secret data to the user space,
 * so it does not leak the secret data.
 */
static ssize_t read_proc(struct file *fp, char *buffer, size_t length, loff_t *offset)
{
    /* load secret variable, which is cached by the CPU */
    memcpy(secret_buffer, &secret, 10);
    return 10;
}

static const struct file_operations test_proc_fops =
{
    .owner = THIS_MODULE,
    .open = test_proc_open,
    .read = read_proc,
    .llseek = seq_lseek,
    .release = single_release,
};

static int test_proc_init(void)
{
    /* write message in kernel message buffer, which is publicly accessible */
    printk(KERN_INFO "secret data address: %p\n", &secret);

    secret_buffer = (char *)vmalloc(10);

    /* create data entry: /proc/secret_data
     * When a user-level program reads from this entry,
     * the read_proc() function will be invoked,
     * inside which the secret variable will be loaded.
     */
    secret_entry = proc_create_data("secret_data", 0444, NULL, (const struct proc_ops *)&test_proc_fops, NULL);
    if (secret_entry) return 0;

    return -ENOMEM;
}

static void test_proc_cleanup(void)
{
    remove_proc_entry("secret_data", NULL);
}

module_init(test_proc_init);
module_exit(test_proc_cleanup);


MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("SEED Labs 2.0");
MODULE_DESCRIPTION ("Meltdown");