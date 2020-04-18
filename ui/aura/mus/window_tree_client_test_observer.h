// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_WINDOW_TREE_CLIENT_TEST_OBSERVER_H_
#define UI_AURA_MUS_WINDOW_TREE_CLIENT_TEST_OBSERVER_H_

#include <cstdint>

#include "ui/aura/aura_export.h"

namespace aura {
enum class ChangeType;

class AURA_EXPORT WindowTreeClientTestObserver {
 public:
  // Called when a new change id is created.
  virtual void OnChangeStarted(uint32_t change_id, ChangeType type) = 0;

  // Called when a change is completed, successfully or unsuccessfully.
  virtual void OnChangeCompleted(uint32_t change_id,
                                 ChangeType type, bool success) = 0;

 protected:
  virtual ~WindowTreeClientTestObserver() {}
};

}  // namespace aura

#endif  // UI_AURA_MUS_WINDOW_TREE_CLIENT_TEST_OBSERVER_H_
