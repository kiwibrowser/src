// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/local_surface_id.h"

#include "base/strings/stringprintf.h"

namespace viz {

std::string LocalSurfaceId::ToString() const {
  std::string embed_token = VLOG_IS_ON(1)
                                ? embed_token_.ToString()
                                : embed_token_.ToString().substr(0, 4) + "...";

  return base::StringPrintf("LocalSurfaceId(%d, %d, %s)",
                            parent_sequence_number_, child_sequence_number_,
                            embed_token.c_str());
}

std::ostream& operator<<(std::ostream& out,
                         const LocalSurfaceId& local_surface_id) {
  return out << local_surface_id.ToString();
}

}  // namespace viz
