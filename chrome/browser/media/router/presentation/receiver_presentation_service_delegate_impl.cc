// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation/receiver_presentation_service_delegate_impl.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/media/router/presentation/local_presentation_manager.h"
#include "chrome/browser/media/router/presentation/local_presentation_manager_factory.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    media_router::ReceiverPresentationServiceDelegateImpl);

using content::RenderFrameHost;

namespace media_router {

// static
void ReceiverPresentationServiceDelegateImpl::CreateForWebContents(
    content::WebContents* web_contents,
    const std::string& presentation_id) {
  DCHECK(web_contents);

  if (FromWebContents(web_contents))
    return;

  web_contents->SetUserData(
      UserDataKey(),
      base::WrapUnique(new ReceiverPresentationServiceDelegateImpl(
          web_contents, presentation_id)));
  auto* render_view_host = web_contents->GetRenderViewHost();
  DCHECK(render_view_host);
  auto web_prefs = render_view_host->GetWebkitPreferences();
  web_prefs.presentation_receiver = true;
  render_view_host->UpdateWebkitPreferences(web_prefs);
}

void ReceiverPresentationServiceDelegateImpl::AddObserver(
    int render_process_id,
    int render_frame_id,
    content::PresentationServiceDelegate::Observer* observer) {
  DCHECK(observer);
  observers_.AddObserver(render_process_id, render_frame_id, observer);
}

void ReceiverPresentationServiceDelegateImpl::RemoveObserver(
    int render_process_id,
    int render_frame_id) {
  observers_.RemoveObserver(render_process_id, render_frame_id);
}

void ReceiverPresentationServiceDelegateImpl::Reset(int render_process_id,
                                                    int render_frame_id) {
  DVLOG(2) << __FUNCTION__ << render_process_id << ", " << render_frame_id;
  local_presentation_manager_->OnLocalPresentationReceiverTerminated(
      presentation_id_);
}

ReceiverPresentationServiceDelegateImpl::
    ReceiverPresentationServiceDelegateImpl(content::WebContents* web_contents,
                                            const std::string& presentation_id)
    : web_contents_(web_contents),
      presentation_id_(presentation_id),
      local_presentation_manager_(
          LocalPresentationManagerFactory::GetOrCreateForWebContents(
              web_contents_)) {
  DCHECK(web_contents_);
  DCHECK(!presentation_id.empty());
  DCHECK(local_presentation_manager_);
}

void ReceiverPresentationServiceDelegateImpl::
    RegisterReceiverConnectionAvailableCallback(
        const content::ReceiverConnectionAvailableCallback&
            receiver_available_callback) {
  local_presentation_manager_->OnLocalPresentationReceiverCreated(
      blink::mojom::PresentationInfo(web_contents_->GetLastCommittedURL(),
                                     presentation_id_),
      receiver_available_callback);
}

}  // namespace media_router
