// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/scheduler/web_render_widget_scheduling_state.h"

#include "third_party/blink/renderer/platform/scheduler/main_thread/render_widget_signals.h"

namespace blink {
namespace scheduler {

WebRenderWidgetSchedulingState::WebRenderWidgetSchedulingState(
    RenderWidgetSignals* render_widget_scheduling_signals)
    : render_widget_signals_(render_widget_scheduling_signals),
      hidden_(false),
      has_touch_handler_(false) {
  render_widget_signals_->IncNumVisibleRenderWidgets();
}

WebRenderWidgetSchedulingState::~WebRenderWidgetSchedulingState() {
  if (hidden_)
    return;

  render_widget_signals_->DecNumVisibleRenderWidgets();

  if (has_touch_handler_) {
    render_widget_signals_->DecNumVisibleRenderWidgetsWithTouchHandlers();
  }
}

void WebRenderWidgetSchedulingState::SetHidden(bool hidden) {
  if (hidden_ == hidden)
    return;

  hidden_ = hidden;

  if (hidden_) {
    render_widget_signals_->DecNumVisibleRenderWidgets();
    if (has_touch_handler_) {
      render_widget_signals_->DecNumVisibleRenderWidgetsWithTouchHandlers();
    }
  } else {
    render_widget_signals_->IncNumVisibleRenderWidgets();
    if (has_touch_handler_) {
      render_widget_signals_->IncNumVisibleRenderWidgetsWithTouchHandlers();
    }
  }
}

void WebRenderWidgetSchedulingState::SetHasTouchHandler(
    bool has_touch_handler) {
  if (has_touch_handler_ == has_touch_handler)
    return;

  has_touch_handler_ = has_touch_handler;

  if (hidden_)
    return;

  if (has_touch_handler_) {
    render_widget_signals_->IncNumVisibleRenderWidgetsWithTouchHandlers();
  } else {
    render_widget_signals_->DecNumVisibleRenderWidgetsWithTouchHandlers();
  }
}

}  // namespace scheduler
}  // namespace blink
