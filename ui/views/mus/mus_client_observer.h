// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_MUS_CLIENT_OBSERVER_H_
#define UI_VIEWS_MUS_MUS_CLIENT_OBSERVER_H_

#include "ui/views/mus/mus_export.h"

namespace views {

// Allows individual DesktopWindowTreeHostsMus to observe MusClient events.
class VIEWS_MUS_EXPORT MusClientObserver {
 public:
  // Notification that windows should update per window values from the
  // WindowManagerFrameValues.
  virtual void OnWindowManagerFrameValuesChanged() = 0;

 protected:
  virtual ~MusClientObserver() {}
};

}  // namespace views

#endif  // UI_VIEWS_MUS_MUS_CLIENT_OBSERVER_H_
