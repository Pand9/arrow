// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <gtest/gtest.h>

#include "arrow/testing/gtest_util.h"
#include "arrow/util/io-util.h"

namespace arrow {
namespace internal {

void AssertExists(const PlatformFilename& path) {
  bool exists = false;
  ASSERT_OK(FileExists(path, &exists));
  ASSERT_TRUE(exists) << "Path '" << path.ToString() << "' doesn't exist";
}

void AssertNotExists(const PlatformFilename& path) {
  bool exists = true;
  ASSERT_OK(FileExists(path, &exists));
  ASSERT_FALSE(exists) << "Path '" << path.ToString() << "' exists";
}

TEST(CreateDirDeleteDir, Basics) {
  const std::string BASE = "xxx-io-util-test-dir";
  bool created, deleted;
  PlatformFilename parent, child;

  ASSERT_OK(PlatformFilename::FromString(BASE, &parent));
  ASSERT_EQ(parent.ToString(), BASE);

  // Make sure the directory doesn't exist already
  ARROW_UNUSED(DeleteDirTree(parent));

  AssertNotExists(parent);

  ASSERT_OK(CreateDir(parent, &created));
  ASSERT_TRUE(created);
  AssertExists(parent);
  ASSERT_OK(CreateDir(parent, &created));
  ASSERT_FALSE(created);  // already exists
  AssertExists(parent);

  ASSERT_OK(PlatformFilename::FromString(BASE + "/some-child", &child));
  ASSERT_OK(CreateDir(child, &created));
  ASSERT_TRUE(created);
  AssertExists(child);

  ASSERT_OK(DeleteDirTree(parent, &deleted));
  ASSERT_TRUE(deleted);
  AssertNotExists(parent);
  AssertNotExists(child);

  // Parent is deleted, cannot create child again
  ASSERT_RAISES(IOError, CreateDir(child, &created));

  // It's not an error to call DeleteDirTree on a non-existent path.
  ASSERT_OK(DeleteDirTree(parent, &deleted));
  ASSERT_FALSE(deleted);
}

TEST(TemporaryDir, Basics) {
  std::unique_ptr<TemporaryDir> dir;
  PlatformFilename fn;

  ASSERT_OK(TemporaryDir::Make("some-prefix-", &dir));
  fn = dir->path();
  // Path has a trailing separator, for convenience
  ASSERT_EQ(fn.ToString().back(), '/');
#if defined(_WIN32)
  ASSERT_EQ(fn.ToNative().back(), L'/');
#else
  ASSERT_EQ(fn.ToNative().back(), '/');
#endif
  AssertExists(fn);
  ASSERT_NE(fn.ToString().find("some-prefix-"), std::string::npos);

  // Create child contents to check that they're cleaned up at the end
#if defined(_WIN32)
  PlatformFilename child(fn.ToNative() + L"some-child");
#else
  PlatformFilename child(fn.ToNative() + "some-child");
#endif
  ASSERT_OK(CreateDir(child));
  AssertExists(child);

  dir.reset();
  AssertNotExists(fn);
  AssertNotExists(child);
}

TEST(DeleteFile, Basics) {
  std::unique_ptr<TemporaryDir> dir;
  PlatformFilename fn;
  int fd;
  bool deleted;

  ASSERT_OK(TemporaryDir::Make("io-util-test-", &dir));
  ASSERT_OK(dir->path().Join("test-file", &fn));

  AssertNotExists(fn);
  ASSERT_OK(FileOpenWritable(fn, true /* write_only */, true /* truncate */,
                             false /* append */, &fd));
  ASSERT_OK(FileClose(fd));
  AssertExists(fn);
  ASSERT_OK(DeleteFile(fn, &deleted));
  ASSERT_TRUE(deleted);
  AssertNotExists(fn);
  ASSERT_OK(DeleteFile(fn, &deleted));
  ASSERT_FALSE(deleted);
  AssertNotExists(fn);

  // Cannot call DeleteFile on directory
  ASSERT_OK(dir->path().Join("test-dir", &fn));
  ASSERT_OK(CreateDir(fn));
  AssertExists(fn);
  ASSERT_RAISES(IOError, DeleteFile(fn));
}

}  // namespace internal
}  // namespace arrow