// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/content/background_loader/background_loader_contents.h"

#include "content/public/browser/web_contents.h"

namespace background_loader {

BackgroundLoaderContents::BackgroundLoaderContents(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  web_contents_ = content::WebContents::Create(
      content::WebContents::CreateParams(browser_context_));
  web_contents_->SetAudioMuted(true);
  web_contents_->SetDelegate(this);
}

BackgroundLoaderContents::~BackgroundLoaderContents() {}

void BackgroundLoaderContents::LoadPage(const GURL& url) {
  web_contents_->GetController().LoadURL(
      url /* url to be loaded */,
      content::Referrer() /* Default referrer policy, no referring url */,
      ui::PAGE_TRANSITION_LINK /* page transition type: clicked on link */,
      std::string() /* extra headers */);
}

void BackgroundLoaderContents::Cancel() {
  web_contents_->Close();
}

void BackgroundLoaderContents::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

bool BackgroundLoaderContents::IsNeverVisible(
    content::WebContents* web_contents) {
  // Background, so not visible.
  return true;
}

void BackgroundLoaderContents::CloseContents(content::WebContents* source) {
  // Do nothing. Other pages should not be able to close a background page.
  NOTREACHED();
}

bool BackgroundLoaderContents::ShouldSuppressDialogs(
    content::WebContents* source) {
  // Dialog prompts are not actionable in the background.
  return true;
}

bool BackgroundLoaderContents::ShouldFocusPageAfterCrash() {
  // Background page should never be focused.
  return false;
}

void BackgroundLoaderContents::CanDownload(
    const GURL& url,
    const std::string& request_method,
    const base::Callback<void(bool)>& callback) {
  if (delegate_) {
    delegate_->CanDownload(callback);
  } else {
    // Do not download anything if there's no delegate.
    callback.Run(false);
  }
}

bool BackgroundLoaderContents::ShouldCreateWebContents(
    content::WebContents* web_contents,
    content::RenderFrameHost* opener,
    content::SiteInstance* source_site_instance,
    int32_t route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    content::mojom::WindowContainerType window_container_type,
    const GURL& opener_url,
    const std::string& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  // Background pages should not create other webcontents/tabs.
  return false;
}

void BackgroundLoaderContents::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture,
    bool* was_blocked) {
  // Pop-ups should be blocked;
  // background pages should not create other contents
  if (was_blocked != nullptr)
    *was_blocked = true;
}

#if defined(OS_ANDROID)
bool BackgroundLoaderContents::ShouldBlockMediaRequest(const GURL& url) {
  // Background pages should not have access to media.
  return true;
}
#endif

void BackgroundLoaderContents::RequestMediaAccessPermission(
    content::WebContents* contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  // No permissions granted, act as if dismissed.
  callback.Run(
      content::MediaStreamDevices(),
      content::MediaStreamRequestResult::MEDIA_DEVICE_PERMISSION_DISMISSED,
      std::unique_ptr<content::MediaStreamUI>());
}

bool BackgroundLoaderContents::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    content::MediaStreamType type) {
  return false;  // No permissions granted.
}

void BackgroundLoaderContents::AdjustPreviewsStateForNavigation(
    content::WebContents* web_contents,
    content::PreviewsState* previews_state) {
  DCHECK(previews_state);

  // If previews are already disabled, do nothing.
  if (*previews_state == content::PREVIEWS_OFF ||
      *previews_state == content::PREVIEWS_NO_TRANSFORM) {
    return;
  }

  if (*previews_state == content::PREVIEWS_UNSPECIFIED) {
    *previews_state = content::PARTIAL_CONTENT_SAFE_PREVIEWS;
  } else {
    *previews_state &= content::PARTIAL_CONTENT_SAFE_PREVIEWS;
    if (*previews_state == 0)
      *previews_state = content::PREVIEWS_OFF;
  }
}

BackgroundLoaderContents::BackgroundLoaderContents()
    : browser_context_(nullptr) {
  web_contents_.reset();
}

}  // namespace background_loader
