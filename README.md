# GemDroid: QEMU Part

## System Requirement

AOSP [android-11.0.0_r10](https://source.android.com/setup/build/downloading)

## To Patch

Assume that $AOSP is your aosp folder, run the following commands to run an android emulator for
GemDroid.

    cd $AOSP
    git clone https://github.com/GemDroidResearch/GemDroid_QEMU.git
    cd $AOSP/sdk/emulator
    git checkout -b gemdroid cbf40c
    cd $AOSP/external
    cp -r $AOSP/GemDroid_QEMU/GemDroid_QEMU qemu
    cd qemu
    ./android-configure.sh
    make 
    #use emulators in objs/
