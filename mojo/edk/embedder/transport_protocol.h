// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_TRANSPORT_PROTOCOL_H_
#define MOJO_EDK_EMBEDDER_TRANSPORT_PROTOCOL_H_

#include <stdint.h>

namespace mojo {
namespace edk {

// Selects the transport protocol to use when connecting to the remote
// process.
//
// New code should generally prefer to use |kAutomatic| to allow the protocol
// to be negotiated by the two processes involved in the connection. As noted
// below, this is not possible when either one of the two processes only
// supports the legacy protocol. In such cases both ends of the connection
// must be aware and explicitly use |kLegacy|, which is the default for now.
enum class TransportProtocol : int32_t {
  // Legacy transport protocol. Should only be used when connecting to
  // embedders which may not support at least |kVersion0|. Deprecated.
  kLegacy = -2,

  // Automatically negotiate the transport protocol. This will ultimately
  // select the highest protocol version supported by all of the calling
  // process, the broker process (if any), and the target process.
  //
  // This may only be used when connecting to embedders which support at least
  // |kVersion0|. Otherwise the deprecated |kLegacy| protocol must be used.
  kAutomatic = -1,

  // Protocol version 0. This is NOT backwards compatible with the legacy
  // protocol. If this is used to establish a connection, the remote embedder
  // (and the broker, if applicable) must also support at least version 0.
  kVersion0 = 0,

  // The maximum version supported by the current process. This must be
  // updated if a new version is added.
  kMaxSupportedVersion = kVersion0,
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_TRANSPORT_PROTOCOL_H_
