// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/secondary_test_window_observer.h"

#include "content/public/browser/render_frame_host.h"
#include "content/shell/browser/layout_test/blink_test_controller.h"
#include "content/shell/common/shell_messages.h"

namespace content {

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SecondaryTestWindowObserver);

SecondaryTestWindowObserver::SecondaryTestWindowObserver(
    WebContents* web_contents)
    : WebContentsObserver(web_contents) {
  BlinkTestController* blink_test_controller = BlinkTestController::Get();
  if (!blink_test_controller)
    return;
  DCHECK(!blink_test_controller->IsMainWindow(web_contents));

  // Ensure that any preexisting frames (likely just the main frame) are handled
  // as well.
  for (RenderFrameHost* frame : web_contents->GetAllFrames()) {
    if (frame->IsRenderFrameLive())
      blink_test_controller->HandleNewRenderFrameHost(frame);
  }
}

SecondaryTestWindowObserver::~SecondaryTestWindowObserver() {}

void SecondaryTestWindowObserver::RenderFrameCreated(
    RenderFrameHost* render_frame_host) {
  BlinkTestController* blink_test_controller = BlinkTestController::Get();
  if (!blink_test_controller)
    return;
  DCHECK(!blink_test_controller->IsMainWindow(
      WebContents::FromRenderFrameHost(render_frame_host)));
  blink_test_controller->HandleNewRenderFrameHost(render_frame_host);
}

}  // namespace content
