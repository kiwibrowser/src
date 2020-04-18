// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_NAMED_PLATFORM_CHANNEL_PAIR_H_
#define MOJO_EDK_EMBEDDER_NAMED_PLATFORM_CHANNEL_PAIR_H_

#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/system_impl_export.h"

namespace base {
class CommandLine;
}

namespace mojo {
namespace edk {

// This is used to create a named bidirectional pipe to connect new child
// processes. The resulting server handle should be passed to the EDK, and the
// child end passed as a pipe name on the command line to the child process. The
// child process can then retrieve the pipe name from the command line and
// resolve it into a client handle.
class MOJO_SYSTEM_IMPL_EXPORT NamedPlatformChannelPair {
 public:
  struct Options {
#if defined(OS_WIN)
    // If non-empty, a security descriptor to use when creating the pipe. If
    // empty, a default security descriptor will be used. See
    // kDefaultSecurityDescriptor in named_platform_handle_utils_win.cc.
    base::string16 security_descriptor;
#else
    // On POSIX, every new NamedPlatformChannelPair creates a new server socket
    // with a random name. This controls the directory where that happens.
    base::FilePath socket_dir;
#endif
  };

  NamedPlatformChannelPair(const Options& options = {});
  ~NamedPlatformChannelPair();

  // Note: It is NOT acceptable to use this handle as a generic pipe channel. It
  // MUST be passed to OutgoingBrokerClientInvitation::Send() only.
  ScopedInternalPlatformHandle PassServerHandle();

  // To be called in the child process, after the parent process called
  // |PrepareToPassClientHandleToChildProcess()| and launched the child (using
  // the provided data), to create a client handle connected to the server
  // handle (in the parent process).
  static ScopedInternalPlatformHandle PassClientHandleFromParentProcess(
      const base::CommandLine& command_line);

  // Prepares to pass the client channel to a new child process, to be launched
  // using |LaunchProcess()| (from base/launch.h). Modifies |*command_line| and
  // |*handle_passing_info| as needed.
  // Note: For Windows, this method only works on Vista and later.
  void PrepareToPassClientHandleToChildProcess(
      base::CommandLine* command_line) const;

  const NamedPlatformHandle& handle() const { return pipe_handle_; }

 private:
  NamedPlatformHandle pipe_handle_;
  ScopedInternalPlatformHandle server_handle_;

  DISALLOW_COPY_AND_ASSIGN(NamedPlatformChannelPair);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_NAMED_PLATFORM_CHANNEL_PAIR_H_
