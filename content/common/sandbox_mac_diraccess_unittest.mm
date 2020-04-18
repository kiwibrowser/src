// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include <dirent.h>
#include <stddef.h>

extern "C" {
#include <sandbox.h>
}

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/process/kill.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/multiprocess_test.h"
#include "sandbox/mac/sandbox_compiler.h"
#include "services/service_manager/sandbox/mac/sandbox_mac.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace {

static const char* kSandboxAccessPathKey = "sandbox_dir";
static const char* kDeniedSuffix = "_denied";

}  // namespace

// Tests need to be in the same namespace as the Sandbox class to be useable
// with FRIEND_TEST() declaration.
namespace content {

class MacDirAccessSandboxTest : public base::MultiProcessTest {
 public:
  bool CheckSandbox(const std::string& directory_to_try) {
    setenv(kSandboxAccessPathKey, directory_to_try.c_str(), 1);
    base::Process child_process = SpawnChild("mac_sandbox_path_access");
    if (!child_process.IsValid()) {
      LOG(WARNING) << "SpawnChild failed";
      return false;
    }
    int code = -1;
    if (!child_process.WaitForExit(&code)) {
      LOG(WARNING) << "Process::WaitForExit failed";
      return false;
    }
    return code == 0;
  }
};

// A class to handle auto-deleting a directory.
struct ScopedDirectoryDelete {
  inline void operator()(base::FilePath* x) const {
    if (x)
      base::DeleteFile(*x, true);
  }
};

typedef std::unique_ptr<base::FilePath, ScopedDirectoryDelete> ScopedDirectory;

TEST_F(MacDirAccessSandboxTest, SandboxAccess) {
  using base::CreateDirectory;

  base::FilePath tmp_dir;
  ASSERT_TRUE(base::CreateNewTempDirectory(base::FilePath::StringType(),
                                           &tmp_dir));
  // This step is important on OS X since the sandbox only understands "real"
  // paths and the paths CreateNewTempDirectory() returns are empirically in
  // /var which is a symlink to /private/var .
  tmp_dir = service_manager::SandboxMac::GetCanonicalPath(tmp_dir);
  ScopedDirectory cleanup(&tmp_dir);

  const char* sandbox_dir_cases[] = {
      "simple_dir_name",
      "^hello++ $",       // Regex.
      "\\^.$|()[]*+?{}",  // All regex characters.
      "\n",
      "\tfile\b",
      "ÖÖÖÖÖ",
      "ȓȓȓȓȓ",
  };

  for (size_t i = 0; i < arraysize(sandbox_dir_cases); ++i) {
    const char* sandbox_dir_name = sandbox_dir_cases[i];
    base::FilePath sandbox_dir = tmp_dir.Append(sandbox_dir_name);
    ASSERT_TRUE(CreateDirectory(sandbox_dir));
    ScopedDirectory cleanup_sandbox(&sandbox_dir);

    // Create a sibling directory of the sandbox dir, whose name has sandbox dir
    // as a substring but to which access is denied.
    std::string sibling_sandbox_dir_name_denied =
        std::string(sandbox_dir_cases[i]) + kDeniedSuffix;
    base::FilePath sibling_sandbox_dir = tmp_dir.Append(
                                      sibling_sandbox_dir_name_denied.c_str());
    ASSERT_TRUE(CreateDirectory(sibling_sandbox_dir));
    ScopedDirectory cleanup_sandbox_sibling(&sibling_sandbox_dir);

    EXPECT_TRUE(CheckSandbox(sandbox_dir.value()));
  }
}

MULTIPROCESS_TEST_MAIN(mac_sandbox_path_access) {
  char *sandbox_allowed_dir = getenv(kSandboxAccessPathKey);
  if (!sandbox_allowed_dir)
    return -1;

  // Build up a sandbox profile that only allows access to a single directory.
  std::string sandbox_profile =
      "(version 1)"
      "(define perm_dir (param \"PERMITTED_DIR\"))"
      "(deny default)"
      "(allow signal (target self))"
      "(allow sysctl-read)"
      "(if (string? perm_dir)"
      "    (begin"
      "       (allow file-read-metadata )"
      "       (allow file-read* file-write* (subpath perm_dir))))";

  // Setup the parameters to pass to the sandbox.
  sandbox::SandboxCompiler compiler(sandbox_profile);
  CHECK(compiler.InsertStringParam("PERMITTED_DIR", sandbox_allowed_dir));

  // Enable Sandbox.
  std::string error_str;
  if (!compiler.CompileAndApplyProfile(&error_str)) {
    LOG(ERROR) << "Failed to Initialize Sandbox: " << error_str;
    return -1;
  }

  // Test Sandbox.

  // We should be able to list the contents of the sandboxed directory.
  DIR *file_list = NULL;
  file_list = opendir(sandbox_allowed_dir);
  if (!file_list) {
    PLOG(ERROR) << "Sandbox overly restrictive: call to opendir("
                << sandbox_allowed_dir
                << ") failed";
    return -1;
  }
  closedir(file_list);

  // Test restrictions on accessing files.
  base::FilePath allowed_dir_path(sandbox_allowed_dir);
  base::FilePath allowed_file = allowed_dir_path.Append("ok_to_write");
  base::FilePath denied_file1 =
      allowed_dir_path.DirName().Append("cant_access");

  // Try to write a file who's name has the same prefix as the directory we
  // allow access to.
  base::FilePath basename = allowed_dir_path.BaseName();
  base::FilePath allowed_parent_dir = allowed_dir_path.DirName();
  std::string tricky_filename = basename.value() + "123";
  base::FilePath denied_file2 =  allowed_parent_dir.Append(tricky_filename);

  if (open(allowed_file.value().c_str(), O_WRONLY | O_CREAT) <= 0) {
    PLOG(ERROR) << "Sandbox overly restrictive: failed to write ("
                << allowed_file.value()
                << ")";
    return -1;
  }

  // Test that we deny access to a sibling of the sandboxed directory whose
  // name has the sandboxed directory name as a substring. e.g. if the sandbox
  // directory is /foo/baz then test /foo/baz_denied.
  {
    struct stat tmp_stat_info;
    std::string denied_sibling =
        std::string(sandbox_allowed_dir) + kDeniedSuffix;
    if (stat(denied_sibling.c_str(), &tmp_stat_info) > 0) {
      PLOG(ERROR) << "Sandbox breach: was able to stat ("
                  << denied_sibling.c_str()
                  << ")";
      return -1;
    }
  }

  // Test that we can stat parent directories of the "allowed" directory.
  {
    struct stat tmp_stat_info;
    if (stat(allowed_parent_dir.value().c_str(), &tmp_stat_info) != 0) {
      PLOG(ERROR) << "Sandbox overly restrictive: unable to stat ("
                  << allowed_parent_dir.value()
                  << ")";
      return -1;
    }
  }

  // Test that we can't stat files outside the "allowed" directory.
  {
    struct stat tmp_stat_info;
    if (stat(denied_file1.value().c_str(), &tmp_stat_info) > 0) {
      PLOG(ERROR) << "Sandbox breach: was able to stat ("
                  << denied_file1.value()
                  << ")";
      return -1;
    }
  }

  if (open(denied_file1.value().c_str(), O_WRONLY | O_CREAT) > 0) {
    PLOG(ERROR) << "Sandbox breach: was able to write ("
                << denied_file1.value()
                << ")";
    return -1;
  }

  if (open(denied_file2.value().c_str(), O_WRONLY | O_CREAT) > 0) {
    PLOG(ERROR) << "Sandbox breach: was able to write ("
                << denied_file2.value()
                << ")";
    return -1;
  }

  return 0;
}

}  // namespace content
