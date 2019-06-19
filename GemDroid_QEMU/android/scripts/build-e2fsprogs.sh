#!/bin/sh

# Copyright 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

. $(dirname "$0")/utils/common.shi

shell_import utils/aosp_dir.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/install_dir.shi
shell_import utils/option_parser.shi
shell_import utils/package_list_parser.shi
shell_import utils/package_builder.shi

# This script cannot build windows-x86_64 binaries - only 32-bit support
# is achievable as the 64-bit distribution of e2fsprogs requires a newer
# version of a dll dependency than is available.  The available dependencies
# (version 1.42.7) do not export the ext2fs_close_free function that was
# added to a later version of e2fsprogs (at version 1.42.11), and no early
# enough executable version of e2fsprogs is readily available from Cygwin.
case $(get_build_os) in
    linux)
        DEFAULT_HOST_SYSTEMS="linux-x86,linux-x86_64,windows-x86"
        ;;
esac

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build prebuilt e2fsprogs for Linux and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option e2fsprogs

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

ARCHIVE_DIR=$PREBUILTS_DIR/archive
if [ ! -d "$ARCHIVE_DIR" ]; then
    dump "Downloading dependencies sources first."
    $(program_directory)/download-sources.sh \
        --verbosity=$(get_verbosity) \
        --prebuilts-dir="$PREBUILTS_DIR" ||
            panic "Could not download source archives!"
fi
if [ ! -d "$ARCHIVE_DIR" ]; then
    panic "Missing archive directory: $ARCHIVE_DIR"
fi
PACKAGE_LIST=$ARCHIVE_DIR/PACKAGES.TXT
if [ ! -f "$PACKAGE_LIST" ]; then
    panic "Missing package list file, run download-sources.sh: $PACKAGE_LIST"
fi

package_builder_process_options e2fsprogs

package_list_parse_file "$PACKAGE_LIST"

BUILD_SRC_DIR=$TEMP_DIR/src

# Unpack package source into $BUILD_SRC_DIR if needed.
# $1: Package basename.
unpack_package_source () {
    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_NAME=$(package_list_get_unpack_src_dir $1)
    PKG_SRC_TIMESTAMP=$BUILD_SRC_DIR/timestamp-$PKG_NAME
    if [ ! -f "$PKG_SRC_TIMESTAMP" ]; then
        package_list_unpack_and_patch "$1" "$ARCHIVE_DIR" "$BUILD_SRC_DIR"
        touch $PKG_SRC_TIMESTAMP
    fi
}

# For windows we have already downloaded the executables
# so just uncompress them to the correct directory in
# preparation for android-rebuild.sh.
# $1: Destination directory of dependencies
WINDOWS_DEPENDENCIES="e2fsprogs-windows cygwin libcom_err2\
    libe2p2 libblkid1 libuuid1 libext2fs2 libgcc1 libiconv2 libintl8"
unpack_windows_dependencies () {
    local DEP DSTDIR
    DSTDIR=$1
    for DEP in $WINDOWS_DEPENDENCIES; do
        run mkdir -p "$BUILD_SRC_DIR/$(package_list_get_unpack_src_dir $DEP)"
        unpack_package_source "$DEP"
    done

    copy_directory_files \
                        "$BUILD_SRC_DIR/usr/bin" \
                        "$DSTDIR/sbin" \
                        cygblkid-1.dll \
                        cygcom_err-2.dll \
                        cyge2p-2.dll \
                        cygext2fs-2.dll \
                        cyggcc_s-1.dll \
                        cygiconv-2.dll \
                        cygintl-8.dll \
                        cyguuid-1.dll \
                        cygwin1.dll

    copy_directory_files \
                        "$BUILD_SRC_DIR/usr" \
                        "$DSTDIR" \
                        sbin/e2fsck.exe \
                        sbin/resize2fs.exe \
                        sbin/tune2fs.exe
}

# $1: Package basename (e.g. 'libpthread-stubs-0.3')
# $2+: Extra configuration options.
build_package () {
    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_NAME=$(package_list_get_src_dir $1)
    unpack_package_source "$1"
    shift
    PKG_SRC_DIR="$BUILD_SRC_DIR/$PKG_NAME"
    PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/$PKG_NAME
    PKG_TIMESTAMP=$TEMP_DIR/build-$SYSTEM/$PKG_NAME-timestamp
    if [ ! -f "$PKG_TIMESTAMP" -o -n "$OPT_FORCE" ]; then
        case $SYSTEM in
            darwin*)
                # Required for proper build on Darwin!
                builder_disable_verbose_install
                ;;
        esac
        builder_build_autotools_package \
            "$PKG_SRC_DIR" \
            "$PKG_BUILD_DIR" \
            "$@"

        touch "$PKG_TIMESTAMP"
    fi
}

# Perform a Darwin build through ssh to a remote machine.
# $1: Darwin host name.
# $2: List of darwin target systems to build for.
do_remote_darwin_build () {
    builder_prepare_remote_darwin_build \
            "/tmp/$USER-rebuild-darwin-ssh-$$/e2fsprogs-build"

    copy_directory "$ARCHIVE_DIR" "$DARWIN_PKG_DIR"/archive

    local PKG_DIR="$DARWIN_PKG_DIR"
    local REMOTE_DIR=/tmp/$DARWIN_PKG_NAME
    # Generate a script to rebuild all binaries from sources.
    # Note that the use of the '-l' flag is important to ensure
    # that this is run under a login shell. This ensures that
    # ~/.bash_profile is sourced before running the script, which
    # puts MacPorts' /opt/local/bin in the PATH properly.
    #
    # If not, the build is likely to fail with a cryptic error message
    # like "readlink: illegal option -- f"
    cat > $PKG_DIR/build.sh <<EOF
#!/bin/bash -l
PROGDIR=\$(dirname \$0)
\$PROGDIR/scripts/$(program_name) \\
    --build-dir=$REMOTE_DIR/build \\
    --host=$(spaces_to_commas "$DARWIN_SYSTEMS") \\
    --install-dir=$REMOTE_DIR/install-prefix \\
    --prebuilts-dir=$REMOTE_DIR \\
    --aosp-dir=$REMOTE_DIR/aosp \\
    $DARWIN_BUILD_FLAGS
EOF
    builder_run_remote_darwin_build

    run rm -rf "$PKG_DIR"

    run mkdir -p "$INSTALL_DIR" ||
            panic "Could not create final directory: $INSTALL_DIR"

    for SYSTEM in $DARWIN_SYSTEMS; do
        dump "[$SYSTEM] Retrieving remote darwin binaries"
        run rm -rf "$INSTALL_DIR"/* &&
        run rsync -haz --delete --exclude=intermediates --exclude=libs \
                $DARWIN_SSH:$REMOTE_DIR/install-prefix/$SYSTEM \
                $INSTALL_DIR
    done
}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote e2fsprogs build for: $DARWIN_SYSTEMS"
    do_remote_darwin_build "$DARWIN_SSH" "$DARWIN_SYSTEMS"
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        dump "$(builder_text) Building e2fsprogs"

        CONFIGURE_FLAGS=
        var_append CONFIGURE_FLAGS \
                --disable-nls \
                --disable-defrag \
                --disable-jbd-debug \
                --disable-profile \
                --disable-testio-debug \
                --disable-rpath \

        case $SYSTEM in
            windows-x86)
                unpack_windows_dependencies "$INSTALL_DIR/$SYSTEM"
                ;;
            windows-x86_64)
                dump "WARNING: windows-x86_64 isn't supported with this script!"
                ;;
            *)
                build_package e2fsprogs $CONFIGURE_FLAGS

                # Copy binaries necessary for the build itself as well as static
                # libraries.
                copy_directory_files \
                        "$(builder_install_prefix)" \
                        "$INSTALL_DIR/$SYSTEM" \
                        sbin/e2fsck \
                        sbin/fsck.ext4 \
                        sbin/mkfs.ext4 \
                        sbin/resize2fs \
                        sbin/tune2fs \
                ;;
        esac


    ) || panic "[$SYSTEM] Could not build e2fsprogs!"

done

log "Done building e2fsprogs."
