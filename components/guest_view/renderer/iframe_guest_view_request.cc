// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/guest_view/renderer/iframe_guest_view_request.h"

#include <utility>

#include "components/guest_view/common/guest_view_messages.h"
#include "components/guest_view/renderer/guest_view_container.h"
#include "content/public/renderer/render_frame.h"

namespace guest_view {

GuestViewAttachIframeRequest::GuestViewAttachIframeRequest(
    guest_view::GuestViewContainer* container,
    int render_frame_routing_id,
    int guest_instance_id,
    std::unique_ptr<base::DictionaryValue> params,
    v8::Local<v8::Function> callback,
    v8::Isolate* isolate)
    : GuestViewRequest(container, callback, isolate),
      render_frame_routing_id_(render_frame_routing_id),
      guest_instance_id_(guest_instance_id),
      params_(std::move(params)) {}

GuestViewAttachIframeRequest::~GuestViewAttachIframeRequest() {
}

void GuestViewAttachIframeRequest::PerformRequest() {
  DCHECK(container()->render_frame());

  container()->render_frame()->Send(new GuestViewHostMsg_AttachToEmbedderFrame(
      render_frame_routing_id_, container()->element_instance_id(),
      guest_instance_id_, *params_));
}

void GuestViewAttachIframeRequest::HandleResponse(const IPC::Message& message) {
  GuestViewMsg_AttachToEmbedderFrame_ACK::Param param;
  bool message_read_status =
      GuestViewMsg_AttachToEmbedderFrame_ACK::Read(&message, &param);
  DCHECK(message_read_status);

  ExecuteCallbackIfAvailable(0, nullptr);
}

}  // namespace guest_view
