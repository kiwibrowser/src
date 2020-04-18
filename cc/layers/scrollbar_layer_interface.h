// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_SCROLLBAR_LAYER_INTERFACE_H_
#define CC_LAYERS_SCROLLBAR_LAYER_INTERFACE_H_

#include "base/macros.h"
#include "cc/cc_export.h"
#include "cc/input/scrollbar.h"

namespace cc {

class CC_EXPORT ScrollbarLayerInterface {
 public:
  virtual void SetScrollElementId(ElementId element_id) = 0;

 protected:
  ScrollbarLayerInterface() {}
  virtual ~ScrollbarLayerInterface() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ScrollbarLayerInterface);
};

}  // namespace cc

#endif  // CC_LAYERS_SCROLLBAR_LAYER_INTERFACE_H_
