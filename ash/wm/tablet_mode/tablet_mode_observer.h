// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TABLET_MODE_TABLET_MODE_OBSERVER_H_
#define ASH_TABLET_MODE_TABLET_MODE_OBSERVER_H_

#include "ash/ash_export.h"

namespace ash {

// Used to observe tablet mode changes inside ash. Exported for tests.
// NOTE: Code in chrome should use TabletModeClientObserver.
class ASH_EXPORT TabletModeObserver {
 public:
  // Called when the tablet mode has started. Windows might still be animating
  // though.
  virtual void OnTabletModeStarted() {}

  // Called when the tablet mode is about to end.
  virtual void OnTabletModeEnding() {}

  // Called when the tablet mode has ended. Windows may still be animating but
  // have been restored.
  virtual void OnTabletModeEnded() {}

 protected:
  virtual ~TabletModeObserver() {}
};

}  // namespace ash

#endif  // ASH_TABLET_MODE_TABLET_MODE_OBSERVER_H_
