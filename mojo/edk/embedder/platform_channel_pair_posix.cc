// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/platform_channel_pair.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include <limits>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/posix/global_descriptors.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/platform_handle.h"

#if !defined(OS_NACL_SFI)
#include <sys/socket.h>
#else
#include "native_client/src/public/imc_syscalls.h"
#endif

#if !defined(SO_PEEK_OFF)
#define SO_PEEK_OFF 42
#endif

namespace mojo {
namespace edk {

namespace {

#if defined(OS_ANDROID)
enum {
  // Leave room for any other descriptors defined in content for example.
  // TODO(jcivelli): consider changing base::GlobalDescriptors to generate a
  //   key when setting the file descriptor (http://crbug.com/676442).
  kAndroidClientHandleDescriptor =
      base::GlobalDescriptors::kBaseDescriptor + 10000,
};
#else
bool IsTargetDescriptorUsed(
    const base::FileHandleMappingVector& file_handle_mapping,
    int target_fd) {
  for (size_t i = 0; i < file_handle_mapping.size(); i++) {
    if (file_handle_mapping[i].second == target_fd)
      return true;
  }
  return false;
}
#endif

}  // namespace

PlatformChannelPair::PlatformChannelPair(bool client_is_blocking) {
  // Create the Unix domain socket.
  int fds[2];
  // TODO(vtl): Maybe fail gracefully if |socketpair()| fails.

#if defined(OS_NACL_SFI)
  PCHECK(imc_socketpair(fds) == 0);
#else
  PCHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

  // Set the ends to nonblocking.
  PCHECK(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0);
  if (!client_is_blocking)
    PCHECK(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

#if defined(OS_MACOSX)
  // This turns off |SIGPIPE| when writing to a closed socket (causing it to
  // fail with |EPIPE| instead). On Linux, we have to use |send...()| with
  // |MSG_NOSIGNAL| -- which is not supported on Mac -- instead.
  int no_sigpipe = 1;
  PCHECK(setsockopt(fds[0], SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
                    sizeof(no_sigpipe)) == 0);
  PCHECK(setsockopt(fds[1], SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
                    sizeof(no_sigpipe)) == 0);
#endif  // defined(OS_MACOSX)
#endif  // defined(OS_NACL_SFI)

  server_handle_.reset(InternalPlatformHandle(fds[0]));
  DCHECK(server_handle_.is_valid());
  client_handle_.reset(InternalPlatformHandle(fds[1]));
  DCHECK(client_handle_.is_valid());
}

// static
ScopedInternalPlatformHandle
PlatformChannelPair::PassClientHandleFromParentProcess(
    const base::CommandLine& command_line) {
  std::string client_fd_string =
      command_line.GetSwitchValueASCII(kMojoPlatformChannelHandleSwitch);
  return PassClientHandleFromParentProcessFromString(client_fd_string);
}

ScopedInternalPlatformHandle
PlatformChannelPair::PassClientHandleFromParentProcessFromString(
    const std::string& value) {
  int client_fd = -1;
#if defined(OS_ANDROID)
  base::GlobalDescriptors::Key key = -1;
  if (value.empty() || !base::StringToUint(value, &key)) {
    LOG(ERROR) << "Missing or invalid --" << kMojoPlatformChannelHandleSwitch;
    return ScopedInternalPlatformHandle();
  }
  client_fd = base::GlobalDescriptors::GetInstance()->Get(key);
#else
  if (value.empty() ||
      !base::StringToInt(value, &client_fd) ||
      client_fd < base::GlobalDescriptors::kBaseDescriptor) {
    LOG(ERROR) << "Missing or invalid --" << kMojoPlatformChannelHandleSwitch;
    return ScopedInternalPlatformHandle();
  }
#endif
  return ScopedInternalPlatformHandle(InternalPlatformHandle(client_fd));
}

void PlatformChannelPair::PrepareToPassClientHandleToChildProcess(
    base::CommandLine* command_line,
    base::FileHandleMappingVector* handle_passing_info) const {
  DCHECK(command_line);

  // Log a warning if the command line already has the switch, but "clobber" it
  // anyway, since it's reasonably likely that all the switches were just copied
  // from the parent.
  LOG_IF(WARNING, command_line->HasSwitch(kMojoPlatformChannelHandleSwitch))
      << "Child command line already has switch --"
      << kMojoPlatformChannelHandleSwitch << "="
      << command_line->GetSwitchValueASCII(kMojoPlatformChannelHandleSwitch);
  // (Any existing switch won't actually be removed from the command line, but
  // the last one appended takes precedence.)
  command_line->AppendSwitchASCII(
      kMojoPlatformChannelHandleSwitch,
      PrepareToPassClientHandleToChildProcessAsString(handle_passing_info));
}

std::string
PlatformChannelPair::PrepareToPassClientHandleToChildProcessAsString(
      HandlePassingInformation* handle_passing_info) const {
#if defined(OS_ANDROID)
  int fd = client_handle_.get().handle;
  handle_passing_info->push_back(
      std::pair<int, int>(fd, kAndroidClientHandleDescriptor));
  return base::UintToString(kAndroidClientHandleDescriptor);
#else
  DCHECK(handle_passing_info);
  // This is an arbitrary sanity check. (Note that this guarantees that the loop
  // below will terminate sanely.)
  CHECK_LT(handle_passing_info->size(), 1000u);

  DCHECK(client_handle_.is_valid());

  // Find a suitable FD to map our client handle to in the child process.
  // This has quadratic time complexity in the size of |*handle_passing_info|,
  // but |*handle_passing_info| should be very small (usually/often empty).
  int target_fd = base::GlobalDescriptors::kBaseDescriptor;
  while (IsTargetDescriptorUsed(*handle_passing_info, target_fd))
    target_fd++;

  handle_passing_info->push_back(
      std::pair<int, int>(client_handle_.get().handle, target_fd));
  return base::IntToString(target_fd);
#endif
}

}  // namespace edk
}  // namespace mojo
