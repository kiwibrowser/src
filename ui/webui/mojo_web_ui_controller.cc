// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/webui/mojo_web_ui_controller.h"

#include "content/public/browser/render_process_host.h"
#include "content/public/common/bindings_policy.h"

namespace ui {

MojoWebUIControllerBase::MojoWebUIControllerBase(content::WebUI* contents)
    : content::WebUIController(contents) {
  contents->SetBindings(content::BINDINGS_POLICY_WEB_UI);
}

MojoWebUIControllerBase::~MojoWebUIControllerBase() = default;

MojoWebUIController::MojoWebUIController(content::WebUI* contents)
    : MojoWebUIControllerBase(contents),
      content::WebContentsObserver(contents->GetWebContents()) {}
MojoWebUIController::~MojoWebUIController() = default;

void MojoWebUIController::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  // Right now, this is expected to be called only for main frames.
  if (render_frame_host->GetParent()) {
    LOG(ERROR) << "Terminating renderer for requesting " << interface_name
               << " interface from subframe";
    render_frame_host->GetProcess()->ShutdownForBadMessage(
        content::RenderProcessHost::CrashReportMode::GENERATE_CRASH_DUMP);
    return;
  }

  registry_.TryBindInterface(interface_name, interface_pipe);
}

}  // namespace ui
