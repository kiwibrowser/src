// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_CAPTURE_SYNCHRONIZER_DELEGATE_H_
#define UI_AURA_MUS_CAPTURE_SYNCHRONIZER_DELEGATE_H_

#include <stdint.h>

#include "ui/aura/aura_export.h"

namespace aura {

class WindowMus;

// Used by CaptureSynchronizer to create a change id when capture changes. The
// change id is then sent to ui::mojom::WindowTree.
class AURA_EXPORT CaptureSynchronizerDelegate {
 public:
  virtual uint32_t CreateChangeIdForCapture(WindowMus* window) = 0;

 protected:
  virtual ~CaptureSynchronizerDelegate() {}
};

}  // namespace aura

#endif  // UI_AURA_MUS_CAPTURE_SYNCHRONIZER_DELEGATE_H_
