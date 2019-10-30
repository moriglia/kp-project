# Kernel Programming Project

This project is a Linux Kernel Module that allows a process to wake up periodically. It exploits the miscellaneous device `/dev/kpdev` which a process can open and call `ioctl()` on.

## Build the project
```bash
./build.sh [linux-build-directory [basic-filesystem]]
```
If the `linux-build-directory` is not specified,
``/lib/modules/`uname -r`/build`` will be used.

If `basic-filsystem` is not specified, `TinyFS/tinyfs.gz` will be used.

Note that the 2 must be in order. So to specify the second,
the first must be specified as well.

## Clean the project
```bash
./clean.sh [linux-build-directory]
```
Usage is similar to the one of `./build.sh`.

## Installing and testing the module within the VM

Insert the module:
```bash
sudo ./install_module.sh
```
Test through a user task (for example, launching the `test` user program 3 times):
```bash
./test & ./test & ./test ; sleep 1
```
The `test` program opens `/dev/kpdev` device, then uses `ioctl()` to set the timeout, read the timeout back and block until the next "deadline".

`test2` program lets the user specify the timeout in milliseconds and the number of iterations to repeat some operations (printing something to the `stdout`):
```bash
./test2 [timeout-milliseconds [iteration-cunt]]
```
This program opens the `/dev/kpdev` device. Then calls `ioctl()` multiple times to set a timeout, start the timer and block. The timer will wake up the process at (almost) periodical intervals. (An overhead of about or more than 1 ms is observed).

`test_multithread_reopen` must be called without arguments and does what the other test programs do, but in multiple threads.
Since they reopen the device, the fact that they are not different processes but different threads does not affect the result.

`test_multithread_shared_fd` is almost the same as the previous test program, but the same file descriptor is used, so the distinction
of the caller of the driver functions is delegated to the driver itself. It will store the pid of the thread across different calls.

## Internal operation

### State consistency
Since multiple threads can open and invoke ioctl on `kpdev` simultaneously, a data structure
must be stored for each thread to keep the state consistency
until the next operation the thread performs on `kpdev`. Hence a structure
`struct periodic_conf` (`module/periodic.c`) is allocated
(`create_periodic_conf()`) and put in a list pointed
by the `private_data` field of the `struct file * f`. An empty list is created
every time a thread opens the device, while the periodic configuration is created 
every time a process sets his own timeout for the first time.

When the process closes the device, the saved structure must be deallocated:
this is done by `kp_dev_release()` function (`module/device.c`)
by calling `delete_periodic_conf_list()` (defined in `module/periodic.c`)
on the `private_data` pointer of the `file` structure.

### Core operation
The module exploits the high resolution timers.
The timer configuration is saved in the `periodic_conf` structure.
The timeout can be set by `periodic_conf_set_timeout_ms()` and read by
`periodic_conf_get_timeout_ms()`. This is indirectly done by calling
`ioctl(<fd>, TIMEOUT_SET, <timeout>)` and `ioctl(<fd>, TIMEOUT_GET)` respectively
from the user space application.

The function `start_cycling()` activates the timer
and sets the `hrtimer_hard_isr()` function as the callback.
This callbak will postpone the timer in order to be periodically called.
Then it checks whether the user thread is blocked.
If so the callback "completes" `unblock_ready`
(stored in `periodic_conf` structure) in order to unblock the waiting user.
The function `stop_cycling()` stops the timer.

A user thread can block until the callback "completes" `unblock_ready`
by calling `wait_for_timeout()` (`module/periodic.c`). This is indirectly done
by calling `ioctl(<file-descriptor>, BLOCK)` from a user space application.


From a user space application, the timer must be started and stopped
by calling `ioctl(<fd>, START)` and `ioctl(<fd>, STOP)` respectively.
