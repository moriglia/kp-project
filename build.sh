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

wait 

if [ -e module/kp.ko ] ; then 
    cp module/kp.ko append_fs/modules/ ;
else
    echo Kernel module compilation failed;
    exit 1 ;
fi

#copy user applications to user home folder
for file in user/* ; do
    if [ -x $file ] ; then
	cp $file append_fs/home/myself/
    fi
done

# create install script
echo "#!/bin/sh 
source ./.bashrc
insmod /modules/kp.ko
sleep 2
chmod ugo+rwx /dev/kpdev
" > append_fs/home/myself/install_module.sh
chmod +x append_fs/home/myself/install_module.sh

# create .bashrc
echo "#!/bin/sh
export PATH=\$PATH:/sbin
" > append_fs/home/myself/.bashrc
chmod +x append_fs/home/myself/.bashrc

# prepare fs
cd append_fs
find modules/ home/ | cpio -H newc -o | gzip > append_fs.gz
cd ..

if [ $# -gt 1 ] ; then
    initramfs=`realpath $2`;
else
    initramfs="TinyFS/tinyfs.gz";
fi

if [ -e $initramfs ] ; then
    cat $initramfs append_fs/append_fs.gz > fs.gz ;

    # boot the machine
    qemu-system-x86_64 -kernel $KERNEL_IMAGE \
		       -initrd fs.gz \
		       -accel kvm \
		       -append "console=tty consle=ttyS0" \
		       -serial file:log.txt ;
else
    echo I\'m not going to create an initramfs. ;
    echo I need the initramfs file to continue. ;
    exit 1;
fi

exit 0;
