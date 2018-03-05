#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include "cam_os_wrapper.h"


struct proc_dir_entry *gpRootMfeDir;

static int MfeDbglvlProcShow(struct seq_file *m, void *v)
{
    seq_printf(m, "0:disbale 1: lv1 2:lv2 4:lv3 7:all\n");
    seq_printf(m, "mfedbglvl = XXX\n");
    return 0;
}

static int MfeDbglvlProcOpen(struct inode *inode, struct file *file)
{
    return single_open(file, MfeDbglvlProcShow, NULL);
}

static ssize_t MfeDbglvlProcWrite (struct file *file,
        const char __user *buffer, size_t count, loff_t *pos)
{
    char buf[] = "0x00000000\n";
    size_t len = min(sizeof(buf) - 1, count);
    unsigned long val;

    if (copy_from_user(buf, buffer, len))
        return count;
    buf[len] = 0;
    if (sscanf(buf, "%li", &val) != 1)
        printk(": %s is not in hex or decimal form.\n", buf);
    else
        printk("abc\n");

    return strnlen(buf, len);
}

static const struct file_operations MfeDbglvlProcOps = {
    .owner      = THIS_MODULE,
    .open       = MfeDbglvlProcOpen,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = MfeDbglvlProcWrite,
};

void MfeProcInit(int nDevId)
{
    struct proc_dir_entry *pde;

    gpRootMfeDir = proc_mkdir("mfe", NULL);
    if (!gpRootMfeDir)
    {
        CamOsPrintf("[MFE]can not create proc\n");
        return;
    }

    pde = proc_create("mfedbglvl", S_IRUGO, gpRootMfeDir, &MfeDbglvlProcOps);
    if (!pde)
        goto out_mfedbglvl;
    return ;

out_mfedbglvl:
    remove_proc_entry("mfedbglvl",gpRootMfeDir);
    return ;
}

void MfeProcDeInit(int nDevId)
{
    CamOsPrintf("%s %d\n",__FUNCTION__,__LINE__);
    remove_proc_entry("mfedbglvl", gpRootMfeDir);
    remove_proc_entry("mfe", NULL);
}
