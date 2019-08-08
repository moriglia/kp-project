#!/bin/sh

if [ $# -gt 0 ]
then
    KERNEL_DIR=`realpath $1`
fi;

cd user/
make clean
cd ../module/
if [ -z "$KERNEL_DIR" ]
then
    make clean
else
    make KERNEL_DIR=$KERNEL_DIR clean
fi;
cd ..

rm -rf append_fs/

