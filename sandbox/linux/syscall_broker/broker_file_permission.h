// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_LINUX_SYSCALL_BROKER_BROKER_FILE_PERMISSION_H_
#define SANDBOX_LINUX_SYSCALL_BROKER_BROKER_FILE_PERMISSION_H_

#include <string>

#include "base/macros.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
namespace syscall_broker {

// BrokerFilePermission defines a path for whitelisting.
// Pick the correct static factory method to create a permission.
// CheckOpen and CheckAccess are async signal safe.
// Constuction and Destruction are not async signal safe.
// |path| is the path to be whitelisted.
class SANDBOX_EXPORT BrokerFilePermission {
 public:
  ~BrokerFilePermission() {}
  BrokerFilePermission(const BrokerFilePermission&) = default;
  BrokerFilePermission& operator=(const BrokerFilePermission&) = default;

  static BrokerFilePermission ReadOnly(const std::string& path) {
    return BrokerFilePermission(path, false, false, true, false, false);
  }

  static BrokerFilePermission ReadOnlyRecursive(const std::string& path) {
    return BrokerFilePermission(path, true, false, true, false, false);
  }

  static BrokerFilePermission WriteOnly(const std::string& path) {
    return BrokerFilePermission(path, false, false, false, true, false);
  }

  static BrokerFilePermission ReadWrite(const std::string& path) {
    return BrokerFilePermission(path, false, false, true, true, false);
  }

  static BrokerFilePermission ReadWriteCreate(const std::string& path) {
    return BrokerFilePermission(path, false, false, true, true, true);
  }

  static BrokerFilePermission ReadWriteCreateRecursive(
      const std::string& path) {
    return BrokerFilePermission(path, true, false, true, true, true);
  }

  // Temporary files must always be newly created and do not confer rights to
  // use pre-existing files of the same name.
  static BrokerFilePermission ReadWriteCreateTemporary(
      const std::string& path) {
    return BrokerFilePermission(path, false, true, true, true, true);
  }

  static BrokerFilePermission ReadWriteCreateTemporaryRecursive(
      const std::string& path) {
    return BrokerFilePermission(path, true, true, true, true, true);
  }

  // Returns true if |requested_filename| is allowed to be accessed
  // by this permission as per access(2).
  // If |file_to_access| is not NULL, it is set to point to either
  // the |requested_filename| in the case of a recursive match,
  // or a pointer to the matched path in the whitelist if an absolute
  // match.
  // |mode| is per mode argument of access(2).
  // Async signal safe if |file_to_access| is NULL
  bool CheckAccess(const char* requested_filename,
                   int mode,
                   const char** file_to_access) const;

  // Returns true if |requested_filename| is allowed to be opened
  // by this permission.
  // If |file_to_open| is not NULL it is set to point to either
  // the |requested_filename| in the case of a recursive match,
  // or a pointer the matched path in the whitelist if an absolute
  // match.
  // If not NULL, |unlink_after_open| is set to point to true if the
  // caller is required to unlink the path after opening.
  // Async signal safe if |file_to_open| is NULL.
  bool CheckOpen(const char* requested_filename,
                 int flags,
                 const char** file_to_open,
                 bool* unlink_after_open) const;

  // Returns true if |requested_filename| is allowed to be stat'd
  // by this permission as per stat(2). Differs from CheckAccess()
  // in that if create permission is granted to a file, we permit
  // stat() on all of its leading components.
  // If |file_to_open| is not NULL, it is set to point to either
  // the |requested_filename| in the case of a recursive match,
  // or a pointer to the matched path in the whitelist if an absolute
  // match.
  // Async signal safe if |file_to_access| is NULL
  bool CheckStat(const char* requested_filename,
                 const char** file_to_access) const;

 private:
  friend class BrokerFilePermissionTester;

  // NOTE: Validates the permission and dies if invalid!
  BrokerFilePermission(const std::string& path,
                       bool recursive,
                       bool temporary_only,
                       bool allow_read,
                       bool allow_write,
                       bool allow_create);

  // ValidatePath checks |path| and returns true if these conditions are met
  // * Greater than 0 length
  // * Is an absolute path
  // * No trailing slash
  // * No /../ path traversal
  static bool ValidatePath(const char* path);

  // MatchPath returns true if |requested_filename| is covered by this instance
  bool MatchPath(const char* requested_filename) const;

  // Helper routine for CheckAccess() and CheckStat(). Must be safe to call
  // from an async signal context.
  bool CheckAccessInternal(const char* requested_filename,
                           int mode,
                           const char** file_to_access) const;

  // Used in by BrokerFilePermissionTester for tests.
  static const char* GetErrorMessageForTests();

  // These are not const as std::vector requires copy-assignment and this class
  // is stored in vectors. All methods are marked const so the compiler will
  // still enforce no changes outside of the constructor.
  std::string path_;
  bool recursive_;       // Allow everything under |path| (must be a dir).
  bool temporary_only_;  // File must be unlink'd after opening.
  bool allow_read_;
  bool allow_write_;
  bool allow_create_;
};

}  // namespace syscall_broker
}  // namespace sandbox

#endif  //  SANDBOX_LINUX_SYSCALL_BROKER_BROKER_FILE_PERMISSION_H_
