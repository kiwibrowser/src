// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_CONNECTION_PARAMS_H_
#define MOJO_EDK_EMBEDDER_CONNECTION_PARAMS_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/embedder/transport_protocol.h"
#include "mojo/edk/system/system_impl_export.h"

namespace mojo {
namespace edk {

// A set of parameters used when establishing a connection to another process.
class MOJO_SYSTEM_IMPL_EXPORT ConnectionParams {
 public:
  // Configures an OS pipe-based connection of type |type| to the remote process
  // using the given transport |protocol|.
  ConnectionParams(TransportProtocol protocol,
                   ScopedInternalPlatformHandle channel);

  ConnectionParams(ConnectionParams&& params);
  ConnectionParams& operator=(ConnectionParams&& params);

  TransportProtocol protocol() const { return protocol_; }

  ScopedInternalPlatformHandle TakeChannelHandle();

 private:
  TransportProtocol protocol_;
  ScopedInternalPlatformHandle channel_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionParams);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_CONNECTION_PARAMS_H_
