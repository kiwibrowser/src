// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/platform_channel_pair.h"

#include <zircon/process.h>
#include <zircon/processargs.h>
#include <zircon/syscalls.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "mojo/edk/embedder/platform_handle.h"

namespace mojo {
namespace edk {

namespace {

std::string PrepareToPassHandleToChildProcessAsString(
    const InternalPlatformHandle& handle,
    HandlePassingInformation* handle_passing_info) {
  DCHECK(handle.is_valid());

  const uint32_t id = PA_HND(PA_USER0, 0);
  handle_passing_info->push_back({id, handle.as_handle()});

  return base::UintToString(id);
}

}  // namespace

PlatformChannelPair::PlatformChannelPair(bool client_is_blocking) {
  zx_handle_t handles[2] = {};
  zx_status_t result = zx_channel_create(0, &handles[0], &handles[1]);
  CHECK_EQ(ZX_OK, result);

  server_handle_.reset(InternalPlatformHandle::ForHandle(handles[0]));
  DCHECK(server_handle_.is_valid());
  client_handle_.reset(InternalPlatformHandle::ForHandle(handles[1]));
  DCHECK(client_handle_.is_valid());
}

// static
ScopedInternalPlatformHandle
PlatformChannelPair::PassClientHandleFromParentProcess(
    const base::CommandLine& command_line) {
  std::string handle_string =
      command_line.GetSwitchValueASCII(kMojoPlatformChannelHandleSwitch);
  return PassClientHandleFromParentProcessFromString(handle_string);
}

ScopedInternalPlatformHandle
PlatformChannelPair::PassClientHandleFromParentProcessFromString(
    const std::string& value) {
  unsigned int id = 0;
  if (value.empty() || !base::StringToUint(value, &id)) {
    LOG(ERROR) << "Missing or invalid --" << kMojoPlatformChannelHandleSwitch;
    return ScopedInternalPlatformHandle();
  }
  return ScopedInternalPlatformHandle(InternalPlatformHandle::ForHandle(
      zx_get_startup_handle(base::checked_cast<uint32_t>(id))));
}

void PlatformChannelPair::PrepareToPassClientHandleToChildProcess(
    base::CommandLine* command_line,
    HandlePassingInformation* handle_passing_info) const {
  return PrepareToPassHandleToChildProcess(client_handle_.get(), command_line,
                                           handle_passing_info);
}

// static
void PlatformChannelPair::PrepareToPassHandleToChildProcess(
    const InternalPlatformHandle& handle,
    base::CommandLine* command_line,
    HandlePassingInformation* handle_passing_info) {
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
      PrepareToPassHandleToChildProcessAsString(handle, handle_passing_info));
}

}  // namespace edk
}  // namespace mojo
