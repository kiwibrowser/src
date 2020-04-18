// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/surface_id.h"

#include "base/strings/stringprintf.h"

namespace viz {

std::string SurfaceId::ToString() const {
  return base::StringPrintf("SurfaceId(%s, %s)",
                            frame_sink_id_.ToString().c_str(),
                            local_surface_id_.ToString().c_str());
}

std::ostream& operator<<(std::ostream& out, const SurfaceId& surface_id) {
  return out << surface_id.ToString();
}

}  // namespace viz
