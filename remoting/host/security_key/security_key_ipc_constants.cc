// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/security_key/security_key_ipc_constants.h"

#include "base/lazy_instance.h"
#include "build/build_config.h"

#if defined(OS_POSIX)
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#endif  // defined(OS_POSIX)

namespace {
base::LazyInstance<mojo::edk::NamedPlatformHandle>::DestructorAtExit
    g_security_key_ipc_channel_name = LAZY_INSTANCE_INITIALIZER;

constexpr char kSecurityKeyIpcChannelName[] = "security_key_ipc_channel";

}  // namespace

namespace remoting {

const char kSecurityKeyConnectionError[] = "ssh_connection_error";

const mojo::edk::NamedPlatformHandle& GetSecurityKeyIpcChannel() {
  if (!g_security_key_ipc_channel_name.Get().is_valid()) {
    g_security_key_ipc_channel_name.Get() =
        mojo::edk::NamedPlatformHandle(kSecurityKeyIpcChannelName);
  }

  return g_security_key_ipc_channel_name.Get();
}

void SetSecurityKeyIpcChannelForTest(
    const mojo::edk::NamedPlatformHandle& channel_handle) {
  g_security_key_ipc_channel_name.Get() = channel_handle;
}

std::string GetChannelNamePathPrefixForTest() {
  std::string base_path;
#if defined(OS_POSIX)
  base::FilePath base_file_path;
  if (base::GetTempDir(&base_file_path)) {
    base_path = base_file_path.AsEndingWithSeparator().value();
  } else {
    LOG(ERROR) << "Failed to retrieve temporary directory.";
  }
#endif  // defined(OS_POSIX)
  return base_path;
}

}  // namespace remoting
