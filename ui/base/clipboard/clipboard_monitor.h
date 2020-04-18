// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_CLIPBOARD_CLIPBOARD_MONITOR_H_
#define UI_BASE_CLIPBOARD_CLIPBOARD_MONITOR_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "ui/base/ui_base_export.h"

namespace ui {

class ClipboardObserver;

// A singleton instance to monitor and notify ClipboardObservers for clipboard
// changes.
class UI_BASE_EXPORT ClipboardMonitor : public base::ThreadChecker {
 public:
  static ClipboardMonitor* GetInstance();

  // Adds an observer.
  void AddObserver(ClipboardObserver* observer);

  // Removes an observer.
  void RemoveObserver(ClipboardObserver* observer);

  // Notifies all observers for clipboard data change.
  virtual void NotifyClipboardDataChanged();

 private:
  friend struct base::DefaultSingletonTraits<ClipboardMonitor>;
  ClipboardMonitor();
  virtual ~ClipboardMonitor();

  base::ObserverList<ClipboardObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(ClipboardMonitor);
};

}  // namespace ui

#endif /* UI_BASE_CLIPBOARD_CLIPBOARD_MONITOR_H_ */
