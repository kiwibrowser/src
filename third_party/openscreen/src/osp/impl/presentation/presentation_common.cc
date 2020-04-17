// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/presentation/presentation_common.h"

#include "absl/strings/ascii.h"

namespace openscreen {
namespace presentation {

std::unique_ptr<ProtocolConnection> GetProtocolConnection(
    uint64_t endpoint_id) {
  return NetworkServiceManager::Get()
      ->GetProtocolConnectionServer()
      ->CreateProtocolConnection(endpoint_id);
}

MessageDemuxer* GetServerDemuxer() {
  return NetworkServiceManager::Get()
      ->GetProtocolConnectionServer()
      ->message_demuxer();
}

MessageDemuxer* GetClientDemuxer() {
  return NetworkServiceManager::Get()
      ->GetProtocolConnectionClient()
      ->message_demuxer();
}

PresentationID::PresentationID(std::string presentation_id)
    : id_(Error::Code::kParseError) {
  // The spec dictates that the presentation ID must be composed
  // of at least 16 ASCII characters.
  bool is_valid = presentation_id.length() >= 16;
  for (const char& c : presentation_id) {
    is_valid &= absl::ascii_isprint(c);
  }

  if (is_valid) {
    id_ = std::move(presentation_id);
  }
}

}  // namespace presentation
}  // namespace openscreen
