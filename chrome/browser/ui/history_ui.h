// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_HISTORY_UI_H_
#define CHROME_BROWSER_UI_HISTORY_UI_H_

#include "ui/base/layout.h"

namespace base {
class RefCountedMemory;
}

namespace history_ui {
base::RefCountedMemory* GetFaviconResourceBytes(ui::ScaleFactor scale_factor);
}

#endif  // CHROME_BROWSER_UI_HISTORY_UI_H_
