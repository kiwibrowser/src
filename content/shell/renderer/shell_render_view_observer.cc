// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/renderer/shell_render_view_observer.h"

#include "base/command_line.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/render_view_observer.h"
#include "content/shell/common/shell_switches.h"
#include "third_party/blink/public/web/web_testing_support.h"
#include "third_party/blink/public/web/web_view.h"

namespace content {

ShellRenderViewObserver::ShellRenderViewObserver(RenderView* render_view)
    : RenderViewObserver(render_view) {
}

ShellRenderViewObserver::~ShellRenderViewObserver() {
}

void ShellRenderViewObserver::DidClearWindowObject(
    blink::WebLocalFrame* frame) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kExposeInternalsForTesting)) {
    blink::WebTestingSupport::InjectInternalsObject(frame);
  }
}

void ShellRenderViewObserver::OnDestruct() {
  delete this;
}

}  // namespace content
