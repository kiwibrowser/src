// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_CLIPBOARD_CLIPBOARD_OBSERVER_H_
#define UI_BASE_CLIPBOARD_CLIPBOARD_OBSERVER_H_

#include "base/macros.h"
#include "ui/base/ui_base_export.h"

namespace ui {

// Observer that receives the notifications of clipboard change events.
class UI_BASE_EXPORT ClipboardObserver {
 public:
  // Called when clipboard data is changed.
  virtual void OnClipboardDataChanged() = 0;

 protected:
  virtual ~ClipboardObserver() {}
};

}  // namespace ui

#endif  // UI_BASE_CLIPBOARD_CLIPBOARD_OBSERVER_H_
