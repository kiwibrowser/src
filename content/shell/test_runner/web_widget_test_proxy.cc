// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/web_widget_test_proxy.h"

#include "content/shell/test_runner/event_sender.h"

namespace test_runner {

WebWidgetTestProxyBase::WebWidgetTestProxyBase()
    : web_widget_(nullptr),
      web_view_test_proxy_base_(nullptr),
      event_sender_(new EventSender(this)) {}

WebWidgetTestProxyBase::~WebWidgetTestProxyBase() {}

void WebWidgetTestProxyBase::Reset() {
  event_sender_->Reset();
}

void WebWidgetTestProxyBase::BindTo(blink::WebLocalFrame* frame) {
  event_sender_->Install(frame);
}

}  // namespace test_runner
