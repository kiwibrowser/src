// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_OBSERVER_H_
#define ASH_SHELF_SHELF_OBSERVER_H_

#include "ash/ash_export.h"
#include "ash/public/cpp/shelf_types.h"

namespace ash {

enum class AnimationChangeType;

// Used to observe changes to the shelf.
class ASH_EXPORT ShelfObserver {
 public:
  virtual void OnBackgroundTypeChanged(ShelfBackgroundType background_type,
                                       AnimationChangeType change_type) {}
  virtual void WillChangeVisibilityState(ShelfVisibilityState new_state) {}
  virtual void OnAutoHideStateChanged(ShelfAutoHideState new_state) {}
  virtual void OnShelfIconPositionsChanged() {}

 protected:
  virtual ~ShelfObserver() {}
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_OBSERVER_H_
