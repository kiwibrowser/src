// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/mock_renderer_ppapi_host.h"

#include "content/public/renderer/render_view.h"
#include "content/renderer/pepper/fake_pepper_plugin_instance.h"
#include "ui/gfx/geometry/point.h"

namespace content {

MockRendererPpapiHost::MockRendererPpapiHost(RenderView* render_view,
                                             PP_Instance instance)
    : sink_(),
      ppapi_host_(&sink_, ppapi::PpapiPermissions()),
      render_view_(render_view),
      pp_instance_(instance),
      has_user_gesture_(false),
      plugin_instance_(new FakePepperPluginInstance) {
  if (render_view)
    render_frame_ = render_view->GetMainRenderFrame();
}

MockRendererPpapiHost::~MockRendererPpapiHost() {}

ppapi::host::PpapiHost* MockRendererPpapiHost::GetPpapiHost() {
  return &ppapi_host_;
}

bool MockRendererPpapiHost::IsValidInstance(PP_Instance instance) const {
  return instance == pp_instance_;
}

PepperPluginInstance* MockRendererPpapiHost::GetPluginInstance(
    PP_Instance instance) const {
  return plugin_instance_.get();
}

RenderFrame* MockRendererPpapiHost::GetRenderFrameForInstance(
    PP_Instance instance) const {
  if (instance == pp_instance_)
    return render_frame_;
  return nullptr;
}

RenderView* MockRendererPpapiHost::GetRenderViewForInstance(
    PP_Instance instance) const {
  if (instance == pp_instance_)
    return render_view_;
  return nullptr;
}

blink::WebPluginContainer* MockRendererPpapiHost::GetContainerForInstance(
    PP_Instance instance) const {
  NOTIMPLEMENTED();
  return nullptr;
}

bool MockRendererPpapiHost::HasUserGesture(PP_Instance instance) const {
  return has_user_gesture_;
}

int MockRendererPpapiHost::GetRoutingIDForWidget(PP_Instance instance) const {
  return 0;
}

gfx::Point MockRendererPpapiHost::PluginPointToRenderFrame(
    PP_Instance instance,
    const gfx::Point& pt) const {
  return gfx::Point();
}

IPC::PlatformFileForTransit MockRendererPpapiHost::ShareHandleWithRemote(
    base::PlatformFile handle,
    bool should_close_source) {
  NOTIMPLEMENTED();
  return IPC::InvalidPlatformFileForTransit();
}

base::SharedMemoryHandle
MockRendererPpapiHost::ShareSharedMemoryHandleWithRemote(
    const base::SharedMemoryHandle& handle) {
  NOTIMPLEMENTED();
  return base::SharedMemoryHandle();
}

bool MockRendererPpapiHost::IsRunningInProcess() const { return false; }

std::string MockRendererPpapiHost::GetPluginName() const {
  return std::string();
}

void MockRendererPpapiHost::SetToExternalPluginHost() {
  NOTIMPLEMENTED();
}

void MockRendererPpapiHost::CreateBrowserResourceHosts(
    PP_Instance instance,
    const std::vector<IPC::Message>& nested_msgs,
    const base::Callback<void(const std::vector<int>&)>& callback) const {
  callback.Run(std::vector<int>(nested_msgs.size(), 0));
  return;
}

GURL MockRendererPpapiHost::GetDocumentURL(PP_Instance instance) const {
  NOTIMPLEMENTED();
  return GURL();
}

}  // namespace content
