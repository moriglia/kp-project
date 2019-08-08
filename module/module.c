#include <linux/module.h>
#include <linux/init.h>

#include "device.h"

MODULE_AUTHOR("Marco Origlia");
MODULE_DESCRIPTION("Misc device used as a timer");
MODULE_LICENSE("GPL");



static int __init kp_module_init(void){
  return kp_device_create();
}

void __exit kp_module_exit(void){
  kp_device_destroy() ;
  return ;
}


module_init(kp_module_init);
module_exit(kp_module_exit);
