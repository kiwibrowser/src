// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_DRAG_DROP_CONTROLLER_HOST_H_
#define UI_AURA_MUS_DRAG_DROP_CONTROLLER_HOST_H_

#include <stdint.h>

#include "ui/aura/aura_export.h"

namespace aura {

class WindowMus;

// Used by DragDropControllerMus to create a change id when a drag starts that
// is sent to ui::mojom::WindowTree.
class AURA_EXPORT DragDropControllerHost {
 public:
  virtual uint32_t CreateChangeIdForDrag(WindowMus* window) = 0;

 protected:
  virtual ~DragDropControllerHost() {}
};

}  // namespace aura

#endif  // UI_AURA_MUS_DRAG_DROP_CONTROLLER_HOST_H_
