// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/ime_event_guard.h"

#include "content/renderer/render_widget.h"

namespace content {

// When ThreadedInputConnection is used, we want to make sure that FROM_IME
// is set only for OnRequestTextInputStateUpdate() so that we can distinguish
// it from other updates so that we can wait for it safely. So it is false by
// default.
ImeEventGuard::ImeEventGuard(RenderWidget* widget)
    : widget_(widget), show_virtual_keyboard_(false), reply_to_request_(false) {
  widget_->OnImeEventGuardStart(this);
}

ImeEventGuard::~ImeEventGuard() {
  widget_->OnImeEventGuardFinish(this);
}

} //  namespace content
