// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/renderer/render_view_observer.h"

#include "content/renderer/render_view_impl.h"

namespace content {

RenderViewObserver::RenderViewObserver(RenderView* render_view)
    : render_view_(static_cast<RenderViewImpl*>(render_view)),
      routing_id_(MSG_ROUTING_NONE) {
  // |render_view_| can be null on unit testing or if Observe() is used.
  if (render_view_) {
    routing_id_ = render_view_->GetRoutingID();
    render_view_->AddObserver(this);
  }
}

RenderViewObserver::~RenderViewObserver() {
  if (render_view_)
    render_view_->RemoveObserver(this);
}

bool RenderViewObserver::OnMessageReceived(const IPC::Message& message) {
  return false;
}

bool RenderViewObserver::Send(IPC::Message* message) {
  if (render_view_)
    return render_view_->Send(message);

  delete message;
  return false;
}

RenderView* RenderViewObserver::render_view() const {
  return render_view_;
}

void RenderViewObserver::RenderViewGone() {
  render_view_ = nullptr;
}

void RenderViewObserver::Observe(RenderView* render_view) {
  if (render_view_) {
    render_view_->RemoveObserver(this);
    routing_id_ = MSG_ROUTING_NONE;
  }

  render_view_ = static_cast<RenderViewImpl*>(render_view);
  if (render_view_) {
    routing_id_ = render_view_->GetRoutingID();
    render_view_->AddObserver(this);
  }
}

}  // namespace content
