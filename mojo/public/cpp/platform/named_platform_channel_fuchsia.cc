// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/platform/named_platform_channel.h"

namespace mojo {

// static
PlatformChannelServerEndpoint NamedPlatformChannel::CreateServerEndpoint(
    const Options& options,
    ServerName* server_name) {
  // TODO(https://crbug.com/754038): Implement, or remove dependencies.
  NOTREACHED();
  return {};
}

// static
PlatformChannelEndpoint NamedPlatformChannel::CreateClientEndpoint(
    const ServerName& server_name) {
  // TODO(https://crbug.com/754038): Implement, or remove dependencies.
  NOTREACHED();
  return {};
}

}  // namespace mojo
