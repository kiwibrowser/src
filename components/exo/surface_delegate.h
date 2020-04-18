// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SURFACE_DELEGATE_H_
#define COMPONENTS_EXO_SURFACE_DELEGATE_H_

#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"

namespace exo {
class Surface;

// Frame types that can be used to decorate a surface.
enum class SurfaceFrameType { NONE, NORMAL, SHADOW, AUTOHIDE, OVERLAY };

// Handles events on surfaces in context-specific ways.
class SurfaceDelegate {
 public:
  // Called when surface was requested to commit all double-buffered state.
  virtual void OnSurfaceCommit() = 0;

  // Returns true if surface is in synchronized mode. ie. commit of
  // double-buffered state should be synchronized with parent surface.
  virtual bool IsSurfaceSynchronized() const = 0;

  // Returns true if surface should receive input events.
  virtual bool IsInputEnabled(Surface* surface) const = 0;

  // Called when surface was requested to use a specific frame type.
  virtual void OnSetFrame(SurfaceFrameType type) = 0;

  // Called when surface was requested to use a specific set of frame colors.
  virtual void OnSetFrameColors(SkColor active_color,
                                SkColor inactive_color) = 0;

  // Called when a new "parent" was requested for this surface. |position|
  // is the initial position of surface relative to origin of parent.
  virtual void OnSetParent(Surface* parent, const gfx::Point& position) = 0;

  // Called when surface was requested to set a specific startup ID label.
  virtual void OnSetStartupId(const char* startup_id) = 0;

  // Called when surface was requested to set a specific application ID label.
  virtual void OnSetApplicationId(const char* application_id) = 0;

 protected:
  virtual ~SurfaceDelegate() {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SURFACE_DELEGATE_H_
