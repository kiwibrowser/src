// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_coordinate_system.h"

#include "third_party/blink/renderer/modules/xr/xr_session.h"

namespace blink {

XRCoordinateSystem::XRCoordinateSystem(XRSession* session)
    : session_(session) {}

XRCoordinateSystem::~XRCoordinateSystem() = default;

// If possible, get the matrix required to transform between two coordinate
// systems.
DOMFloat32Array* XRCoordinateSystem::getTransformTo(
    XRCoordinateSystem* other) const {
  if (session_ != other->session()) {
    // Cannot get relationships between coordinate systems that belong to
    // different sessions.
    return nullptr;
  }

  // TODO(bajones): Track relationship to other coordinate systems and return
  // the transforms here. In the meantime we're allowed to return null to
  // indicate that the transform between the two coordinate systems is unknown.
  return nullptr;
}

void XRCoordinateSystem::Trace(blink::Visitor* visitor) {
  visitor->Trace(session_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
