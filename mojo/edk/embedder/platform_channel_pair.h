// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_PLATFORM_CHANNEL_PAIR_H_
#define MOJO_EDK_EMBEDDER_PLATFORM_CHANNEL_PAIR_H_

#include <memory>

#include "base/macros.h"
#include "base/process/launch.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/system_impl_export.h"

namespace base {
class CommandLine;
}

namespace mojo {
namespace edk {

// It would be nice to refactor base/process/launch.h to have a more platform-
// independent way of representing handles that are passed to child processes.
#if defined(OS_WIN)
using HandlePassingInformation = base::HandlesToInheritVector;
#elif defined(OS_FUCHSIA)
using HandlePassingInformation = base::HandlesToTransferVector;
#elif defined(OS_POSIX)
using HandlePassingInformation = base::FileHandleMappingVector;
#else
#error "Unsupported."
#endif

// This is used to create a pair of |InternalPlatformHandle|s that are connected
// by a suitable (platform-specific) bidirectional "pipe" (e.g., socket on
// POSIX, named pipe on Windows). The resulting handles can then be used in the
// same process (e.g., in tests) or between processes. (The "server" handle is
// the one that will be used in the process that created the pair, whereas the
// "client" handle is the one that will be used in a different process.)
//
// This class provides facilities for passing the client handle to a child
// process. The parent should call |PrepareToPassClientHandlelToChildProcess()|
// to get the data needed to do this, spawn the child using that data, and then
// call |ChildProcessLaunched()|. Note that on Windows this facility (will) only
// work on Vista and later (TODO(vtl)).
//
// Note: |PlatformChannelPair()|, |PassClientHandleFromParentProcess()| and
// |PrepareToPassClientHandleToChildProcess()| have platform-specific
// implementations.
//
// Note: On POSIX platforms, to write to the "pipe", use
// |PlatformChannel{Write,Writev}()| (from platform_channel_utils_posix.h)
// instead of |write()|, |writev()|, etc. Otherwise, you have to worry about
// platform differences in suppressing |SIGPIPE|.
class MOJO_SYSTEM_IMPL_EXPORT PlatformChannelPair {
 public:
  static const char kMojoPlatformChannelHandleSwitch[];

  // If |client_is_blocking| is true, then the client handle only supports
  // blocking reads and writes. The default is nonblocking.
  PlatformChannelPair(bool client_is_blocking = false);
  ~PlatformChannelPair();

  ScopedInternalPlatformHandle PassServerHandle();

  // For in-process use (e.g., in tests or to pass over another channel).
  ScopedInternalPlatformHandle PassClientHandle();

  // To be called in the child process, after the parent process called
  // |PrepareToPassClientHandleToChildProcess()| and launched the child (using
  // the provided data), to create a client handle connected to the server
  // handle (in the parent process).
  // TODO(jcivelli): remove the command_line param. http://crbug.com/670106
  static ScopedInternalPlatformHandle PassClientHandleFromParentProcess(
      const base::CommandLine& command_line);

  // Like above, but gets the handle from the passed in string.
  static ScopedInternalPlatformHandle
  PassClientHandleFromParentProcessFromString(const std::string& value);

  // Prepares to pass the client channel to a new child process, to be launched
  // using |LaunchProcess()| (from base/launch.h). Modifies |*command_line| and
  // |*handle_passing_info| as needed.
  // Note: For Windows, this method only works on Vista and later.
  void PrepareToPassClientHandleToChildProcess(
      base::CommandLine* command_line,
      HandlePassingInformation* handle_passing_info) const;

  // Like above, but returns a string instead of changing the command line.
  std::string PrepareToPassClientHandleToChildProcessAsString(
      HandlePassingInformation* handle_passing_info) const;

#if defined(OS_FUCHSIA)
  // Like above, but accepts a caller-supplied client |handle|.
  // TODO(wez): Consider incorporating this call into other platform
  // implementations.
  static void PrepareToPassHandleToChildProcess(
      const InternalPlatformHandle& handle,
      base::CommandLine* command_line,
      HandlePassingInformation* handle_passing_info);
#endif

  // To be called once the child process has been successfully launched, to do
  // any cleanup necessary.
  void ChildProcessLaunched();

 private:
  ScopedInternalPlatformHandle server_handle_;
  ScopedInternalPlatformHandle client_handle_;

  DISALLOW_COPY_AND_ASSIGN(PlatformChannelPair);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_PLATFORM_CHANNEL_PAIR_H_
