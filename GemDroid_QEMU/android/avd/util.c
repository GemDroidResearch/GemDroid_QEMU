/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "android/utils/debug.h"
#include "android/utils/bufprint.h"
#include "android/utils/ini.h"
#include "android/utils/property_file.h"
#include "android/utils/panic.h"
#include "android/utils/path.h"
#include "android/utils/system.h"
#include "android/avd/util.h"
#include "android/avd/keys.h"

#define D(...) VERBOSE_PRINT(init,__VA_ARGS__)

/* Return the path to the Android SDK root installation.
 *
 * (*pFromEnv) will be set to 1 if it comes from the $ANDROID_SDK_ROOT
 * environment variable, or 0 otherwise.
 *
 * Caller must free() returned string.
 */
char*
path_getSdkRoot( char *pFromEnv )
{
    const char*  env;
    char*        sdkPath;
    char         temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

    /* If ANDROID_SDK_ROOT is defined is must point to a directory
     * containing a valid SDK installation.
     */
#define  SDK_ROOT_ENV  "ANDROID_SDK_ROOT"

    env = getenv(SDK_ROOT_ENV);
    if (env != NULL && env[0] != 0) {
        if (path_exists(env)) {
            D("found " SDK_ROOT_ENV ": %s", env);
            *pFromEnv = 1;
            return ASTRDUP(env);
        }
        D(SDK_ROOT_ENV " points to unknown directory: %s", env);
    }

    *pFromEnv = 0;

    /* We assume the emulator binary is under tools/ so use its
     * parent as the Android SDK root.
     */
    (void) bufprint_app_dir(temp, end);
    sdkPath = path_parent(temp, 1);
    if (sdkPath == NULL) {
        derror("can't find root of SDK directory");
        return NULL;
    }
    D("found SDK root at %s", sdkPath);
    return sdkPath;
}


/* Return the path to the AVD's root configuration .ini file. it is located in
 * ~/.android/avd/<name>.ini or Windows equivalent
 *
 * This file contains the path to the AVD's content directory, which
 * includes its own config.ini.
 */
char*
path_getRootIniPath( const char*  avdName )
{
    char temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);

    p = bufprint_avd_home_path(temp, end);
    p = bufprint(p, end, PATH_SEP "%s.ini", avdName);
    if (p >= end) {
        return NULL;
    }
    if (!path_exists(temp)) {
        return NULL;
    }
    return ASTRDUP(temp);
}

static char*
_getAvdContentPath(const char* avdName)
{
    char temp[PATH_MAX], *p=temp, *end=p+sizeof(temp);
    IniFile* ini = NULL;
    char*    iniPath = path_getRootIniPath(avdName);
    char*    avdPath = NULL;

    if (iniPath != NULL) {
        ini = iniFile_newFromFile(iniPath);
        if (ini == NULL) {
            APANIC("Could not parse file: %s\n", iniPath);
        }
        AFREE(iniPath);
    } else {
        static const char kHomeSearchDir[] = "$HOME" PATH_SEP ".android" PATH_SEP "avd";
        static const char kSdkHomeSearchDir[] = "$ANDROID_SDK_HOME" PATH_SEP ".android"
            PATH_SEP "avd";
        const char* envName = "HOME";
        const char* searchDir = kHomeSearchDir;
        if (getenv("ANDROID_AVD_HOME")) {
            envName = "ANDROID_AVD_HOME";
            searchDir = "$ANDROID_AVD_HOME";
        } else if (getenv("ANDROID_SDK_HOME")) {
            envName = "ANDROID_SDK_HOME";
            searchDir = kSdkHomeSearchDir;
        }
        APANIC("%s is defined but could not find %s.ini file in %s\n"
                "(Note: avd is searched in the order of $ANDROID_AVD_HOME,"
                "%s and %s)\n",
                envName, avdName, searchDir, kSdkHomeSearchDir, kHomeSearchDir);
    }

    avdPath = iniFile_getString(ini, ROOT_ABS_PATH_KEY, NULL);

    if (!path_is_dir(avdPath)) {
        // If the absolute path doesn't match an actual directory, try
        // the relative path if present.
        const char* relPath = iniFile_getString(ini, ROOT_REL_PATH_KEY, NULL);
        if (relPath != NULL) {
            p = bufprint_config_path(temp, end);
            p = bufprint(p, end, PATH_SEP "%s", relPath);
            if (p < end && path_is_dir(temp)) {
                AFREE(avdPath);
                avdPath = ASTRDUP(temp);
            }
        }
    }

    iniFile_free(ini);

    return avdPath;
}

char*
propertyFile_getTargetAbi(const FileData* data) {
    return propertyFile_getValue((const char*)data->data,
                                 data->size,
                                 "ro.product.cpu.abi");
}


char*
propertyFile_getTargetArch(const FileData* data) {
    char* ret = propertyFile_getTargetAbi(data);
    if (ret) {
        // Translate ABI name into architecture name.
        // By default, there are the same with a few exceptions.
        static const struct {
            const char* input;
            const char* output;
        } kData[] = {
            { "armeabi", "arm" },
            { "armeabi-v7a", "arm" },
            { "arm64-v8a", "arm64" },
        };
        size_t n;
        for (n = 0; n < sizeof(kData)/sizeof(kData[0]); ++n) {
            if (!strcmp(ret, kData[n].input)) {
                free(ret);
                ret = ASTRDUP(kData[n].output);
                break;
            }
        }
    }
    return ret;
}


int
propertyFile_getInt(const FileData* data, const char* key, int _default,
                    SearchResult* searchResult) {
    char* prop = propertyFile_getValue((const char*)data->data,
                                       data->size,
                                       key);
    if (!prop) {
        if (searchResult) {
            *searchResult = RESULT_NOT_FOUND;
        }
        return _default;
    }

    char* end;
    // long is only 32 bits on windows so it isn't enough to detect int overflow
    long long val = strtoll(prop, &end, 10);
    if (val < INT_MIN || val > INT_MAX ||
        end == prop || *end != '\0') {
        D("Invalid int property: '%s:%s'", key, prop);
        AFREE(prop);
        if (searchResult) {
            *searchResult = RESULT_INVALID;
        }
        return _default;
    }

    AFREE(prop);

    if (searchResult) {
        *searchResult = RESULT_FOUND;
    }
    return (int)val;
}

int
propertyFile_getApiLevel(const FileData* data) {
    const int kMinLevel = 3;
    const int kMaxLevel = 10000;
    SearchResult searchResult;
    int level = propertyFile_getInt(data, "ro.build.version.sdk", kMinLevel,
                                    &searchResult);
    if (searchResult == RESULT_NOT_FOUND) {
        level = kMaxLevel;
        D("Could not find SDK version in build.prop, default is: %d", level);
    } else if (searchResult == RESULT_INVALID || level < 0) {
        D("Defaulting to target API sdkVersion %d", level);
    } else {
        D("Found target API sdkVersion: %d\n", level);
    }
    return level;
}

int
propertyFile_getAdbdCommunicationMode(const FileData* data) {
    // adb sporadically hangs when using a pipe to communicate with qemud, so
    // disable the qemud pipe.
    // TODO: Fix the hang with qemud and change back to reading the
    // communication method from the ro.adb.qemud build property.
    D("Forcing ro.adb.qemud to \"0\".");
    return 0;
}

char* path_getBuildBuildProp(const char* androidOut) {
    char temp[MAX_PATH], *p = temp, *end = p + sizeof(temp);
    p = bufprint(temp, end, "%s/system/build.prop", androidOut);
    if (p >= end) {
        D("ANDROID_BUILD_OUT is too long: %s\n", androidOut);
        return NULL;
    }
    if (!path_exists(temp)) {
        D("Cannot find build properties file: %s\n", temp);
        return NULL;
    }
    return ASTRDUP(temp);
}


char* path_getBuildBootProp(const char* androidOut) {
    char temp[MAX_PATH], *p = temp, *end = p + sizeof(temp);
    p = bufprint(temp, end, "%s/boot.prop", androidOut);
    if (p >= end) {
        D("ANDROID_BUILD_OUT is too long: %s\n", androidOut);
        return NULL;
    }
    if (!path_exists(temp)) {
        D("Cannot find boot properties file: %s\n", temp);
        return NULL;
    }
    return ASTRDUP(temp);
}


char*
path_getBuildTargetArch(const char* androidOut) {
    char* buildPropPath = path_getBuildBuildProp(androidOut);
    if (!buildPropPath) {
        return NULL;
    }

    FileData buildProp[1];
    fileData_initFromFile(buildProp, buildPropPath);
    char* ret = propertyFile_getTargetArch(buildProp);
    fileData_done(buildProp);
    AFREE(buildPropPath);
    return ret;
}


static char*
_getAvdConfigValue(const char* avdPath,
                   const char* key,
                   const char* defaultValue)
{
    IniFile* ini;
    char* result = NULL;
    char temp[PATH_MAX], *p = temp, *end = p + sizeof(temp);
    p = bufprint(temp, end, "%s" PATH_SEP "config.ini", avdPath);
    if (p >= end) {
        APANIC("AVD path too long: %s\n", avdPath);
    }
    ini = iniFile_newFromFile(temp);
    if (ini == NULL) {
        APANIC("Could not open AVD config file: %s\n", temp);
    }
    result = iniFile_getString(ini, key, defaultValue);
    iniFile_free(ini);

    return result;
}

char*
path_getAvdTargetArch( const char* avdName )
{
    char*  avdPath = _getAvdContentPath(avdName);
    char*  avdArch = _getAvdConfigValue(avdPath, "hw.cpu.arch", "arm");
    AFREE(avdPath);

    return avdArch;
}

char*
path_getAvdGpuMode(const char* avdName)
{
    char* avdPath = _getAvdContentPath(avdName);
    char* gpuEnabled = _getAvdConfigValue(avdPath, "hw.gpu.enabled", "no");
    bool enabled = !strcmp(gpuEnabled, "yes");
    AFREE(gpuEnabled);

    char* gpuMode = NULL;
    if (enabled) {
        gpuMode = _getAvdConfigValue(avdPath, "hw.gpu.mode", "auto");
    }
    AFREE(avdPath);
    return gpuMode;
}

const char*
emulator_getBackendSuffix(const char* targetArch)
{
    if (!targetArch)
        return NULL;

    static const struct {
        const char* avd_arch;
        const char* emulator_suffix;
    } kPairs[] = {
        { "arm", "arm" },
        { "x86", "x86" },
        { "x86_64", "x86" },
        { "mips", "mips" },
        { "arm64", "arm64" },
        { "mips64", "mips64" },
        // Add more if needed here.
    };
    size_t n;
    for (n = 0; n < sizeof(kPairs)/sizeof(kPairs[0]); ++n) {
        if (!strcmp(targetArch, kPairs[n].avd_arch)) {
            return kPairs[n].emulator_suffix;
        }
    }
    return NULL;
}
