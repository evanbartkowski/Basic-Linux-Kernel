# run this script as root from kernelSpace/tests 
echo "=== RUNNING TESTS ===" 1>&2 # 1>&2 forwards to stderr
if [ "$(id -u)" != "0" ]; then
    echo "Please run this script as root" 1>&2
    exit 1
fi
echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" 1>&2
echo "# SETUP" 1>&2
echo "Clean up any previous mess..." 1>&2
rm -rf test_src
umount /tmp/memefs
rm -rf /tmp/memefs
rmmod filesystem

echo "Copy source files from userSpace into test_src/userSpace..." 1>&2
mkdir /tmp/memefs
mkdir test_src
mkdir test_src/userSpace
cp ../../userSpace/*.c ../../userSpace/*.h ../../userSpace/Makefile test_src/userSpace

echo "Copy source files from kernelSpace into test_src..." 1>&2
cp ../*.c ../*.h ../Makefile test_src
echo "# DONE" 1>&2


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" 1>&2
echo "# COMPILE USERSPACE" 1>&2
cd test_src/userSpace
make
if [ $? != 0 ]; then
    echo "# FAIL -- compilation error" 1>&2
    exit 1
fi
echo "# PASS" 1>&2


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# MOUNT FUSE" 1>&2
echo "Make new filesystem image..." 1>&2
make image
if [ $? != 0 ]; then
    echo "# FAIL -- could not make filesystem image with `make image`" 1>&2
    exit 1
fi

echo "Mount FUSE with new image..." 1>&2
./memefs --volume_label="LABEL" -o allow_other /tmp/memefs filesystem.img
if [ $? != 0 ]; then
    echo "# FAIL -- could not mount FUSE to /tmp/memefs" 1>&2
    exit 1
fi

touch /tmp/memefs/testfile
if [ $? != 0 ]; then
    echo "# FAIL -- could not create new file /tmp/memefs/testfile" 1>&2
    exit 1
fi
echo "whatever you want to say" > /tmp/memefs/testfile
if [ $? != 0 ]; then
    echo "# FAIL -- could not add text to /tmp/memefs/testfile" 1>&2
    exit 1
fi
echo "# PASS" 1>&2


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" 1>&2
echo "# COMPILE KERNELSPACE"  1>&2
cd ..
make
if [ $? != 0 ]; then
    echo "# FAIL -- compilation error" 1>&2
    exit 1
fi
echo "# PASS" 1>&2


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" 1>&2
echo "# INSERT KERNEL MODULE" 1>&2
insmod filesystem.ko filepath="/tmp/memefs/testfile"
if [ $? != 0 ]; then
    echo "# FAIL -- loading kernel module failed" 1>&2
    exit 1
fi
grep memefs /proc/devices
if [ $? != 0 ]; then
    echo "# FAIL -- device was not registered with kernel" 1>&2
    exit 1
fi
dmesg | tail
chmod 666 /dev/memefs
if [ $? != 0 ]; then
    echo "# FAIL -- could not set permissions of /dev/memefs" 1>&2
    exit 1
fi
echo "# PASS" 1>&2


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" 1>&2
echo "# TEST OUT KERNEL MODULE (W.I.P.)" 1>&2
echo "cat out /tmp/memefs/testfile..." 1>&2
cat /tmp/memefs/testfile
if [ $? != 0 ]; then
    echo "# FAIL -- cat on /tmp/memefs/testfile failed" 1>&2
    exit 1
fi
echo "cat out /dev/memefs (should match previous)..." 1>&2
cat /dev/memefs
if [ $? != 0 ]; then
    echo "# FAIL -- cat on /dev/memefs failed" 1>&2
    exit 1
fi
echo "Hello MEMEfs" > /tmp/memefs/testfile
if [ $? != 0 ]; then
    echo "# FAIL -- could not modify /tmp/memefs/testfile" 1>&2
    exit 1
fi
cat /dev/memefs
if [ $? != 0 ]; then
    echo "# FAIL -- cat on /dev/memefs after modifying testfile failed" 1>&2
    exit 1
fi
echo "# PASS" 1>&2


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" 1>&2
echo "# CLEANUP" 1>&2
rmmod filesystem
if [ $? != 0 ]; then
    echo "# FAIL -- failed to remove kernel module" 1>&2
    exit 1
fi
umount /tmp/memefs
cd ..
rm -rf test_src
echo "# DONE" 1>&2


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "    .----.   @   @"
echo "   / .-\"-.\`.  \\v/     __________________" 
echo "   | | '\\ \\ \\_/ )    <  tests complete! |"
echo " ,-\\ \`-.' /.'  /      \`-----------------'"
echo "'---\`----'----'"
