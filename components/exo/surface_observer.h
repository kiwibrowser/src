// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SURFACE_OBSERVER_H_
#define COMPONENTS_EXO_SURFACE_OBSERVER_H_

namespace exo {
class Surface;

// Observers can listen to various events on the Surfaces.
class SurfaceObserver {
 public:
  // Called at the top of the surface's destructor, to give observers a
  // chance to remove themselves.
  virtual void OnSurfaceDestroying(Surface* surface) = 0;

 protected:
  virtual ~SurfaceObserver() {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SURFACE_OBSERVER_H_
