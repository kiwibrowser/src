// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_TASKBAR_DECORATOR_WIN_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_TASKBAR_DECORATOR_WIN_H_

#include "ui/gfx/native_widget_types.h"

namespace gfx {
class Image;
}

namespace chrome {

// Draws a scaled version of the avatar in |image| on the taskbar button
// associated with top level, visible |window|. Currently only implemented
// for Windows 7 and above.
void DrawTaskbarDecoration(gfx::NativeWindow window, const gfx::Image* image);
}  // namespace chrome

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_TASKBAR_DECORATOR_WIN_H_
