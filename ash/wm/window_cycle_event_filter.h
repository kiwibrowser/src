// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WINDOW_CYCLE_EVENT_FILTER_H_
#define ASH_WM_WINDOW_CYCLE_EVENT_FILTER_H_

#include "ash/ash_export.h"

namespace ash {

// Created by WindowCycleController when cycling through windows. Eats all key
// events and stops cycling when the necessary key sequence is encountered.
class ASH_EXPORT WindowCycleEventFilter {
 public:
  virtual ~WindowCycleEventFilter() {}
};

}  // namepsace ash

#endif  // ASH_WM_WINDOW_CYCLE_EVENT_FILTER_H_
