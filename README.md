# Kernel Programming Project

## Build the project
```bash
./build.sh [linux-build-directory]
```
If the `linux-build-directory` is not specified ``/lib/modules/`uname -r`/build`` will be used.

## Clean project
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
The `test` program uses `ioctl()` to set the timeout, read the timeout back and sleep for the given timeout.