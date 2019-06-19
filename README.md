# GemDroid: QEMU Part

## System Requirement

AOSP android-7.1.2_r36

## To Patch

Assume that $AOSP is your aosp folder, run the following commands to run an android emulator for
GemDroid.

    cd $AOSP
    git clone https://github.com/GemDroidResearch/GemDroid_QEMU_Android7.git
    cd $AOSP/sdk/emulator
    git checkout -b gemdroid cbf40c
    cd $AOSP/external
    cp -r $AOSP/GemDroid_QEMU_Android7/GemDroid_QEMU qemu
    cd qemu
    ./android-configure.sh
    make 
    #use emulators in objs/
