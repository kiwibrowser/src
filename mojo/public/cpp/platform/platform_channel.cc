// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/platform/platform_channel.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>

#include "base/win/scoped_handle.h"
#elif defined(OS_FUCHSIA)
#include <zircon/process.h>
#include <zircon/processargs.h>
#include <zircon/syscalls.h>

#include "base/fuchsia/scoped_zx_handle.h"
#elif defined(OS_POSIX)
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/files/scoped_file.h"
#include "base/posix/global_descriptors.h"
#endif

#if defined(OS_POSIX) && !defined(OS_NACL_SFI)
#include <sys/socket.h>
#elif defined(OS_NACL_SFI)
#include "native_client/src/public/imc_syscalls.h"
#endif

namespace mojo {

namespace {

#if defined(OS_WIN)
void CreateChannel(PlatformHandle* local_endpoint,
                   PlatformHandle* remote_endpoint) {
  base::string16 pipe_name = base::UTF8ToUTF16(base::StringPrintf(
      "\\\\.\\pipe\\mojo.%lu.%lu.%I64u", ::GetCurrentProcessId(),
      ::GetCurrentThreadId(), base::RandUint64()));
  DWORD kOpenMode =
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE;
  const DWORD kPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE;
  *local_endpoint = PlatformHandle(base::win::ScopedHandle(
      ::CreateNamedPipeW(pipe_name.c_str(), kOpenMode, kPipeMode,
                         1,           // Max instances.
                         4096,        // Output buffer size.
                         4096,        // Input buffer size.
                         5000,        // Timeout in ms.
                         nullptr)));  // Default security descriptor.
  PCHECK(local_endpoint->is_valid());

  const DWORD kDesiredAccess = GENERIC_READ | GENERIC_WRITE;
  // The SECURITY_ANONYMOUS flag means that the server side cannot impersonate
  // the client.
  DWORD kFlags = SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS;
  // Allow the handle to be inherited by child processes.
  SECURITY_ATTRIBUTES security_attributes = {sizeof(SECURITY_ATTRIBUTES),
                                             nullptr, TRUE};
  *remote_endpoint = PlatformHandle(base::win::ScopedHandle(
      ::CreateFileW(pipe_name.c_str(), kDesiredAccess, 0, &security_attributes,
                    OPEN_EXISTING, kFlags, nullptr)));
  PCHECK(remote_endpoint->is_valid());

  // Since a client has connected, ConnectNamedPipe() should return zero and
  // GetLastError() should return ERROR_PIPE_CONNECTED.
  CHECK(!::ConnectNamedPipe(local_endpoint->GetHandle().Get(), nullptr));
  PCHECK(::GetLastError() == ERROR_PIPE_CONNECTED);
}
#elif defined(OS_FUCHSIA)
void CreateChannel(PlatformHandle* local_endpoint,
                   PlatformHandle* remote_endpoint) {
  zx_handle_t handles[2] = {};
  zx_status_t result = zx_channel_create(0, &handles[0], &handles[1]);
  CHECK_EQ(ZX_OK, result);

  *local_endpoint = PlatformHandle(base::ScopedZxHandle(handles[0]));
  *remote_endpoint = PlatformHandle(base::ScopedZxHandle(handles[1]));
  DCHECK(local_endpoint->is_valid());
  DCHECK(remote_endpoint->is_valid());
}
#elif defined(OS_POSIX)

#if defined(OS_ANDROID)
// Leave room for any other descriptors defined in content for example.
// TODO(https://crbug.com/676442): Consider changing base::GlobalDescriptors to
// generate a key when setting the file descriptor.
constexpr int kAndroidClientHandleDescriptor =
    base::GlobalDescriptors::kBaseDescriptor + 10000;
#else
bool IsTargetDescriptorUsed(const base::FileHandleMappingVector& mapping,
                            int target_fd) {
  for (size_t i = 0; i < mapping.size(); ++i) {
    if (mapping[i].second == target_fd)
      return true;
  }
  return false;
}
#endif

void CreateChannel(PlatformHandle* local_endpoint,
                   PlatformHandle* remote_endpoint) {
  int fds[2];
#if defined(OS_NACL_SFI)
  PCHECK(imc_socketpair(fds) == 0);
#else
  PCHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

  // Set non-blocking on both ends.
  PCHECK(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0);
  PCHECK(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

#if defined(OS_MACOSX)
  // This turns off |SIGPIPE| when writing to a closed socket, causing the call
  // to fail with |EPIPE| instead. On Linux we have to use |send...()| with
  // |MSG_NOSIGNAL| instead, which is not supported on Mac.
  int no_sigpipe = 1;
  PCHECK(setsockopt(fds[0], SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
                    sizeof(no_sigpipe)) == 0);
  PCHECK(setsockopt(fds[1], SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
                    sizeof(no_sigpipe)) == 0);
#endif  // defined(OS_MACOSX)
#endif  // defined(OS_NACL_SFI)

  *local_endpoint = PlatformHandle(base::ScopedFD(fds[0]));
  *remote_endpoint = PlatformHandle(base::ScopedFD(fds[1]));
  DCHECK(local_endpoint->is_valid());
  DCHECK(remote_endpoint->is_valid());
}
#else
#error "Unsupported platform."
#endif

}  // namespace

const char PlatformChannel::kHandleSwitch[] = "mojo-platform-channel-handle";

PlatformChannel::PlatformChannel() {
  PlatformHandle local_handle;
  PlatformHandle remote_handle;
  CreateChannel(&local_handle, &remote_handle);
  local_endpoint_ = PlatformChannelEndpoint(std::move(local_handle));
  remote_endpoint_ = PlatformChannelEndpoint(std::move(remote_handle));
}

PlatformChannel::PlatformChannel(PlatformChannel&& other) = default;

PlatformChannel::~PlatformChannel() = default;

PlatformChannel& PlatformChannel::operator=(PlatformChannel&& other) = default;

void PlatformChannel::PrepareToPassRemoteEndpoint(HandlePassingInfo* info,
                                                  std::string* value) {
  DCHECK(info);
  DCHECK(value);
  DCHECK(remote_endpoint_.is_valid());

#if defined(OS_WIN)
  info->push_back(remote_endpoint_.platform_handle().GetHandle().Get());
  *value = base::NumberToString(
      HandleToLong(remote_endpoint_.platform_handle().GetHandle().Get()));
#elif defined(OS_FUCHSIA)
  const uint32_t id = PA_HND(PA_USER0, 0);
  info->push_back({id, remote_endpoint_.platform_handle().GetHandle().get()});
  *value = base::NumberToString(id);
#elif defined(OS_ANDROID)
  int fd = remote_endpoint_.platform_handle().GetFD().get();
  info->emplace_back(fd, kAndroidClientHandleDescriptor);
  *value = base::NumberToString(kAndroidClientHandleDescriptor);
#elif defined(OS_POSIX)
  // Arbitrary sanity check to ensure the loop below terminates reasonably
  // quickly.
  CHECK_LT(info->size(), 1000u);

  // Find a suitable FD to map the remote endpoint handle to in the child
  // process. This has quadratic time complexity in the size of |*info|, but
  // |*info| should be very small and is usually empty.
  int target_fd = base::GlobalDescriptors::kBaseDescriptor;
  while (IsTargetDescriptorUsed(*info, target_fd))
    ++target_fd;
  info->emplace_back(remote_endpoint_.platform_handle().GetFD().get(),
                     target_fd);
  *value = base::NumberToString(target_fd);
#endif
}

void PlatformChannel::PrepareToPassRemoteEndpoint(
    HandlePassingInfo* info,
    base::CommandLine* command_line) {
  std::string value;
  PrepareToPassRemoteEndpoint(info, &value);
  if (!value.empty())
    command_line->AppendSwitchASCII(kHandleSwitch, value);
}

void PlatformChannel::RemoteProcessLaunched() {
#if defined(OS_FUCHSIA)
  // Unlike other platforms, Fuchsia just transfers handle ownership to the
  // launcher process rather than duplicating it.
  DCHECK(remote_endpoint_.platform_handle().is_valid_handle());
  ignore_result(remote_endpoint_.TakePlatformHandle().ReleaseHandle());
#else
  remote_endpoint_.reset();
#endif
}

// static
PlatformChannelEndpoint PlatformChannel::RecoverPassedEndpointFromString(
    base::StringPiece value) {
#if defined(OS_WIN)
  int handle_value = 0;
  if (value.empty() || !base::StringToInt(value, &handle_value)) {
    DLOG(ERROR) << "Invalid PlatformChannel endpoint string.";
    return PlatformChannelEndpoint();
  }
  return PlatformChannelEndpoint(
      PlatformHandle(base::win::ScopedHandle(LongToHandle(handle_value))));
#elif defined(OS_FUCHSIA)
  unsigned int handle_value = 0;
  if (value.empty() || !base::StringToUint(value, &handle_value)) {
    DLOG(ERROR) << "Invalid PlatformChannel endpoint string.";
    return PlatformChannelEndpoint();
  }
  return PlatformChannelEndpoint(PlatformHandle(base::ScopedZxHandle(
      zx_get_startup_handle(base::checked_cast<uint32_t>(handle_value)))));
#elif defined(OS_ANDROID)
  base::GlobalDescriptors::Key key = -1;
  if (value.empty() || !base::StringToUint(value, &key)) {
    DLOG(ERROR) << "Invalid PlatformChannel endpoint string.";
    return PlatformChannelEndpoint();
  }
  return PlatformChannelEndpoint(PlatformHandle(
      base::ScopedFD(base::GlobalDescriptors::GetInstance()->Get(key))));
#elif defined(OS_POSIX)
  int fd = -1;
  if (value.empty() || !base::StringToInt(value, &fd) ||
      fd < base::GlobalDescriptors::kBaseDescriptor) {
    DLOG(ERROR) << "Invalid PlatformChannel endpoint string.";
    return PlatformChannelEndpoint();
  }
  return PlatformChannelEndpoint(PlatformHandle(base::ScopedFD(fd)));
#endif
}

// static
PlatformChannelEndpoint PlatformChannel::RecoverPassedEndpointFromCommandLine(
    const base::CommandLine& command_line) {
  return RecoverPassedEndpointFromString(
      command_line.GetSwitchValueASCII(kHandleSwitch));
}

}  // namespace mojo
