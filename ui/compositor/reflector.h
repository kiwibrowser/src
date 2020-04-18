// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_REFLECTOR_H_
#define UI_COMPOSITOR_REFLECTOR_H_

#include "ui/compositor/compositor_export.h"

namespace ui {
class Layer;

class COMPOSITOR_EXPORT Reflector {
 public:
  virtual ~Reflector();
  virtual void OnMirroringCompositorResized() = 0;
  virtual void AddMirroringLayer(Layer* layer) = 0;
  virtual void RemoveMirroringLayer(Layer* layer) = 0;
};

}  // namespace ui

#endif  // UI_COMPOSITOR_REFLECTOR_H_
