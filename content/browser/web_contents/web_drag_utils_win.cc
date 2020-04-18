// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_drag_utils_win.h"

#include <oleidl.h>
#include "base/logging.h"

using blink::WebDragOperation;
using blink::WebDragOperationsMask;
using blink::kWebDragOperationNone;
using blink::kWebDragOperationCopy;
using blink::kWebDragOperationLink;
using blink::kWebDragOperationMove;
using blink::kWebDragOperationGeneric;

namespace content {

WebDragOperation WinDragOpToWebDragOp(DWORD effect) {
  DCHECK(effect == DROPEFFECT_NONE || effect == DROPEFFECT_COPY ||
         effect == DROPEFFECT_LINK || effect == DROPEFFECT_MOVE);

  return WinDragOpMaskToWebDragOpMask(effect);
}

WebDragOperationsMask WinDragOpMaskToWebDragOpMask(DWORD effects) {
  WebDragOperationsMask ops = kWebDragOperationNone;
  if (effects & DROPEFFECT_COPY)
    ops = static_cast<WebDragOperationsMask>(ops | kWebDragOperationCopy);
  if (effects & DROPEFFECT_LINK)
    ops = static_cast<WebDragOperationsMask>(ops | kWebDragOperationLink);
  if (effects & DROPEFFECT_MOVE)
    ops = static_cast<WebDragOperationsMask>(ops | kWebDragOperationMove |
                                             kWebDragOperationGeneric);
  return ops;
}

DWORD WebDragOpToWinDragOp(WebDragOperation op) {
  DCHECK(op == kWebDragOperationNone || op == kWebDragOperationCopy ||
         op == kWebDragOperationLink || op == kWebDragOperationMove ||
         op == (kWebDragOperationMove | kWebDragOperationGeneric));

  return WebDragOpMaskToWinDragOpMask(op);
}

DWORD WebDragOpMaskToWinDragOpMask(WebDragOperationsMask ops) {
  DWORD win_ops = DROPEFFECT_NONE;
  if (ops & kWebDragOperationCopy)
    win_ops |= DROPEFFECT_COPY;
  if (ops & kWebDragOperationLink)
    win_ops |= DROPEFFECT_LINK;
  if (ops & (kWebDragOperationMove | kWebDragOperationGeneric))
    win_ops |= DROPEFFECT_MOVE;
  return win_ops;
}

}  // namespace content
