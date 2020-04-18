// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/renderer/print_render_frame_observer.h"

#include "components/printing/common/print_messages.h"
#include "components/printing/renderer/print_render_frame_helper.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace android_webview {

PrintRenderFrameObserver::PrintRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
}

PrintRenderFrameObserver::~PrintRenderFrameObserver() {
}

bool PrintRenderFrameObserver::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintRenderFrameObserver, message)
    IPC_MESSAGE_HANDLER(PrintMsg_PrintNodeUnderContextMenu,
                        OnPrintNodeUnderContextMenu)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void PrintRenderFrameObserver::OnPrintNodeUnderContextMenu() {
  printing::PrintRenderFrameHelper* helper =
      printing::PrintRenderFrameHelper::Get(render_frame());
  if (helper)
    helper->PrintNode(render_frame()->GetWebFrame()->ContextMenuNode());
}

void PrintRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace android_webview
