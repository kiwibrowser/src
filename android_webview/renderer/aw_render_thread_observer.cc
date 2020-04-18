// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/renderer/aw_render_thread_observer.h"

#include "android_webview/common/render_view_messages.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/blink/public/platform/web_cache.h"
#include "third_party/blink/public/platform/web_network_state_notifier.h"

namespace android_webview {

AwRenderThreadObserver::AwRenderThreadObserver() {
}

AwRenderThreadObserver::~AwRenderThreadObserver() {
}

bool AwRenderThreadObserver::OnControlMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AwRenderThreadObserver, message)
    IPC_MESSAGE_HANDLER(AwViewMsg_ClearCache, OnClearCache)
    IPC_MESSAGE_HANDLER(AwViewMsg_KillProcess, OnKillProcess)
    IPC_MESSAGE_HANDLER(AwViewMsg_SetJsOnlineProperty, OnSetJsOnlineProperty)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void AwRenderThreadObserver::OnClearCache() {
  blink::WebCache::Clear();
}

void AwRenderThreadObserver::OnKillProcess() {
  LOG(ERROR) << "Killing process (" << getpid() << ") upon request.";
  kill(getpid(), SIGKILL);
}

void AwRenderThreadObserver::OnSetJsOnlineProperty(bool network_up) {
  blink::WebNetworkStateNotifier::SetOnLine(network_up);
}

}  // namespace android_webview
