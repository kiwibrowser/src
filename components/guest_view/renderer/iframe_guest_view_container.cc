// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/guest_view/renderer/iframe_guest_view_container.h"

#include "base/feature_list.h"
#include "components/guest_view/common/guest_view_messages.h"
#include "content/public/common/content_features.h"
#include "content/public/renderer/render_frame.h"

namespace guest_view {

IframeGuestViewContainer::IframeGuestViewContainer(
    content::RenderFrame* render_frame)
    : GuestViewContainer(render_frame) {
  CHECK(base::FeatureList::IsEnabled(::features::kGuestViewCrossProcessFrames));
  // There is no BrowserPluginDelegate to wait for.
  ready_ = true;
}

IframeGuestViewContainer::~IframeGuestViewContainer() {
}

bool IframeGuestViewContainer::OnMessage(const IPC::Message& message) {
  // TODO(lazyboy): Do not send this message in --site-per-process.
  if (message.type() == GuestViewMsg_GuestAttached::ID)
    return true;

  if (message.type() != GuestViewMsg_AttachToEmbedderFrame_ACK::ID)
    return false;

  OnHandleCallback(message);
  return true;
}

}  // namespace guest_view
