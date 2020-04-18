// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_SCREEN_SECURITY_SCREEN_SHARE_OBSERVER_H_
#define ASH_SYSTEM_SCREEN_SECURITY_SCREEN_SHARE_OBSERVER_H_

#include "base/callback.h"
#include "base/strings/string16.h"

namespace ash {

class ScreenShareObserver {
 public:
  // Called when screen share is started.
  virtual void OnScreenShareStart(const base::Closure& stop_callback,
                                  const base::string16& helper_name) = 0;

  // Called when screen share is stopped.
  virtual void OnScreenShareStop() = 0;

 protected:
  virtual ~ScreenShareObserver() {}
};

}  // namespace ash

#endif  // ASH_SYSTEM_SCREEN_SECURITY_SCREEN_SHARE_OBSERVER_H_
