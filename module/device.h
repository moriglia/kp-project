#ifndef   __KP_DEVICE__
#define   __KP_DEVICE__

#include <linux/ioctl.h>

#define TIMEOUT_SET _IOW('k', 's', unsigned long )
#define TIMEOUT_GET _IO ('k', 'g')
#define BLOCK       _IO ('k', 'b')
#define START       _IO ('k', 't')
#define STOP        _IO ('k', 'p')

/* Error codes */
#define ERR_GENERIC                        1
#define ERR_NO_KMALLOC                     2
#define ERR_TIMEOUT_NOT_SET                3
#define ERR_UNKNOWN_IOCTL_COMMAND          4
#define ERR_RUNNING_STATE                  5
#define ERR_INVALID_TIMEOUT                6

/* Registration/deregistration functions */
int  kp_device_create  (void);
void kp_device_destroy (void);


#endif /* __KP_DEVICE__*/
