echo "# RUNNING TESTS"
echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# SETUP"
echo "Clearing test_src directory"
umount test_src/mountpoint
rm -rf test_src
echo "Copying files from .. into test_src"
mkdir test_src
cp ../*.c ../*.h ../Makefile test_src


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# COMPILING"
echo "Running Makefile"
cd test_src && make
if [ $? != 0 ]; then
    echo "# FAIL -- compilation error"
    exit 1
fi
echo "# PASS"


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# GENERATE IMAGE FILE"
echo "Run ./mkmemefs memefs.img \"TestVolume\""
./mkmemefs filesystem.img "TestVolume"
if [ $? != 0 ]; then
    echo "# FAIL -- failed to run mkmemefs"
    exit 1
fi
if [ ! -f filesystem.img ]; then
    echo " FAIL -- cannot find memefs.img"
    exit 1
fi
echo "# PASS"


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# MOUNT DIRECTORY"
echo "Make directory 'mountpoint' to be used as mountpoint"
mkdir mountpoint
echo "Run ./memefs with some sample content, the mountpoint, and the image file"
make mount
if [ $? != 0 ]; then
    echo "# FAIL -- memefs returned nonzero"
    exit 1
fi
if mountpoint -q mountpoint && [ $? != 0 ]; then
    echo "# FAIL -- mountpoint was not made a mountpoint"
    exit 1
fi
cd mountpoint
echo "# PASS"


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# TEST TOUCH"
echo "Should make a new file 'hello'"
touch hello
if [ $? != 0 ]; then
    echo "# FAIL -- touch fail"
    exit 1
fi
echo "# PASS -> but verify file is set up well behind the scenes"


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# TEST LS"
echo "Should find file named 'hello'"
if ! ls | grep "hello"; then
    echo "# FAIL -- ls fail"
    exit 1
fi
echo "# PASS"


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# TEST ECHO APPEND"
echo "Should add text to hello"
echo 'File content goes here\n...' >> hello
if [ $? != 0 ]; then
    echo "# FAIL -- echo append fail"
    exit 1
fi
echo "# PASS"


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# TEST CAT"
echo "Should read content from the 'hello' file"
if ! cat hello | grep -F 'File content goes here\n...'; then
    echo "# FAIL - cat fail"
    exit 1
fi
echo "# PASS"


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# TEST STAT"
if ! stat hello | grep $(date +%Y); then
    echo "# FAIL - stat fail"
    exit 1
fi
echo "# PASS -> but verify that every attribute is accurate"


echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# TEST ECHO NEW"
echo "A new file with a a lot more content in it. This content could go on all day. When will the content ever end." > newfile
if ! cat newfile | grep -F "A new file with a a lot more content in it. This content could go on all day. When will the content ever end."; then
    echo "# FAIL - echo new"
    exit 1
fi
echo "# PASS"


cd ..
make unmount
echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "# ALL TESTS PASSED!"
echo "      _____         _____         _____         _____"
echo "    .'     '.     .'     '.     .'     '.     .'     '. "
echo "   /  o   o  \   /  o   o  \   /  o   o  \   /  o   o  \ "
echo "  |           | |           | |           | |           | "
echo "  |  \     /  | |  \     /  | |  \     /  | |  \     /  | "
echo "   \  '---'  /   \  '---'  /   \  '---'  /   \  '---'  / "
echo "    '._____.'     '._____.'     '._____.'     '._____.' "
echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
