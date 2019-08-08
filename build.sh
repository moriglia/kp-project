#!/bin/sh

# make module
if [ $# -gt 0 ]
then
    KERNEL_DIR=`realpath $1`
fi;

if [ -z "$KERNEL_DIR" ]
then
    cd module/
    make &> ../module.out
    cd ../user/
    make &> ../user.out
    KERNEL_IMAGE=/boot/vmlinuz-`uname -r`
else
    cd module/
    make KERNEL_DIR=$KERNEL_DIR &> ../module.out
    cd ../user/
    make KERNEL_INCLUDE=$KERNEL_DIR/include &> ../user.out
    KERNEL_IMAGE=$KERNEL_DIR/arch/x86_64/boot/bzImage
fi;
cd ..

# copy files to dummy fs
mkdir -p append_fs/modules
mkdir -p append_fs/home/myself

cp module/kp.ko append_fs/modules/
cp user/test append_fs/home/myself/

# create install script
echo "#!/bin/sh" > append_fs/home/myself/install_module.sh
echo "source ./.bashrc" >> append_fs/home/myself/install_module.sh
echo "insmod /modules/kp.ko" >> append_fs/home/myself/install_module.sh
chmod +x append_fs/home/myself/install_module.sh
echo "#!/bin/sh" >> append_fs/home/myself/.bashrc
echo "export PATH=\$PATH:/sbin" >> append_fs/home/myself/.bashrc
chmod +x append_fs/home/myself/.bashrc

# prepare fs
cd append_fs
find modules/ home/ | cpio -H newc -o | gzip > append_fs.gz
cd ..
cat TinyFS/tinyfs.gz append_fs/append_fs.gz > fs.gz

# boot the machine
qemu-system-x86_64 -kernel $KERNEL_IMAGE \
		   -initrd fs.gz \
		   -accel kvm \
		   -append "console=tty consle=ttyS0" \
		   -serial file:log.txt


