/* Copyright (C) 2006-2008 The Android Open Source Project
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

#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#ifdef CONFIG_POSIX
#include <pthread.h>
#endif
#ifdef _WIN32
#include <process.h>
#endif

#if defined(__APPLE__) && defined(CONFIG_SDL)
// This include is currently required to ensure that 'main' is renamed
// to 'SDL_main' as a macro. This is required by SDL 1.x on OS X.
#include "SDL.h"
#endif  // __APPLE__

#include "config.h"
#include "android/sockets.h"

#include "android/android.h"
#include "qemu-common.h"
#include "sysemu/sysemu.h"
#include "ui/console.h"
#include "android/user-events.h"

#include "math.h"

#include "android/config/config.h"

#include "android/kernel/kernel_utils.h"
#include "android/skin/charmap.h"
#include "android/user-config.h"

#include "android/utils/aconfig-file.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/filelock.h"
#include "android/utils/lineinput.h"
#include "android/utils/path.h"
#include "android/utils/property_file.h"
#include "android/utils/tempfile.h"

#include "android/main-common.h"
#include "android/help.h"
#include "hw/android/goldfish/nand.h"

#include "android/globals.h"

#include "android/display.h"

#include "android/snapshot.h"

#include "android/framebuffer.h"
#include "android/opengl/emugl_config.h"
#include "android/iolooper.h"

#include "android/skin/winsys.h"

SkinRotation  android_framebuffer_rotation;

#define  STRINGIFY(x)   _STRINGIFY(x)
#define  _STRINGIFY(x)  #x

#ifdef ANDROID_SDK_TOOLS_REVISION
#  define  VERSION_STRING  STRINGIFY(ANDROID_SDK_TOOLS_REVISION)".0"
#else
#  define  VERSION_STRING  "standalone"
#endif

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

extern int  control_console_start( int  port );  /* in control.c */

extern int qemu_milli_needed;

/* the default device DPI if none is specified by the skin
 */
#define  DEFAULT_DEVICE_DPI  165

int qemu_main(int argc, char **argv);

/* this function dumps the QEMU help */
extern void  help( void );
extern void  emulator_help( void );

#define  VERBOSE_OPT(str,var)   { str, &var }

#define  _VERBOSE_TAG(x,y)   { #x, VERBOSE_##x, y },

void emulator_help( void )
{
    STRALLOC_DEFINE(out);
    android_help_main(out);
    printf( "%.*s", out->n, out->s );
    stralloc_reset(out);
    exit(1);
}

/*
 * _findQemuInformationalOption: search for informational QEMU options
 *
 * Scans the given command-line options for any informational QEMU option (see
 * |qemu_info_opts| for the list of informational QEMU options). Returns the
 * first matching option, or NULL if no match is found.
 *
 * |qemu_argc| is the number of command-line options in |qemu_argv|.
 * |qemu_argv| is the array of command-line options to be searched. It is the
 * caller's responsibility to ensure that all these options are intended for
 * QEMU.
 */
static char*
_findQemuInformationalOption( int qemu_argc, char** qemu_argv )
{
    /* Informational QEMU options, which make QEMU print some information to the
     * console and exit. */
    static const char* const qemu_info_opts[] = {
        "-h",
        "-help",
        "-version",
        "-audio-help",
        "?",           /* e.g. '-cpu ?' for listing available CPU models */
        NULL           /* denotes the end of the list */
    };
    int i = 0;

    for (; i < qemu_argc; i++) {
        char* arg = qemu_argv[i];
        const char* const* oo = qemu_info_opts;

        for (; *oo; oo++) {
            if (!strcmp(*oo, arg)) {
                return arg;
            }
        }
    }
    return NULL;
}

void enter_qemu_main_loop(int argc, char **argv) {
#ifndef _WIN32
    sigset_t set;
    sigemptyset(&set);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
#endif

    D("Starting QEMU main loop");
    qemu_main(argc, argv);
    D("Done with QEMU main loop");
    exit(0);
}

#if CONFIG_QT && defined(_WIN32)
// On Windows, link against qtmain.lib which provides a WinMain()
// implementation, that latter calls qMain().
#define main qt_main
#endif

int main(int argc, char **argv) {
    char   tmp[MAX_PATH];
    char*  args[128];
    int    n;
    char*  opt;
    char*  qemu_info_opt;

    /* The emulator always uses the first serial port for kernel messages
     * and the second one for qemud. So start at the third if we need one
     * for logcat or 'shell'
     */
    int    serial = 2;
    int    shell_serial = 0;

    AndroidHwConfig*  hw;
    AvdInfo*          avd;
    AConfig*          skinConfig;
    char*             skinPath;
    int               inAndroidBuild;

    AndroidOptions  opts[1];
    /* net.shared_net_ip boot property value. */
    char boot_prop_ip[64];
    boot_prop_ip[0] = '\0';

    args[0] = argv[0];

    if ( android_parse_options( &argc, &argv, opts ) < 0 ) {
        exit(1);
    }

#ifdef _WIN32
    socket_init();
#endif

    while (argc-- > 1) {
        opt = (++argv)[0];

        if(!strcmp(opt, "-qemu")) {
            argc--;
            argv++;
            break;
        }

        if (!strcmp(opt, "-help")) {
            emulator_help();
        }

        if (!strncmp(opt, "-help-",6)) {
            STRALLOC_DEFINE(out);
            opt += 6;

            if (!strcmp(opt, "all")) {
                android_help_all(out);
            }
            else if (android_help_for_option(opt, out) == 0) {
                /* ok */
            }
            else if (android_help_for_topic(opt, out) == 0) {
                /* ok */
            }
            if (out->n > 0) {
                printf("\n%.*s", out->n, out->s);
                exit(0);
            }

            fprintf(stderr, "unknown option: -help-%s\n", opt);
            fprintf(stderr, "please use -help for a list of valid topics\n");
            exit(1);
        }

        if (opt[0] == '-') {
            fprintf(stderr, "unknown option: %s\n", opt);
            fprintf(stderr, "please use -help for a list of valid options\n");
            exit(1);
        }

        fprintf(stderr, "invalid command-line parameter: %s.\n", opt);
        fprintf(stderr, "Hint: use '@foo' to launch a virtual device named 'foo'.\n");
        fprintf(stderr, "please use -help for more information\n");
        exit(1);
    }

    if (opts->version) {
        printf("Android emulator version %s\n"
               "Copyright (C) 2006-2011 The Android Open Source Project and many others.\n"
               "This program is a derivative of the QEMU CPU emulator (www.qemu.org).\n\n",
#if defined ANDROID_BUILD_ID
               VERSION_STRING " (build_id " STRINGIFY(ANDROID_BUILD_ID) ")" );
#else
               VERSION_STRING);
#endif
        printf("  This software is licensed under the terms of the GNU General Public\n"
               "  License version 2, as published by the Free Software Foundation, and\n"
               "  may be copied, distributed, and modified under those terms.\n\n"
               "  This program is distributed in the hope that it will be useful,\n"
               "  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
               "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
               "  GNU General Public License for more details.\n\n");

        exit(0);
    }

    if (opts->snapshot_list) {
        if (opts->snapstorage == NULL) {
            /* Need to find the default snapstorage */
            avd = createAVD(opts, &inAndroidBuild);
            opts->snapstorage = avdInfo_getSnapStoragePath(avd);
            if (opts->snapstorage != NULL) {
                D("autoconfig: -snapstorage %s", opts->snapstorage);
            } else {
                if (inAndroidBuild) {
                    derror("You must use the -snapstorage <file> option to specify a snapshot storage file!\n");
                } else {
                    derror("This AVD doesn't have snapshotting enabled!\n");
                }
                exit(1);
            }
        }
        snapshot_print_and_exit(opts->snapstorage);
    }

    /* Both |argc| and |argv| have been modified by the big while loop above:
     * |argc| should now be the number of options after '-qemu', and if that is
     * positive, |argv| should point to the first option following '-qemu'.
     * Now we check if any of these QEMU options is an 'informational' option,
     * e.g. '-h', '-version', etc.
     * The extra pair of parentheses is to keep gcc happy.
     */
    if ((qemu_info_opt = _findQemuInformationalOption(argc, argv))) {
        D("Found informational option '%s' after '-qemu'.\n"
          "All options before '-qemu' will be ignored!", qemu_info_opt);

        /* Copy all QEMU options to |args|, and set |n| to the number of options
         * in |args| (|argc| must be positive here). */
        n = 1;
        do {
            args[n] = argv[n - 1];
        } while (n++ < argc);
        args[n] = NULL;

        /* Skip the translation of command-line options and jump straight to
         * qemu_main(). */
        enter_qemu_main_loop(n, args);
        return 0;
    }

    sanitizeOptions(opts);

    /* Initialization of UI started with -attach-core should work differently
     * than initialization of UI that starts the core. In particular....
     */

    /* -charmap is incompatible with -attach-core, because particular
     * charmap gets set up in the running core. */
    if (skin_charmap_setup(opts->charmap)) {
        exit(1);
    }

    /* Parses options and builds an appropriate AVD. */
    avd = android_avdInfo = createAVD(opts, &inAndroidBuild);

    /* get the skin from the virtual device configuration */
    if (opts->skindir != NULL) {
        if (opts->skin == NULL) {
            /* NOTE: Normally handled by sanitizeOptions(), just be safe */
            derror("The -skindir <path> option requires a -skin <name> option");
            exit(2);
        }
    } else {
        char* skinName;
        char* skinDir;

        avdInfo_getSkinInfo(avd, &skinName, &skinDir);

        if (opts->skin == NULL) {
            opts->skin = skinName;
            D("autoconfig: -skin %s", opts->skin);
        } else {
            AFREE(skinName);
        }

        opts->skindir = skinDir;
        D("autoconfig: -skindir %s", opts->skindir);
    }
    /* update the avd hw config from this new skin */
    avdInfo_getSkinHardwareIni(avd, opts->skin, opts->skindir);

    if (opts->dynamic_skin == 0) {
        opts->dynamic_skin = avdInfo_shouldUseDynamicSkin(avd);
    }

    /* Read hardware configuration */
    hw = android_hw;
    if (avdInfo_initHwConfig(avd, hw) < 0) {
        derror("could not read hardware configuration ?");
        exit(1);
    }

    SkinKeyset* keyset = NULL;
    if (opts->keyset) {
        parse_keyset(opts->keyset, opts);
        keyset = skin_keyset_get_default();
        if (!keyset) {
            fprintf(stderr,
                    "emulator: WARNING: could not find keyset file named '%s',"
                    " using defaults instead\n",
                    opts->keyset);
        }
    }
    if (!keyset) {
        parse_keyset("default", opts);
        keyset = skin_keyset_get_default();
        if (!keyset) {
            keyset = skin_keyset_new_from_text(
                    skin_keyset_get_default_text());
            if (!keyset) {
                fprintf(stderr, "PANIC: default keyset file is corrupted !!\n" );
                fprintf(stderr, "PANIC: please update the code in android/skin/keyset.c\n" );
                exit(1);
            }
            skin_keyset_set_default(keyset);
            if (!opts->keyset)
                write_default_keyset();
        }
    }

    if (opts->shared_net_id) {
        char*  end;
        long   shared_net_id = strtol(opts->shared_net_id, &end, 0);
        if (end == NULL || *end || shared_net_id < 1 || shared_net_id > 255) {
            fprintf(stderr, "option -shared-net-id must be an integer between 1 and 255\n");
            exit(1);
        }
        snprintf(boot_prop_ip, sizeof(boot_prop_ip),
                 "net.shared_net_ip=10.1.2.%ld", shared_net_id);
    }


    user_config_init();
    parse_skin_files(opts->skindir, opts->skin, opts, hw,
                     &skinConfig, &skinPath);

    if (!opts->netspeed && skin_network_speed) {
        D("skin network speed: '%s'", skin_network_speed);
        if (strcmp(skin_network_speed, NETWORK_SPEED_DEFAULT) != 0) {
            opts->netspeed = (char*)skin_network_speed;
        }
    }
    if (!opts->netdelay && skin_network_delay) {
        D("skin network delay: '%s'", skin_network_delay);
        if (strcmp(skin_network_delay, NETWORK_DELAY_DEFAULT) != 0) {
            opts->netdelay = (char*)skin_network_delay;
        }
    }

    if (opts->code_profile) {
        char*   profilePath = avdInfo_getCodeProfilePath(avd, opts->code_profile);
        int     ret;

        if (profilePath == NULL) {
            derror( "bad -code-profile parameter" );
            exit(1);
        }
        ret = path_mkdir_if_needed( profilePath, 0755 );
        if (ret < 0) {
            fprintf(stderr, "could not create directory '%s'\n", tmp);
            exit(2);
        }
        opts->code_profile = profilePath;
    }

    /* Update CPU architecture for HW configs created from build dir. */
    if (inAndroidBuild) {
#if defined(TARGET_ARM)
        reassign_string(&android_hw->hw_cpu_arch, "arm");
#elif defined(TARGET_I386)
        reassign_string(&android_hw->hw_cpu_arch, "x86");
#elif defined(TARGET_MIPS)
        reassign_string(&android_hw->hw_cpu_arch, "mips");
#elif defined(TARGET_MIPS64)
        reassign_string(&android_hw->hw_cpu_arch, "mips64");
#endif
    }

    handleCommonEmulatorOptions(opts, hw, avd);

    n = 1;

    if (boot_prop_ip[0]) {
        args[n++] = "-boot-property";
        args[n++] = boot_prop_ip;
    }

    if (opts->tcpdump) {
        args[n++] = "-tcpdump";
        args[n++] = opts->tcpdump;
    }

#ifdef CONFIG_NAND_LIMITS
    if (opts->nand_limits) {
        args[n++] = "-nand-limits";
        args[n++] = opts->nand_limits;
    }
#endif

    if (opts->timezone) {
        args[n++] = "-timezone";
        args[n++] = opts->timezone;
    }

    if (opts->netspeed) {
        args[n++] = "-netspeed";
        args[n++] = opts->netspeed;
    }
    if (opts->netdelay) {
        args[n++] = "-netdelay";
        args[n++] = opts->netdelay;
    }
    if (opts->netfast) {
        args[n++] = "-netfast";
    }

    if (opts->audio) {
        args[n++] = "-audio";
        args[n++] = opts->audio;
    }

    if (opts->cpu_delay) {
        args[n++] = "-cpu-delay";
        args[n++] = opts->cpu_delay;
    }

    if (opts->dns_server) {
        args[n++] = "-dns-server";
        args[n++] = opts->dns_server;
    }

    /** SNAPSHOT STORAGE HANDLING */

    /* Determine snapstorage path. -no-snapstorage disables all snapshotting
     * support. This means you can't resume a snapshot at load, save it at
     * exit, or even load/save them dynamically at runtime with the console.
     */
    if (opts->no_snapstorage) {

        if (opts->snapshot) {
            dwarning("ignoring -snapshot option due to the use of -no-snapstorage");
            opts->snapshot = NULL;
        }

        if (opts->snapstorage) {
            dwarning("ignoring -snapstorage option due to the use of -no-snapstorage");
            opts->snapstorage = NULL;
        }
    }
    else
    {
        if (!opts->snapstorage && avdInfo_getSnapshotPresent(avd)) {
            opts->snapstorage = avdInfo_getSnapStoragePath(avd);
            if (opts->snapstorage != NULL) {
                D("autoconfig: -snapstorage %s", opts->snapstorage);
            }
        }

        if (opts->snapstorage && !path_exists(opts->snapstorage)) {
            D("no image at '%s', state snapshots disabled", opts->snapstorage);
            opts->snapstorage = NULL;
        }
    }

    /* If we have a valid snapshot storage path */

    if (opts->snapstorage) {

        hw->disk_snapStorage_path = ASTRDUP(opts->snapstorage);

        /* -no-snapshot is equivalent to using both -no-snapshot-load
        * and -no-snapshot-save. You can still load/save snapshots dynamically
        * from the console though.
        */
        if (opts->no_snapshot) {

            opts->no_snapshot_load = 1;
            opts->no_snapshot_save = 1;

            if (opts->snapshot) {
                dwarning("ignoring -snapshot option due to the use of -no-snapshot.");
            }
        }

        if (!opts->no_snapshot_load || !opts->no_snapshot_save) {
            if (opts->snapshot == NULL) {
                opts->snapshot = "default-boot";
                D("autoconfig: -snapshot %s", opts->snapshot);
            }
        }

        /* We still use QEMU command-line options for the following since
        * they can change from one invokation to the next and don't really
        * correspond to the hardware configuration itself.
        */
        if (!opts->no_snapshot_load) {
            args[n++] = "-loadvm";
            args[n++] = ASTRDUP(opts->snapshot);
        }

        if (!opts->no_snapshot_save) {
            args[n++] = "-savevm-on-exit";
            args[n++] = ASTRDUP(opts->snapshot);
        }

        if (opts->no_snapshot_update_time) {
            args[n++] = "-snapshot-no-time-update";
        }
    }

    if (!opts->logcat || opts->logcat[0] == 0) {
        opts->logcat = getenv("ANDROID_LOG_TAGS");
        if (opts->logcat && opts->logcat[0] == 0)
            opts->logcat = NULL;
    }

    /* we always send the kernel messages from ttyS0 to android_kmsg */
    if (opts->show_kernel) {
        args[n++] = "-show-kernel";
    }

    /* XXXX: TODO: implement -shell and -logcat through qemud instead */
    if (!opts->shell_serial) {
#ifdef _WIN32
        opts->shell_serial = "con:";
#else
        opts->shell_serial = "stdio";
#endif
    }
    else
        opts->shell = 1;

    if (opts->shell || opts->logcat) {
        args[n++] = "-serial";
        args[n++] = opts->shell_serial;
        shell_serial = serial++;
    }

    if (opts->radio) {
        args[n++] = "-radio";
        args[n++] = opts->radio;
    }

    if (opts->gps) {
        args[n++] = "-gps";
        args[n++] = opts->gps;
    }

    if (opts->selinux) {
        if ((strcmp(opts->selinux, "permissive") != 0)
                && (strcmp(opts->selinux, "disabled") != 0)) {
            derror("-selinux must be \"disabled\" or \"permissive\"");
            exit(1);
        }
    }

    if (hw->vm_heapSize == 0) {
        /* Compute the default heap size based on the RAM size.
         * Essentially, we want to ensure the following liberal mappings:
         *
         *   96MB RAM -> 16MB heap
         *  128MB RAM -> 24MB heap
         *  256MB RAM -> 48MB heap
         */
        int  ramSize = hw->hw_ramSize;
        int  heapSize;

        if (ramSize < 100)
            heapSize = 16;
        else if (ramSize < 192)
            heapSize = 24;
        else
            heapSize = 48;

        hw->vm_heapSize = heapSize;
    }

    if (opts->code_profile) {
        args[n++] = "-code-profile";
        args[n++] = opts->code_profile;
    }

    /* Pass boot properties to the core. First, those from boot.prop,
     * then those from the command-line */
    const FileData* bootProperties = avdInfo_getBootProperties(avd);
    if (!fileData_isEmpty(bootProperties)) {
        PropertyFileIterator iter[1];
        propertyFileIterator_init(iter,
                                  bootProperties->data,
                                  bootProperties->size);
        while (propertyFileIterator_next(iter)) {
            char temp[MAX_PROPERTY_NAME_LEN + MAX_PROPERTY_VALUE_LEN + 2];
            snprintf(temp, sizeof temp, "%s=%s", iter->name, iter->value);
            args[n++] = "-boot-property";
            args[n++] = ASTRDUP(temp);
        }
    }

    if (opts->prop != NULL) {
        ParamList*  pl = opts->prop;
        for ( ; pl != NULL; pl = pl->next ) {
            args[n++] = "-boot-property";
            args[n++] = pl->param;
        }
    }

    /* Setup the kernel init options
     */
    {
        static char  params[1024];
        char        *p = params, *end = p + sizeof(params);

        /* Don't worry about having a leading space here, this is handled
         * by the core later. */

        p = bufprint(p, end, " androidboot.hardware=goldfish");
#ifdef TARGET_I386
        p = bufprint(p, end, " clocksource=pit");
#endif

        if (opts->shell || opts->logcat) {
            p = bufprint(p, end, " androidboot.console=%s%d",
                         androidHwConfig_getKernelSerialPrefix(android_hw),
                         shell_serial );
        }

        if (!opts->no_jni) {
            p = bufprint(p, end, " android.checkjni=1");
        }

        if (opts->no_boot_anim) {
            p = bufprint( p, end, " android.bootanim=0" );
        }

        if (opts->logcat) {
            char*  q = bufprint(p, end, " androidboot.logcat=%s", opts->logcat);

            if (q < end) {
                /* replace any space by a comma ! */
                {
                    int  nn;
                    for (nn = 1; p[nn] != 0; nn++)
                        if (p[nn] == ' ' || p[nn] == '\t')
                            p[nn] = ',';
                    p += nn;
                }
            }
            p = q;
        }

        if (opts->bootchart) {
            p = bufprint(p, end, " androidboot.bootchart=%s", opts->bootchart);
        }

        if (opts->selinux) {
            p = bufprint(p, end, " androidboot.selinux=%s", opts->selinux);
        }

        if (p >= end) {
            fprintf(stderr, "### ERROR: kernel parameters too long\n");
            exit(1);
        }

        hw->kernel_parameters = strdup(params);
    }

    if (opts->ports) {
        args[n++] = "-android-ports";
        args[n++] = opts->ports;
    }

    if (opts->port) {
        args[n++] = "-android-port";
        args[n++] = opts->port;
    }

    if (opts->report_console) {
        args[n++] = "-android-report-console";
        args[n++] = opts->report_console;
    }

    if (opts->http_proxy) {
        args[n++] = "-http-proxy";
        args[n++] = opts->http_proxy;
    }

    if (!opts->charmap) {
        /* Try to find a valid charmap name */
        char* charmap = avdInfo_getCharmapFile(avd, hw->hw_keyboard_charmap);
        if (charmap != NULL) {
            D("autoconfig: -charmap %s", charmap);
            opts->charmap = charmap;
        }
    }

    if (opts->charmap) {
        char charmap_name[SKIN_CHARMAP_NAME_SIZE];

        if (!path_exists(opts->charmap)) {
            derror("Charmap file does not exist: %s", opts->charmap);
            exit(1);
        }
        /* We need to store the charmap name in the hardware configuration.
         * However, the charmap file itself is only used by the UI component
         * and doesn't need to be set to the emulation engine.
         */
        kcm_extract_charmap_name(opts->charmap, charmap_name,
                                 sizeof(charmap_name));
        reassign_string(&hw->hw_keyboard_charmap, charmap_name);
    }

    {
        EmuglConfig config;

        if (!emuglConfig_init(&config,
                              hw->hw_gpu_enabled,
                              hw->hw_gpu_mode,
                              opts->gpu,
                              0,
                              opts->no_window)) {
            derror("%s", config.status);
            exit(1);
        }
        hw->hw_gpu_enabled = config.enabled;
        reassign_string(&hw->hw_gpu_mode, config.backend);
        D("%s", config.status);
    }

    /* Quit emulator on condition that both, gpu and snapstorage are on. This is
     * a temporary solution preventing the emulator from crashing until GPU state
     * can be properly saved / resored in snapshot file. */
    if (hw->hw_gpu_enabled && opts->snapstorage && (!opts->no_snapshot_load ||
                                                    !opts->no_snapshot_save)) {
        derror("Snapshots and gpu are mutually exclusive at this point. Please turn one of them off, and restart the emulator.");
        exit(1);
    }

    /* Deal with camera emulation */
    if (opts->webcam_list) {
        /* List connected webcameras */
        args[n++] = "-list-webcam";
    }

    if (opts->camera_back) {
        /* Validate parameter. */
        if (memcmp(opts->camera_back, "webcam", 6) &&
            strcmp(opts->camera_back, "emulated") &&
            strcmp(opts->camera_back, "none")) {
            derror("Invalid value for -camera-back <mode> parameter: %s\n"
                   "Valid values are: 'emulated', 'webcam<N>', or 'none'\n",
                   opts->camera_back);
            exit(1);
        }
        hw->hw_camera_back = ASTRDUP(opts->camera_back);
    }

    if (opts->camera_front) {
        /* Validate parameter. */
        if (memcmp(opts->camera_front, "webcam", 6) &&
            strcmp(opts->camera_front, "emulated") &&
            strcmp(opts->camera_front, "none")) {
            derror("Invalid value for -camera-front <mode> parameter: %s\n"
                   "Valid values are: 'emulated', 'webcam<N>', or 'none'\n",
                   opts->camera_front);
            exit(1);
        }
        hw->hw_camera_front = ASTRDUP(opts->camera_front);
    }

    /* physical memory is now in hw->hw_ramSize */

    hw->avd_name = ASTRDUP(avdInfo_getName(avd));

    /* Set up the interfaces for inter-emulator networking */
    if (opts->shared_net_id) {
        unsigned int shared_net_id = atoi(opts->shared_net_id);
        char nic[37];

        args[n++] = "-net";
        args[n++] = "nic,vlan=0";
        args[n++] = "-net";
        args[n++] = "user,vlan=0";

        args[n++] = "-net";
        snprintf(nic, sizeof nic, "nic,vlan=1,macaddr=52:54:00:12:34:%02x", shared_net_id);
        args[n++] = strdup(nic);
        args[n++] = "-net";
        args[n++] = "socket,vlan=1,mcast=230.0.0.10:1234";
    }

#if defined(TARGET_I386) || defined(TARGET_X86_64)
    char* accel_status = NULL;
    CpuAccelMode accel_mode = ACCEL_AUTO;
    bool accel_ok = handleCpuAcceleration(opts, avd, &accel_mode, accel_status);

    // CPU acceleration only works for x86 and x86_64 system images.
    if (accel_mode == ACCEL_OFF && accel_ok) {
        args[n++] = ASTRDUP(kDisableAccelerator);
    } else if (accel_mode == ACCEL_ON) {
        if (!accel_ok) {
            derror("CPU acceleration not supported on this machine!");
            derror("Reason: %s", accel_status);
            exit(1);
        }
        args[n++] = ASTRDUP(kEnableAccelerator);
    } else {
        args[n++] = accel_ok ? ASTRDUP(kEnableAccelerator)
                             : ASTRDUP(kDisableAccelerator);
    }

    AFREE(accel_status);
#else
    if (VERBOSE_CHECK(init)) {
        dwarning("CPU acceleration only works with x86/x86_64 "
            "system images.");
    }
#endif

    /* Setup screen emulation */
    if (opts->screen) {
        if (strcmp(opts->screen, "touch") &&
            strcmp(opts->screen, "multi-touch") &&
            strcmp(opts->screen, "no-touch")) {

            derror("Invalid value for -screen <mode> parameter: %s\n"
                   "Valid values are: touch, multi-touch, or no-touch\n",
                   opts->screen);
            exit(1);
        }
        hw->hw_screen = ASTRDUP(opts->screen);
    }

    while(argc-- > 0) {
        args[n++] = *argv++;
    }
    args[n] = 0;

    /* Generate a hardware-qemu.ini for this AVD. The real hardware
     * configuration is ususally stored in several files, e.g. the AVD's
     * config.ini plus the skin-specific hardware.ini.
     *
     * The new file will group all definitions and will be used to
     * launch the core with the -android-hw <file> option.
     */
    {
        const char* coreHwIniPath = avdInfo_getCoreHwIniPath(avd);
        IniFile*    hwIni         = iniFile_newFromMemory("", NULL);
        androidHwConfig_write(hw, hwIni);

        if (filelock_create(coreHwIniPath) == NULL) {
            /* The AVD is already in use, we still support this as an
             * experimental feature. Use a temporary hardware-qemu.ini
             * file though to avoid overwriting the existing one. */
             TempFile*  tempIni = tempfile_create();
             coreHwIniPath = tempfile_path(tempIni);
        }

        /* While saving HW config, ignore valueless entries. This will not break
         * anything, but will significantly simplify comparing the current HW
         * config with the one that has been associated with a snapshot (in case
         * VM starts from a snapshot for this instance of emulator). */
        if (iniFile_saveToFileClean(hwIni, coreHwIniPath) < 0) {
            derror("Could not write hardware.ini to %s: %s", coreHwIniPath, strerror(errno));
            exit(2);
        }
        args[n++] = "-android-hw";
        args[n++] = strdup(coreHwIniPath);

        /* In verbose mode, dump the file's content */
        if (VERBOSE_CHECK(init)) {
            FILE* file = fopen(coreHwIniPath, "rt");
            if (file == NULL) {
                derror("Could not open hardware configuration file: %s\n",
                       coreHwIniPath);
            } else {
                LineInput* input = lineInput_newFromStdFile(file);
                const char* line;
                printf("Content of hardware configuration file:\n");
                while ((line = lineInput_getLine(input)) !=  NULL) {
                    printf("  %s\n", line);
                }
                printf(".\n");
                lineInput_free(input);
                fclose(file);
            }
        }
    }

    if(VERBOSE_CHECK(init)) {
        int i;
        printf("QEMU options list:\n");
        for(i = 0; i < n; i++) {
            printf("emulator: argv[%02d] = \"%s\"\n", i, args[i]);
        }
        /* Dump final command-line option to make debugging the core easier */
        printf("Concatenated QEMU options:\n");
        for (i = 0; i < n; i++) {
            /* To make it easier to copy-paste the output to a command-line,
             * quote anything that contains spaces.
             */
            if (strchr(args[i], ' ') != NULL) {
                printf(" '%s'", args[i]);
            } else {
                printf(" %s", args[i]);
            }
        }
        printf("\n");
    }

    /* Setup SDL UI just before calling the code */
#if defined(CONFIG_SDL)
    init_sdl_ui(skinConfig, skinPath, opts);
    enter_qemu_main_loop(n, args);
#elif defined(CONFIG_QT)
#ifndef _WIN32
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
#endif
    init_sdl_ui(skinConfig, skinPath, opts);
    skin_winsys_spawn_thread(enter_qemu_main_loop, n, args);
    skin_winsys_enter_main_loop(argc, argv);
#endif
    return 0;
}
