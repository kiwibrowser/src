// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WEBUI_MOJO_WEB_UI_CONTROLLER_H_
#define UI_WEBUI_MOJO_WEB_UI_CONTROLLER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/system/core.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace ui {

class MojoWebUIControllerBase : public content::WebUIController {
 public:
  explicit MojoWebUIControllerBase(content::WebUI* contents);
  ~MojoWebUIControllerBase() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MojoWebUIControllerBase);
};

// MojoWebUIController is intended for web ui pages that use mojo. It is
// expected that subclasses will do two things:
// . In the constructor invoke AddMojoResourcePath() to register the bindings
//   files, eg:
//     AddMojoResourcePath("chrome/browser/ui/webui/omnibox/omnibox.mojom",
//                         IDR_OMNIBOX_MOJO_JS);
// . Call AddHandlerToRegistry for all Mojo Interfaces it wishes to handle.
class MojoWebUIController : public MojoWebUIControllerBase,
                            public content::WebContentsObserver {
 public:
  explicit MojoWebUIController(content::WebUI* contents);
  ~MojoWebUIController() override;

  // content::WebContentsObserver implementation.
  void OnInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;

  template <typename Binder>
  void AddHandlerToRegistry(Binder binder) {
    registry_.AddInterface(std::move(binder));
  }

 private:
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(MojoWebUIController);
};

}  // namespace ui

#endif  // UI_WEBUI_MOJO_WEB_UI_CONTROLLER_H_
