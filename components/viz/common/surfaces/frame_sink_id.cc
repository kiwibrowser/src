// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/frame_sink_id.h"

#include "base/strings/stringprintf.h"

namespace viz {

std::string FrameSinkId::ToString() const {
  return base::StringPrintf("FrameSinkId(%u, %u)", client_id_, sink_id_);
}

std::ostream& operator<<(std::ostream& out, const FrameSinkId& frame_sink_id) {
  return out << frame_sink_id.ToString();
}

}  // namespace viz
