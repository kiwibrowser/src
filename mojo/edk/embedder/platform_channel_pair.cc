// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/platform_channel_pair.h"

#include <utility>

#include "base/logging.h"
#include "build/build_config.h"

namespace mojo {
namespace edk {

const char PlatformChannelPair::kMojoPlatformChannelHandleSwitch[] =
    "mojo-platform-channel-handle";

PlatformChannelPair::~PlatformChannelPair() {
}

ScopedInternalPlatformHandle PlatformChannelPair::PassServerHandle() {
  return std::move(server_handle_);
}

ScopedInternalPlatformHandle PlatformChannelPair::PassClientHandle() {
  return std::move(client_handle_);
}

void PlatformChannelPair::ChildProcessLaunched() {
  DCHECK(client_handle_.is_valid());
#if defined(OS_FUCHSIA)
  // The |client_handle_| is transferred, not cloned, to the child.
  ignore_result(client_handle_.release());
#else
  client_handle_.reset();
#endif
}

}  // namespace edk
}  // namespace mojo
