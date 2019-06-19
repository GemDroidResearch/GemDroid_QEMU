// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/system/System.h"

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/String.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <unistd.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

static void make_subfile(const String& dir, const char* file) {
    String path = dir;
    path.append("/");
    path.append(file);
    int fd = ::open(path.c_str(), O_WRONLY|O_CREAT, 0755);
    EXPECT_GE(fd, 0) << "Path: " << path.c_str();
    ::close(fd);
}

TEST(System, get) {
    System* sys1 = System::get();
    EXPECT_TRUE(sys1);

    System* sys2 = System::get();
    EXPECT_EQ(sys1, sys2);
}

TEST(System, getProgramDirectory) {
    String dir = System::get()->getProgramDirectory();
    EXPECT_FALSE(dir.empty());
    LOG(INFO) << "Program directory: [" << dir.c_str() << "]";
}

TEST(System, TestSystem) {
    const char kProgramDir[] = "/foo/bar";
    TestSystem testSys(kProgramDir, 32);
    String dir = System::get()->getProgramDirectory();
    EXPECT_STREQ(kProgramDir, dir.c_str());
}

TEST(System, getHostBitness) {
    int hostBitness = System::get()->getHostBitness();
    LOG(INFO) << "Host bitness: " << hostBitness;
    EXPECT_TRUE(hostBitness == 32 || hostBitness == 64);

    {
        TestSystem sysTest("/foo", 32);
        EXPECT_EQ(32, System::get()->getHostBitness());
    }
    {
        TestSystem sysTest("/foo64", 64);
        EXPECT_EQ(64, System::get()->getHostBitness());
    }
}

TEST(System, getProgramBitness) {
    const int kExpected = (sizeof(void*) == 8) ? 64 : 32;
    EXPECT_EQ(kExpected, System::get()->getProgramBitness());
}

TEST(System, scandDirEntries) {
    static const char* const kExpected[] = {
        "fifth", "first", "fourth", "second", "sixth", "third"
    };
    static const char* const kInput[] = {
        "first", "second", "third", "fourth", "fifth", "sixth"
    };
    const size_t kCount = ARRAYLEN(kInput);

    TestTempDir myDir("scanDirEntries");
    for (size_t n = 0; n < kCount; ++n) {
        make_subfile(myDir.path(), kInput[n]);
    }

    StringVector entries = System::get()->scanDirEntries(myDir.path());

    EXPECT_EQ(kCount, entries.size());
    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_STREQ(kExpected[n], entries[n].c_str()) << "#" << n;
    }
}

TEST(System, envGetAndSet) {
    System* sys = System::get();
    const char kVarName[] = "FOO_BAR_TESTING_STUFF";
    const char kVarValue[] = "SomethingCompletelyRandomForYou!";

    EXPECT_FALSE(sys->envGet(kVarName));
    sys->envSet(kVarName, kVarValue);
    EXPECT_STREQ(kVarValue, sys->envGet(kVarName));
    sys->envSet(kVarName, NULL);
    EXPECT_FALSE(sys->envGet(kVarName));
}

TEST(System, scanDirEntriesWithFullPaths) {
    static const char* const kExpected[] = {
        "fifth", "first", "fourth", "second", "sixth", "third"
    };
    static const char* const kInput[] = {
        "first", "second", "third", "fourth", "fifth", "sixth"
    };
    const size_t kCount = ARRAYLEN(kInput);

    TestTempDir myDir("scanDirEntriesFull");
    for (size_t n = 0; n < kCount; ++n) {
        make_subfile(myDir.path(), kInput[n]);
    }

    StringVector entries = System::get()->scanDirEntries(myDir.path(), true);

    EXPECT_EQ(kCount, entries.size());
    for (size_t n = 0; n < kCount; ++n) {
        String expected(myDir.path());
        expected = PathUtils::addTrailingDirSeparator(expected);
        expected += kExpected[n];
        EXPECT_STREQ(expected.c_str(), entries[n].c_str()) << "#" << n;
    }
}

TEST(System, isRemoteSession) {
    String sessionType;
    bool isRemote = System::get()->isRemoteSession(&sessionType);
    if (isRemote) {
        LOG(INFO) << "Remote session type [" << sessionType.c_str() << "]";
    } else {
        LOG(INFO) << "Local session type";
    }
}

TEST(System, addLibrarySearchDir) {
    TestSystem testSys("/foo/bar", 32);
    TestTempDir* testDir = testSys.getTempRoot();
    ASSERT_TRUE(testDir->makeSubDir("lib"));
    testSys.addLibrarySearchDir("lib");
}

TEST(System, findBundledExecutable) {
#ifdef _WIN32
    static const char kProgramFile[] = "myprogram.exe";
#else
    static const char kProgramFile[] = "myprogram";
#endif

    TestSystem testSys("/foo", System::kProgramBitness);
    TestTempDir* testDir = testSys.getTempRoot();
    ASSERT_TRUE(testDir->makeSubDir("foo"));

    StringVector pathList;
    pathList.push_back(String("foo"));
    pathList.push_back(String(System::kBinSubDir));
    ASSERT_TRUE(testDir->makeSubDir(PathUtils::recompose(pathList).c_str()));

    pathList.push_back(String(kProgramFile));
    String programPath = PathUtils::recompose(pathList);
    make_subfile(testDir->path(), programPath.c_str());

    String path = testSys.findBundledExecutable("myprogram");
    String expectedPath("/");
    expectedPath += programPath;
    EXPECT_STREQ(expectedPath.c_str(), path.c_str());

    path = testSys.findBundledExecutable("otherprogram");
    EXPECT_FALSE(path.size());
}

}  // namespace base
}  // namespace android
