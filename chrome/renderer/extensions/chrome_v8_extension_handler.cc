// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/chrome_v8_extension_handler.h"

#include "base/logging.h"
#include "content/public/renderer/render_thread.h"

using content::RenderThread;

namespace extensions {

ChromeV8ExtensionHandler::ChromeV8ExtensionHandler()
    : routing_id_(MSG_ROUTING_NONE) {
}

ChromeV8ExtensionHandler::~ChromeV8ExtensionHandler() {
  if (routing_id_ != MSG_ROUTING_NONE)
    RenderThread::Get()->RemoveRoute(routing_id_);
}

int ChromeV8ExtensionHandler::GetRoutingID() {
  if (routing_id_ == MSG_ROUTING_NONE) {
    routing_id_ = RenderThread::Get()->GenerateRoutingID();
    RenderThread::Get()->AddRoute(routing_id_, this);
  }

  return routing_id_;
}

void ChromeV8ExtensionHandler::Send(IPC::Message* message) {
  RenderThread::Get()->Send(message);
}

}  // namespace extensions
