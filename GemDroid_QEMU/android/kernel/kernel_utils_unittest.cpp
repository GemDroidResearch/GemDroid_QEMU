// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/kernel/kernel_utils.h"

#include "android/kernel/kernel_utils_testing.h"

#include <gtest/gtest.h>

namespace android {
namespace kernel {

TEST(KernelUtils, GetKernelSerialDevicePrefix) {
    EXPECT_STREQ("ttyS",
                 android_kernelSerialDevicePrefix(KERNEL_VERSION_2_6_29));
    EXPECT_STREQ("ttyS",
                 android_kernelSerialDevicePrefix(KERNEL_VERSION_3_4_0));
    EXPECT_STREQ("ttyS",
                 android_kernelSerialDevicePrefix(KERNEL_VERSION_3_4_67));
    EXPECT_STREQ("ttyGF",
                 android_kernelSerialDevicePrefix(KERNEL_VERSION_3_10_0));
}

TEST(KernelUtils, ProbeKernelVersionString) {
    // you can regenerate these tables using 
    // android/kernel/testing/print_mock_kernel_data.sh

    const char kMockKernelVersion[] =
        "Linux version 3.10.0+ (vharron@tifa.mtv.corp.google.com) "
        "(gcc version 4.7 (GCC) ) #1 PREEMPT Sat Jan 5 2:45:24 PDT 2008\n";

    // a mock uncompressed kernel
    // ELF header, followed by an unspecified number of bytes
    // followed by a 'Linux version ' string
    static const unsigned char kMockKernelElf[] = {
        0x7f, 0x45, 0x4c, 0x46, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x4c, 0x69, 0x6e, 0x75, 0x78, 0x20, 0x76, 0x65, 0x72, 0x73,
        0x69, 0x6f, 0x6e, 0x20, 0x33, 0x2e, 0x31, 0x30, 0x2e, 0x30, 0x2b, 0x20,
        0x28, 0x76, 0x68, 0x61, 0x72, 0x72, 0x6f, 0x6e, 0x40, 0x74, 0x69, 0x66,
        0x61, 0x2e, 0x6d, 0x74, 0x76, 0x2e, 0x63, 0x6f, 0x72, 0x70, 0x2e, 0x67,
        0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x29, 0x20, 0x28,
        0x67, 0x63, 0x63, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x20,
        0x34, 0x2e, 0x37, 0x20, 0x28, 0x47, 0x43, 0x43, 0x29, 0x20, 0x29, 0x20,
        0x23, 0x31, 0x20, 0x50, 0x52, 0x45, 0x45, 0x4d, 0x50, 0x54, 0x20, 0x53,
        0x61, 0x74, 0x20, 0x4a, 0x61, 0x6e, 0x20, 0x35, 0x20, 0x32, 0x3a, 0x34,
        0x35, 0x3a, 0x32, 0x34, 0x20, 0x50, 0x44, 0x54, 0x20, 0x32, 0x30, 0x30,
        0x38, 0x0a, 0x00, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39
    };

    // a mock uncompressed kernel without version string
    // ELF header, followed by an unspecified number of bytes
    static const unsigned char kMockKernelElfNoString[] = {
        0x7f, 0x45, 0x4c, 0x46, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39
    };

    // a mock compressed kernel
    // an unspecified number of bytes, followed by a gzip header
    // gzip stream starts 10 bytes after gzip header, uncompressed gzip stream
    // begins with an ELF header as above
    static const unsigned char kMockKernelGzip[] = {
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x1f, 0x8b,
        0x08, 0x00, 0x24, 0xc7, 0xc6, 0x53, 0x00, 0x03, 0xab, 0x77, 0xf5, 0x71,
        0x33, 0x30, 0x34, 0x32, 0x36, 0x31, 0x35, 0x33, 0xb7, 0xb0, 0xf4, 0xc9,
        0xcc, 0x2b, 0xad, 0x50, 0x28, 0x4b, 0x2d, 0x2a, 0xce, 0xcc, 0xcf, 0x53,
        0x30, 0xd6, 0x33, 0x34, 0xd0, 0x33, 0xd0, 0x56, 0xd0, 0x28, 0xcb, 0x48,
        0x2c, 0x2a, 0xca, 0xcf, 0x73, 0x28, 0xc9, 0x4c, 0x4b, 0xd4, 0xcb, 0x2d,
        0x29, 0xd3, 0x4b, 0xce, 0x2f, 0x2a, 0xd0, 0x4b, 0xcf, 0xcf, 0x4f, 0xcf,
        0x49, 0x05, 0xb2, 0x73, 0x35, 0x15, 0x34, 0xd2, 0x93, 0x93, 0xe1, 0xfa,
        0x4c, 0xf4, 0xcc, 0x15, 0x34, 0xdc, 0x9d, 0x9d, 0x35, 0x15, 0x34, 0x15,
        0x94, 0x0d, 0x15, 0x02, 0x82, 0x5c, 0x5d, 0x7d, 0x03, 0x42, 0x14, 0x82,
        0x13, 0x4b, 0x14, 0xbc, 0x12, 0xf3, 0x14, 0x4c, 0x15, 0x8c, 0xac, 0x4c,
        0x4c, 0xad, 0x8c, 0x4c, 0x14, 0x02, 0x5c, 0x42, 0x14, 0x8c, 0x0c, 0x0c,
        0x2c, 0xb8, 0x18, 0xe0, 0x2e, 0x00, 0x00, 0xd3, 0x6e, 0x68, 0xa6, 0x90,
        0x00, 0x00, 0x00
    };

    // a mock semi-compressed kernel
    // an unspecified number of bytes, followed by a version string,
    // followed by a gzip header
    static const unsigned char kMockKernelSemiCompressed[] = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x39, 0x61,
    0x62, 0x63, 0x64, 0x65, 0x66, 0x4c, 0x69, 0x6e, 0x75, 0x78, 0x20, 0x76,
    0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x33, 0x2e, 0x31, 0x30, 0x2e,
    0x30, 0x2b, 0x20, 0x28, 0x76, 0x68, 0x61, 0x72, 0x72, 0x6f, 0x6e, 0x40,
    0x74, 0x69, 0x66, 0x61, 0x2e, 0x6d, 0x74, 0x76, 0x2e, 0x63, 0x6f, 0x72,
    0x70, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d,
    0x29, 0x20, 0x28, 0x67, 0x63, 0x63, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69,
    0x6f, 0x6e, 0x20, 0x34, 0x2e, 0x37, 0x20, 0x28, 0x47, 0x43, 0x43, 0x29,
    0x20, 0x29, 0x20, 0x23, 0x31, 0x20, 0x50, 0x52, 0x45, 0x45, 0x4d, 0x50,
    0x54, 0x20, 0x53, 0x61, 0x74, 0x20, 0x4a, 0x61, 0x6e, 0x20, 0x35, 0x20,
    0x32, 0x3a, 0x34, 0x35, 0x3a, 0x32, 0x34, 0x20, 0x50, 0x44, 0x54, 0x20,
    0x32, 0x30, 0x30, 0x38, 0x0a, 0x00, 0x31, 0x32, 0x33, 0x34, 0x35, 0x35,
    0x37, 0x38, 0x39, 0x1f, 0x8b, 0x08, 0x00, 0x3e, 0xa9, 0xc7, 0x53, 0x00,
    0x03, 0x0b, 0xf7, 0x70, 0x0c, 0x71, 0x0d, 0x73, 0x0d, 0x02, 0x00, 0xff,
    0xdf, 0x22, 0x88, 0x08, 0x00, 0x00, 0x00
    };

    char kernelVersionString[256];

    kernelVersionString[0] = 0;
    EXPECT_EQ(true, android_imageProbeKernelVersionString(
        kMockKernelElf,
        sizeof(kMockKernelElf),
        kernelVersionString,
        sizeof(kernelVersionString)));
    EXPECT_STREQ(kMockKernelVersion, kernelVersionString);

    kernelVersionString[0] = 0;
    EXPECT_EQ(true, android_imageProbeKernelVersionString(
        kMockKernelGzip,
        sizeof(kMockKernelGzip),
        kernelVersionString,
        sizeof(kernelVersionString)));
    EXPECT_STREQ(kMockKernelVersion, kernelVersionString);

    kernelVersionString[0] = 0;
    EXPECT_EQ(true, android_imageProbeKernelVersionString(
        kMockKernelSemiCompressed,
        sizeof(kMockKernelSemiCompressed),
        kernelVersionString,
        sizeof(kernelVersionString)));
    EXPECT_STREQ(kMockKernelVersion, kernelVersionString);

    kernelVersionString[0] = 127;
    EXPECT_FALSE(android_imageProbeKernelVersionString(
        0,
        0,
        kernelVersionString,
        sizeof(kernelVersionString)));
    EXPECT_EQ(127, kernelVersionString[0]);

    kernelVersionString[0] = 127;
    EXPECT_FALSE(android_imageProbeKernelVersionString(
        kMockKernelElfNoString,
        sizeof(kMockKernelElfNoString),
        kernelVersionString,
        sizeof(kernelVersionString)));
    EXPECT_EQ(127, kernelVersionString[0]);
}

void ParseKernelVersionString(const char* versionString,
                              KernelVersion expectedVersion) {
    KernelVersion actualVersion;
    EXPECT_EQ(true, android_parseLinuxVersionString(versionString,
                                                    &actualVersion));
    EXPECT_EQ(expectedVersion, actualVersion);
}

TEST(KernelUtils, ParseKernelVersionString) {
    ParseKernelVersionString("Linux version 2.6.29-gea477bb (kroo...\n",
                             KERNEL_VERSION_2_6_29);

    ParseKernelVersionString("Linux version 2.6.29 (vcht...\n",
                             KERNEL_VERSION_2_6_29);

    ParseKernelVersionString("Linux version 3.4.0-66985-gb04946b (digi...\n",
                             KERNEL_VERSION_3_4_0);

    ParseKernelVersionString("Linux version 3.4.67+ (ghac...\n",
                             KERNEL_VERSION_3_4_67 );

    ParseKernelVersionString("Linux version 3.4.67-01413-g9ac497f (ghac...\n",
                             KERNEL_VERSION_3_4_67);

    ParseKernelVersionString("Linux version 3.10.0+ (ghac...\n",
                             KERNEL_VERSION_3_10_0);
}

}  // namespace kernel
}  // namespace android
