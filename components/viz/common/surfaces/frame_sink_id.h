// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_SURFACES_FRAME_SINK_ID_H_
#define COMPONENTS_VIZ_COMMON_SURFACES_FRAME_SINK_ID_H_

#include <stdint.h>

#include <iosfwd>
#include <string>
#include <tuple>

#include "base/hash.h"
#include "components/viz/common/viz_common_export.h"

namespace viz {

class VIZ_COMMON_EXPORT FrameSinkId {
 public:
  constexpr FrameSinkId() : client_id_(0), sink_id_(0) {}

  constexpr FrameSinkId(const FrameSinkId& other)
      : client_id_(other.client_id_), sink_id_(other.sink_id_) {}

  constexpr FrameSinkId(uint32_t client_id, uint32_t sink_id)
      : client_id_(client_id), sink_id_(sink_id) {}

  constexpr bool is_valid() const { return client_id_ != 0 || sink_id_ != 0; }

  constexpr uint32_t client_id() const { return client_id_; }

  constexpr uint32_t sink_id() const { return sink_id_; }

  bool operator==(const FrameSinkId& other) const {
    return client_id_ == other.client_id_ && sink_id_ == other.sink_id_;
  }

  bool operator!=(const FrameSinkId& other) const { return !(*this == other); }

  bool operator<(const FrameSinkId& other) const {
    return std::tie(client_id_, sink_id_) <
           std::tie(other.client_id_, other.sink_id_);
  }

  size_t hash() const { return base::HashInts(client_id_, sink_id_); }

  std::string ToString() const;

 private:
  uint32_t client_id_;
  uint32_t sink_id_;
};

VIZ_COMMON_EXPORT std::ostream& operator<<(std::ostream& out,
                                           const FrameSinkId& frame_sink_id);

struct FrameSinkIdHash {
  size_t operator()(const FrameSinkId& key) const { return key.hash(); }
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_SURFACES_FRAME_SINK_ID_H_
