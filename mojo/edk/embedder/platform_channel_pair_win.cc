// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/platform_channel_pair.h"

#include <windows.h>

#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "mojo/edk/embedder/platform_handle.h"

namespace mojo {
namespace edk {

namespace {

std::wstring GeneratePipeName() {
  return base::StringPrintf(L"\\\\.\\pipe\\mojo.%u.%u.%I64u",
                            GetCurrentProcessId(), GetCurrentThreadId(),
                            base::RandUint64());
}

}  // namespace

PlatformChannelPair::PlatformChannelPair(bool client_is_blocking) {
  std::wstring pipe_name = GeneratePipeName();

  DWORD kOpenMode =
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE;
  const DWORD kPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE;
  server_handle_.reset(InternalPlatformHandle(
      CreateNamedPipeW(pipe_name.c_str(), kOpenMode, kPipeMode,
                       1,           // Max instances.
                       4096,        // Out buffer size.
                       4096,        // In buffer size.
                       5000,        // Timeout in milliseconds.
                       nullptr)));  // Default security descriptor.
  PCHECK(server_handle_.is_valid());

  const DWORD kDesiredAccess = GENERIC_READ | GENERIC_WRITE;
  // The SECURITY_ANONYMOUS flag means that the server side cannot impersonate
  // the client.
  DWORD kFlags = SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS;
  if (!client_is_blocking)
    kFlags |= FILE_FLAG_OVERLAPPED;
  // Allow the handle to be inherited by child processes.
  SECURITY_ATTRIBUTES security_attributes = {
      sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
  client_handle_.reset(InternalPlatformHandle(
      CreateFileW(pipe_name.c_str(), kDesiredAccess,
                  0,  // No sharing.
                  &security_attributes, OPEN_EXISTING, kFlags,
                  nullptr)));  // No template file.
  PCHECK(client_handle_.is_valid());

  // Since a client has connected, ConnectNamedPipe() should return zero and
  // GetLastError() should return ERROR_PIPE_CONNECTED.
  CHECK(!ConnectNamedPipe(server_handle_.get().handle, nullptr));
  PCHECK(GetLastError() == ERROR_PIPE_CONNECTED);
}

// static
ScopedInternalPlatformHandle
PlatformChannelPair::PassClientHandleFromParentProcess(
    const base::CommandLine& command_line) {
  std::string client_handle_string =
      command_line.GetSwitchValueASCII(kMojoPlatformChannelHandleSwitch);
  return PassClientHandleFromParentProcessFromString(client_handle_string);
}

ScopedInternalPlatformHandle
PlatformChannelPair::PassClientHandleFromParentProcessFromString(
    const std::string& value) {
  int client_handle_value = 0;
  if (value.empty() ||
      !base::StringToInt(value, &client_handle_value)) {
    LOG(ERROR) << "Missing or invalid --" << kMojoPlatformChannelHandleSwitch;
    return ScopedInternalPlatformHandle();
  }

  return ScopedInternalPlatformHandle(
      InternalPlatformHandle(LongToHandle(client_handle_value)));
}

void PlatformChannelPair::PrepareToPassClientHandleToChildProcess(
    base::CommandLine* command_line,
    base::HandlesToInheritVector* handle_passing_info) const {
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
  DCHECK(handle_passing_info);
  DCHECK(client_handle_.is_valid());

  handle_passing_info->push_back(client_handle_.get().handle);

  return base::IntToString(HandleToLong(client_handle_.get().handle));
}

}  // namespace edk
}  // namespace mojo
