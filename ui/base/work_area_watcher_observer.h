// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_WORK_AREA_WATCHER_OBSERVER_H_
#define UI_BASE_WORK_AREA_WATCHER_OBSERVER_H_

#include "ui/base/ui_base_export.h"

namespace ui {

// This interface should be implemented by classes that want to be notified
// when the work area has changed due to something like screen resolution or
// taskbar changes.
class UI_BASE_EXPORT WorkAreaWatcherObserver {
 public:
  virtual void WorkAreaChanged() = 0;

 protected:
  virtual ~WorkAreaWatcherObserver() {}
};

}  // namespace ui

#endif  // UI_BASE_WORK_AREA_WATCHER_OBSERVER_H_
