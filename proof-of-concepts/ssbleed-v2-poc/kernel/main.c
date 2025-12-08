/*
 * Copyright (c) 2025 Hongpei Zheng
 * SPDX-License-Identifier: Apache-2.0
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/cdev.h>

#if !defined(__aarch64__)
#error Module can only be compiled on ARM64 machines.
#endif

extern void stld_kernel(void* st_addr, void* ld_addr);

static void call_stld(void* dummy) {
    size_t A[10];
    stld_kernel(&A[0], &A[8]);
}

#define DEV_NAME "mdu_kernel"
#define DEV_MAJOR 223
#define DEV_MINOR 0

dev_t msrdrv_dev;
struct cdev *msrdrv_cdev;
struct ioctl_param_t {
	int cpu;
    size_t res;
};

static long msrdrv_ioctl(struct file *f, unsigned int cmd, unsigned long ioctl_param) {
    struct ioctl_param_t* param = vmalloc(sizeof(struct ioctl_param_t));
    int res;
    res = copy_from_user(param, (struct ioctl_param_t*)ioctl_param, sizeof(struct ioctl_param_t));
    // printk(KERN_INFO "inctol_param = {%d, %d}", param->cpu, param->idx);
    if (cmd == 0) {
        smp_call_function_single(param->cpu, call_stld, NULL, 1);
    }
    vfree(param);
    return 0;
}

struct file_operations msrdrv_fops = {
    .owner =          THIS_MODULE,
    .read =           NULL,
    .write =          NULL,
    .open =           NULL,
    .release =        NULL,
    .unlocked_ioctl = msrdrv_ioctl,
    .compat_ioctl =   NULL,
};

static int __init init(void) {
    // register device
    msrdrv_dev = MKDEV(DEV_MAJOR, DEV_MINOR);
    register_chrdev_region(msrdrv_dev, 1, DEV_NAME);
    msrdrv_cdev = cdev_alloc();
    msrdrv_cdev->owner = THIS_MODULE;
    msrdrv_cdev->ops = &msrdrv_fops;
    cdev_init(msrdrv_cdev, &msrdrv_fops);
    cdev_add(msrdrv_cdev, msrdrv_dev, 1);
    printk(KERN_INFO "%lx\n", stld_kernel);
    return 0;
}

static void __exit fini(void) {
    // unregsiter device
    cdev_del(msrdrv_cdev);
    unregister_chrdev_region(msrdrv_dev, 1);
}

MODULE_DESCRIPTION("Showing MDU vulnerability against kernel on Arm-v9.");
MODULE_LICENSE("GPL");
module_init(init);
module_exit(fini);