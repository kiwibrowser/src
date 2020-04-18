// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/extension_port.h"

#include "content/public/renderer/render_frame.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/api/messaging/port_id.h"
#include "extensions/common/extension_messages.h"
#include "extensions/renderer/script_context.h"

namespace extensions {

ExtensionPort::ExtensionPort(ScriptContext* script_context,
                             const PortId& id,
                             int js_id)
    : script_context_(script_context), id_(id), js_id_(js_id) {}

ExtensionPort::~ExtensionPort() {}

void ExtensionPort::PostExtensionMessage(std::unique_ptr<Message> message) {
  content::RenderFrame* render_frame = script_context_->GetRenderFrame();
  // TODO(devlin): What should we do if there's no render frame? Up until now,
  // we've always just dropped the messages, but we might need to figure this
  // out for service workers.
  if (!render_frame)
    return;
  render_frame->Send(new ExtensionHostMsg_PostMessage(
      render_frame->GetRoutingID(), id_, *message));
}

void ExtensionPort::Close(bool close_channel) {
  content::RenderFrame* render_frame = script_context_->GetRenderFrame();
  if (!render_frame)
    return;
  render_frame->Send(new ExtensionHostMsg_CloseMessagePort(
      render_frame->GetRoutingID(), id_, close_channel));
}

}  // namespace extensions
