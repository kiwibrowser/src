// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/sandbox_file_system_backend.h"

#include <stddef.h>

#include <memory>
#include <set>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "storage/browser/fileapi/file_system_backend.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/browser/fileapi/sandbox_file_system_backend_delegate.h"
#include "storage/browser/test/test_file_system_options.h"
#include "storage/common/fileapi/file_system_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using storage::FileSystemURL;
using storage::SandboxFileSystemBackend;
using storage::SandboxFileSystemBackendDelegate;

// PS stands for path separator.
#if defined(FILE_PATH_USES_WIN_SEPARATORS)
#define PS  "\\"
#else
#define PS  "/"
#endif

namespace content {

namespace {

const struct RootPathTest {
  storage::FileSystemType type;
  const char* origin_url;
  const char* expected_path;
} kRootPathTestCases[] = {
      {storage::kFileSystemTypeTemporary, "http://foo:1/", "000" PS "t"},
      {storage::kFileSystemTypePersistent, "http://foo:1/", "000" PS "p"},
      {storage::kFileSystemTypeTemporary, "http://bar.com/", "001" PS "t"},
      {storage::kFileSystemTypePersistent, "http://bar.com/", "001" PS "p"},
      {storage::kFileSystemTypeTemporary, "https://foo:2/", "002" PS "t"},
      {storage::kFileSystemTypePersistent, "https://foo:2/", "002" PS "p"},
      {storage::kFileSystemTypeTemporary, "https://bar.com/", "003" PS "t"},
      {storage::kFileSystemTypePersistent, "https://bar.com/", "003" PS "p"},
};

const struct RootPathFileURITest {
  storage::FileSystemType type;
  const char* origin_url;
  const char* expected_path;
} kRootPathFileURITestCases[] = {
    {storage::kFileSystemTypeTemporary, "file:///", "000" PS "t"},
    {storage::kFileSystemTypePersistent, "file:///", "000" PS "p"}};

void DidOpenFileSystem(base::File::Error* error_out,
                       const GURL& origin_url,
                       const std::string& name,
                       base::File::Error error) {
  *error_out = error;
}

}  // namespace

class SandboxFileSystemBackendTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(data_dir_.CreateUniqueTempDir());
    SetUpNewDelegate(CreateAllowFileAccessOptions());
  }

  void SetUpNewDelegate(const storage::FileSystemOptions& options) {
    delegate_.reset(new SandboxFileSystemBackendDelegate(
        nullptr /* quota_manager_proxy */,
        base::ThreadTaskRunnerHandle::Get().get(), data_dir_.GetPath(),
        nullptr /* special_storage_policy */, options,
        nullptr /* env_override */));
  }

  void SetUpNewBackend(const storage::FileSystemOptions& options) {
    SetUpNewDelegate(options);
    backend_.reset(new SandboxFileSystemBackend(delegate_.get()));
  }

  storage::SandboxFileSystemBackendDelegate::OriginEnumerator*
  CreateOriginEnumerator() const {
    return backend_->CreateOriginEnumerator();
  }

  void CreateOriginTypeDirectory(const GURL& origin,
                                 storage::FileSystemType type) {
    base::FilePath target = delegate_->
        GetBaseDirectoryForOriginAndType(origin, type, true);
    ASSERT_TRUE(!target.empty());
    ASSERT_TRUE(base::DirectoryExists(target));
  }

  bool GetRootPath(const GURL& origin_url,
                   storage::FileSystemType type,
                   storage::OpenFileSystemMode mode,
                   base::FilePath* root_path) {
    base::File::Error error = base::File::FILE_OK;
    backend_->ResolveURL(
        FileSystemURL::CreateForTest(origin_url, type, base::FilePath()), mode,
        base::BindOnce(&DidOpenFileSystem, &error));
    base::RunLoop().RunUntilIdle();
    if (error != base::File::FILE_OK)
      return false;
    base::FilePath returned_root_path =
        delegate_->GetBaseDirectoryForOriginAndType(
            origin_url, type, false /* create */);
    if (root_path)
      *root_path = returned_root_path;
    return !returned_root_path.empty();
  }

  base::FilePath file_system_path() const {
    return data_dir_.GetPath().Append(
        SandboxFileSystemBackendDelegate::kFileSystemDirectory);
  }

  base::ScopedTempDir data_dir_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<storage::SandboxFileSystemBackendDelegate> delegate_;
  std::unique_ptr<storage::SandboxFileSystemBackend> backend_;
};

TEST_F(SandboxFileSystemBackendTest, Empty) {
  SetUpNewBackend(CreateAllowFileAccessOptions());
  std::unique_ptr<SandboxFileSystemBackendDelegate::OriginEnumerator>
      enumerator(CreateOriginEnumerator());
  ASSERT_TRUE(enumerator->Next().is_empty());
}

TEST_F(SandboxFileSystemBackendTest, EnumerateOrigins) {
  SetUpNewBackend(CreateAllowFileAccessOptions());
  const char* temporary_origins[] = {
    "http://www.bar.com/",
    "http://www.foo.com/",
    "http://www.foo.com:1/",
    "http://www.example.com:8080/",
    "http://www.google.com:80/",
  };
  const char* persistent_origins[] = {
    "http://www.bar.com/",
    "http://www.foo.com:8080/",
    "http://www.foo.com:80/",
  };
  size_t temporary_size = arraysize(temporary_origins);
  size_t persistent_size = arraysize(persistent_origins);
  std::set<GURL> temporary_set, persistent_set;
  for (size_t i = 0; i < temporary_size; ++i) {
    CreateOriginTypeDirectory(GURL(temporary_origins[i]),
                              storage::kFileSystemTypeTemporary);
    temporary_set.insert(GURL(temporary_origins[i]));
  }
  for (size_t i = 0; i < persistent_size; ++i) {
    CreateOriginTypeDirectory(GURL(persistent_origins[i]),
                              storage::kFileSystemTypePersistent);
    persistent_set.insert(GURL(persistent_origins[i]));
  }

  std::unique_ptr<SandboxFileSystemBackendDelegate::OriginEnumerator>
      enumerator(CreateOriginEnumerator());
  size_t temporary_actual_size = 0;
  size_t persistent_actual_size = 0;
  GURL current;
  while (!(current = enumerator->Next()).is_empty()) {
    SCOPED_TRACE(testing::Message() << "EnumerateOrigin " << current.spec());
    if (enumerator->HasFileSystemType(storage::kFileSystemTypeTemporary)) {
      ASSERT_TRUE(temporary_set.find(current) != temporary_set.end());
      ++temporary_actual_size;
    }
    if (enumerator->HasFileSystemType(storage::kFileSystemTypePersistent)) {
      ASSERT_TRUE(persistent_set.find(current) != persistent_set.end());
      ++persistent_actual_size;
    }
  }

  EXPECT_EQ(temporary_size, temporary_actual_size);
  EXPECT_EQ(persistent_size, persistent_actual_size);
}

TEST_F(SandboxFileSystemBackendTest, GetRootPathCreateAndExamine) {
  std::vector<base::FilePath> returned_root_path(arraysize(kRootPathTestCases));
  SetUpNewBackend(CreateAllowFileAccessOptions());

  // Create a new root directory.
  for (size_t i = 0; i < arraysize(kRootPathTestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "RootPath (create) #" << i << " "
                 << kRootPathTestCases[i].expected_path);

    base::FilePath root_path;
    EXPECT_TRUE(GetRootPath(GURL(kRootPathTestCases[i].origin_url),
                            kRootPathTestCases[i].type,
                            storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
                            &root_path));

    base::FilePath expected = file_system_path().AppendASCII(
        kRootPathTestCases[i].expected_path);
    EXPECT_EQ(expected.value(), root_path.value());
    EXPECT_TRUE(base::DirectoryExists(root_path));
    ASSERT_TRUE(returned_root_path.size() > i);
    returned_root_path[i] = root_path;
  }

  // Get the root directory with create=false and see if we get the
  // same directory.
  for (size_t i = 0; i < arraysize(kRootPathTestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "RootPath (get) #" << i << " "
                 << kRootPathTestCases[i].expected_path);

    base::FilePath root_path;
    EXPECT_TRUE(GetRootPath(GURL(kRootPathTestCases[i].origin_url),
                            kRootPathTestCases[i].type,
                            storage::OPEN_FILE_SYSTEM_FAIL_IF_NONEXISTENT,
                            &root_path));
    ASSERT_TRUE(returned_root_path.size() > i);
    EXPECT_EQ(returned_root_path[i].value(), root_path.value());
  }
}

TEST_F(SandboxFileSystemBackendTest,
       GetRootPathCreateAndExamineWithNewBackend) {
  std::vector<base::FilePath> returned_root_path(arraysize(kRootPathTestCases));
  SetUpNewBackend(CreateAllowFileAccessOptions());

  GURL origin_url("http://foo.com:1/");

  base::FilePath root_path1;
  EXPECT_TRUE(GetRootPath(origin_url,
                          storage::kFileSystemTypeTemporary,
                          storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
                          &root_path1));

  SetUpNewBackend(CreateDisallowFileAccessOptions());
  base::FilePath root_path2;
  EXPECT_TRUE(GetRootPath(origin_url,
                          storage::kFileSystemTypeTemporary,
                          storage::OPEN_FILE_SYSTEM_FAIL_IF_NONEXISTENT,
                          &root_path2));

  EXPECT_EQ(root_path1.value(), root_path2.value());
}

TEST_F(SandboxFileSystemBackendTest, GetRootPathGetWithoutCreate) {
  SetUpNewBackend(CreateDisallowFileAccessOptions());

  // Try to get a root directory without creating.
  for (size_t i = 0; i < arraysize(kRootPathTestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "RootPath (create=false) #" << i << " "
                 << kRootPathTestCases[i].expected_path);
    EXPECT_FALSE(GetRootPath(GURL(kRootPathTestCases[i].origin_url),
                             kRootPathTestCases[i].type,
                             storage::OPEN_FILE_SYSTEM_FAIL_IF_NONEXISTENT,
                             NULL));
  }
}

TEST_F(SandboxFileSystemBackendTest, GetRootPathInIncognito) {
  SetUpNewBackend(CreateIncognitoFileSystemOptions());

  // Try to get a root directory.
  for (size_t i = 0; i < arraysize(kRootPathTestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "RootPath (incognito) #" << i << " "
                 << kRootPathTestCases[i].expected_path);
    EXPECT_FALSE(GetRootPath(GURL(kRootPathTestCases[i].origin_url),
                             kRootPathTestCases[i].type,
                             storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
                             NULL));
  }
}

TEST_F(SandboxFileSystemBackendTest, GetRootPathFileURI) {
  SetUpNewBackend(CreateDisallowFileAccessOptions());
  for (size_t i = 0; i < arraysize(kRootPathFileURITestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "RootPathFileURI (disallow) #"
                 << i << " " << kRootPathFileURITestCases[i].expected_path);
    EXPECT_FALSE(GetRootPath(GURL(kRootPathFileURITestCases[i].origin_url),
                             kRootPathFileURITestCases[i].type,
                             storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
                             NULL));
  }
}

TEST_F(SandboxFileSystemBackendTest, GetRootPathFileURIWithAllowFlag) {
  SetUpNewBackend(CreateAllowFileAccessOptions());
  for (size_t i = 0; i < arraysize(kRootPathFileURITestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "RootPathFileURI (allow) #"
                 << i << " " << kRootPathFileURITestCases[i].expected_path);
    base::FilePath root_path;
    EXPECT_TRUE(GetRootPath(GURL(kRootPathFileURITestCases[i].origin_url),
                            kRootPathFileURITestCases[i].type,
                            storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
                            &root_path));
    base::FilePath expected = file_system_path().AppendASCII(
        kRootPathFileURITestCases[i].expected_path);
    EXPECT_EQ(expected.value(), root_path.value());
    EXPECT_TRUE(base::DirectoryExists(root_path));
  }
}

}  // namespace content
