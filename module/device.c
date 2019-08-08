#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include "device.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif



static struct miscdevice kp_device;

static int kp_dev_open(struct inode* in, struct file* f){
  char buf[1024];
#if VERBOSE
  printk("Opening device");
#endif
  sprintf(buf, "File private data is %p\n", f->private_data);
  printk(buf);
  f->private_data = NULL;
  return 0;
}

static int kp_dev_release(struct inode* in, struct file* f){
#if VERBOSE
  printk("Closing device");
#endif
  /*
  if ( f->private_data != NULL ){
    kfree( f->private_data );
  }
  */
  return 0;
}

static long kp_dev_ioctl(struct file* f, unsigned int cmd, unsigned long arg){
  void *timeout;

  switch (cmd){

  case WR_VALUE:
    if (f->private_data) {
      timeout = f->private_data;
    } else {
      timeout = kmalloc(sizeof(unsigned int), GFP_USER);
      if (!timeout) {
	return -ERR_NO_KMALLOC;
      }
      f->private_data = timeout;
    }
    *((unsigned long *)timeout) = arg;
    return 0;

  case RD_VALUE:
    if (!f->private_data) {
      return -ERR_TIMEOUT_NOT_SET;
    }
    timeout = f->private_data;
    return *((unsigned long*)timeout);

  default:
    return -ERR_UNKNOWN_IOCTL_COMMAND;
  }
}

static struct file_operations kp_dev_file_operations = {
  .owner            = THIS_MODULE,
  .unlocked_ioctl   = kp_dev_ioctl,
  .open             = kp_dev_open,
  .release          = kp_dev_release
};

static struct miscdevice kp_device = {
  MISC_DYNAMIC_MINOR,
  "kpdev",
  &kp_dev_file_operations
};


/* Registration/deregistration functions */

int kp_device_create(void){
  return misc_register(&kp_device);
}

void kp_device_destroy(void){
  misc_deregister(&kp_device);
  return ;
}
