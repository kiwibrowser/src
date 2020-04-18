// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_UTILS_WIN_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_UTILS_WIN_H_

#include "third_party/blink/public/platform/web_drag_operation.h"

#include <windows.h>

namespace content {

blink::WebDragOperation WinDragOpToWebDragOp(DWORD effect);
blink::WebDragOperationsMask WinDragOpMaskToWebDragOpMask(DWORD effects);

DWORD WebDragOpToWinDragOp(blink::WebDragOperation op);
DWORD WebDragOpMaskToWinDragOpMask(blink::WebDragOperationsMask ops);

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_UTILS_WIN_H_
