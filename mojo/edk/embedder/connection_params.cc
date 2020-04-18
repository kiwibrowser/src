// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/connection_params.h"

#include <utility>

#include "base/logging.h"

namespace mojo {
namespace edk {

ConnectionParams::ConnectionParams(TransportProtocol protocol,
                                   ScopedInternalPlatformHandle channel)
    : protocol_(protocol), channel_(std::move(channel)) {
  // TODO(rockot): Support other protocols.
  DCHECK_EQ(TransportProtocol::kLegacy, protocol);
}

ConnectionParams::ConnectionParams(ConnectionParams&& params) {
  *this = std::move(params);
}

ConnectionParams& ConnectionParams::operator=(ConnectionParams&& params) =
    default;

ScopedInternalPlatformHandle ConnectionParams::TakeChannelHandle() {
  return std::move(channel_);
}

}  // namespace edk
}  // namespace mojo
