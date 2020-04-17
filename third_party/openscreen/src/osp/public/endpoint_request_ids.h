// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_ENDPOINT_REQUEST_IDS_H_
#define OSP_PUBLIC_ENDPOINT_REQUEST_IDS_H_

#include <cstdint>
#include <map>

#include "osp_base/macros.h"

namespace openscreen {

// Tracks the next available message request ID per endpoint by its endpoint ID.
// These can only be incremented while an endpoint is connected but can be reset
// on disconnection.  This is necessary because all APIs that use CBOR messages
// across a QUIC stream share the |request_id| field, which must be unique
// within a pair of endpoints.
class EndpointRequestIds {
 public:
  enum class Role {
    kClient,
    kServer,
  };

  explicit EndpointRequestIds(Role role);
  ~EndpointRequestIds();

  uint64_t GetNextRequestId(uint64_t endpoint_id);
  void ResetRequestId(uint64_t endpoint_id);
  void Reset();

 private:
  const Role role_;
  std::map<uint64_t, uint64_t> request_ids_by_endpoint_id_;

  OSP_DISALLOW_COPY_AND_ASSIGN(EndpointRequestIds);
};

}  // namespace openscreen

#endif  // OSP_PUBLIC_ENDPOINT_REQUEST_IDS_H_
