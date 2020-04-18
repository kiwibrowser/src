// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_DEADLINE_CLIENT_H_
#define COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_DEADLINE_CLIENT_H_

namespace viz {

class SurfaceDeadlineClient {
 public:
  // Called when a deadline passes for a set of dependencies.
  virtual void OnDeadline(base::TimeDelta duration) = 0;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_DEADLINE_CLIENT_H_
