import os, sys, math, glob
android_mk_files = [
"./Android.mk",
"./distrib/googletest/Android.mk",
"./distrib/jpeg-6b/Android.mk",
"./distrib/ext4_utils/src/Android.mk",
"./distrib/android-emugl/Android.mk",
"./distrib/android-emugl/shared/OpenglCodecCommon/Android.mk",
"./distrib/android-emugl/shared/emugl/common/Android.mk",
"./distrib/android-emugl/host/libs/Translator/EGL/Android.mk",
"./distrib/android-emugl/host/libs/Translator/GLES_CM/Android.mk",
"./distrib/android-emugl/host/libs/Translator/GLES_V2/Android.mk",
"./distrib/android-emugl/host/libs/Translator/GLcommon/Android.mk",
"./distrib/android-emugl/host/libs/libOpenglRender/Android.mk",
"./distrib/android-emugl/host/libs/GLESv1_dec/Android.mk",
"./distrib/android-emugl/host/libs/GLESv2_dec/Android.mk",
"./distrib/android-emugl/host/libs/renderControl_dec/Android.mk",
"./distrib/android-emugl/host/tools/emugen/Android.mk"
]
for xx in android_mk_files:
#	print("mv",xx.replace("Android.mk", "Prasdroid.mk"),xx)
	print("mv",xx,xx.replace("Android.mk", "Prasdroid.mk"))
