// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/layout_and_paint_async_then.h"

#include "base/barrier_closure.h"
#include "base/callback.h"
#include "base/trace_event/trace_event.h"
#include "third_party/blink/public/web/web_page_popup.h"
#include "third_party/blink/public/web/web_widget.h"

namespace test_runner {

void LayoutAndPaintAsyncThen(blink::WebWidget* web_widget,
                             base::OnceClosure callback) {
  TRACE_EVENT0("shell", "LayoutAndPaintAsyncThen");

  if (blink::WebPagePopup* popup = web_widget->GetPagePopup()) {
    auto barrier = base::BarrierClosure(2, std::move(callback));
    web_widget->LayoutAndPaintAsync(barrier);
    popup->LayoutAndPaintAsync(barrier);
  } else {
    web_widget->LayoutAndPaintAsync(std::move(callback));
  }
}

}  // namespace test_runner
