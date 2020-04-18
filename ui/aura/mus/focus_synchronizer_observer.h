// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_FOCUS_SYNCHRONIZER_OBSERVER_H_
#define UI_AURA_MUS_FOCUS_SYNCHRONIZER_OBSERVER_H_

#include "ui/aura/aura_export.h"

namespace aura {
class Window;

namespace client {
class FocusClient;
}

// FocusSynchronizerObserver gets notified when the active focus client and the
// window it's associated with (active focus client root) maintained by
// FocusSynchronizer changed. To get notified when the actual focused window
// gets changed, use FocusChangeObserver instead.
class AURA_EXPORT FocusSynchronizerObserver {
 public:
  // Called from FocusSynchronizer::SetActiveFocusClient() to notify its
  // observers of active focus client and active focus client root changes.
  virtual void OnActiveFocusClientChanged(client::FocusClient* focus_client,
                                          Window* focus_client_root) {}

 protected:
  virtual ~FocusSynchronizerObserver() {}
};

}  // namespace aura

#endif  // UI_AURA_MUS_FOCUS_SYNCHRONIZER_OBSERVER_H_
