# Kernel Programming Project

This project is a Linux Kernel Module that allows a process to wake up periodically. It exploits the miscellaneous device `/dev/kpdev` which a process can open and call `ioctl()` on.

## Build the project
```bash
./build.sh [linux-build-directory]
```
If the `linux-build-directory` is not specified ``/lib/modules/`uname -r`/build`` will be used.

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

`test2` program lets the user specify the timeout in millisecond and the number of iterations to repeat some operations (printing something to the `stdout`):
```bash
./test2 [timeout-milliseconds [iteration-cunt]]
```
This program opens the `/dev/kpdev` device. Then calls `ioctl()` multiple times to set a timeout, start the timer and block. The timer will wake up the process at (almost) periodical intervals. (An overhead of about or more then 1 ms is observed).

## Internal operation

### State consistency
Since multiple processes can open `kpdev` simultaneously, a data structure
must be stored for each process to keep the state consistency
until the next operation the process performs on `kpdev`. Hence a structure
`struct periodic_conf` (`module/periodic.c`) is allocated
(`create_periodic_conf()`) and pointed to
by the `private_data` field of the `struct file * f`. It is created
every time a process opens the device
(function `int kp_dev_open(struct inode *in, struct file *f)`in `module/device.c`).

When the process closes the device, the saved structure must be deallocated:
this is done by `kp_dev_release()` function (`module/device.c`)
by calling `delete_periodic_conf()` (defined in `module/periodic.c`)
on the `private_data` pointer of the `file` structure.

### Core operation
For each process a kthread is started. The code of the kthread is the function
`callback_helper_thread()`. It can be started
by `create_thread(struct periodic_conf *pc)`. This is done every time a process
opens `kpdev`.

This kthread cyclically blocks until the timer callback
"completes" `timer_elapsed` (stored in `periodic_conf` structure).
Then checks whether the user thread is blocked.
If so the kthread "completes" `unblock_ready`
(stored in `periodic_conf` structure) in order to unblock it.

A user thread can block until the kthread "completes" `unblock_ready`
by calling `wait_for_timeout()` (`module/periodic.c`). This is indirectly done
by calling `ioctl(<file-descriptor>, BLOCK)` from a user space application.

The timer configuration is saved in the `periodic_conf` structure.
The timeout can be set by `periodic_conf_set_timeout()` and read by
`periodic_conf_get_timeout()`. This is indirectly done by calling
`ioctl(<fd>, TIMEOUT_SET, <timeout>)` and `ioctl(<fd>, TIMEOUT_GET)` respectively
from the user space application.

The function `start_cycling()` activates the timer
and sets the `quick_periodic_callback()` function as the callback.
This callbak will postpone the timer in order to be periodically called.
It will then "complete" `timer_elapsed` so that the helper thread
`callback_helper_thread()` can possibly unblock the user process in turn,
as described above. The function `stop_cycling()` stops the timer.

From a user space application, the timer must be started and stopped
by calling `ioctl(<fd>, START)` and `ioctl(<fd>, STOP)` respectively.