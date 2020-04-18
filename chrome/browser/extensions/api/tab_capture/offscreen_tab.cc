// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/tab_capture/offscreen_tab.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/macros.h"
#include "base/stl_util.h"
#include "chrome/browser/extensions/api/tab_capture/tab_capture_registry.h"
#include "chrome/browser/media/router/presentation/presentation_navigation_policy.h"
#include "chrome/browser/media/router/presentation/receiver_presentation_service_delegate_impl.h"  // nogncheck
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/web_contents_sizer.h"
#include "content/public/browser/keyboard_event_processing_result.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/process_manager.h"
#include "third_party/blink/public/web/web_presentation_receiver_flags.h"

using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(extensions::OffscreenTabsOwner);

namespace {

// Upper limit on the number of simultaneous off-screen tabs per extension
// instance.
const int kMaxOffscreenTabsPerExtension = 4;

// Time intervals used by the logic that detects when the capture of an
// offscreen tab has stopped, to automatically tear it down and free resources.
const int kMaxSecondsToWaitForCapture = 60;
const int kPollIntervalInSeconds = 1;

}  // namespace

namespace extensions {

OffscreenTabsOwner::OffscreenTabsOwner(WebContents* extension_web_contents)
    : extension_web_contents_(extension_web_contents) {
  DCHECK(extension_web_contents_);
}

OffscreenTabsOwner::~OffscreenTabsOwner() {}

// static
OffscreenTabsOwner* OffscreenTabsOwner::Get(
    content::WebContents* extension_web_contents) {
  // CreateForWebContents() really means "create if not exists."
  CreateForWebContents(extension_web_contents);
  return FromWebContents(extension_web_contents);
}

OffscreenTab* OffscreenTabsOwner::OpenNewTab(
    const GURL& start_url,
    const gfx::Size& initial_size,
    const std::string& optional_presentation_id) {
  if (tabs_.size() >= kMaxOffscreenTabsPerExtension)
    return nullptr;  // Maximum number of offscreen tabs reached.

  // OffscreenTab cannot be created with MakeUnique<OffscreenTab> since the
  // constructor is protected. So create it separately, and then move it to
  // |tabs_| below.
  std::unique_ptr<OffscreenTab> offscreen_tab(new OffscreenTab(this));
  tabs_.push_back(std::move(offscreen_tab));
  tabs_.back()->Start(start_url, initial_size, optional_presentation_id);
  return tabs_.back().get();
}

void OffscreenTabsOwner::DestroyTab(OffscreenTab* tab) {
  for (std::vector<std::unique_ptr<OffscreenTab>>::iterator iter =
           tabs_.begin();
       iter != tabs_.end(); ++iter) {
    if (iter->get() == tab) {
      tabs_.erase(iter);
      break;
    }
  }
}

OffscreenTab::OffscreenTab(OffscreenTabsOwner* owner)
    : owner_(owner),
      otr_profile_registration_(
          IndependentOTRProfileManager::GetInstance()
              ->CreateFromOriginalProfile(
                  Profile::FromBrowserContext(
                      owner->extension_web_contents()->GetBrowserContext()),
                  base::BindOnce(&OffscreenTab::DieIfOriginalProfileDestroyed,
                                 base::Unretained(this)))),
      capture_poll_timer_(false, false),
      content_capture_was_detected_(false),
      navigation_policy_(
          std::make_unique<media_router::DefaultNavigationPolicy>()) {
  DCHECK(otr_profile_registration_->profile());
}

OffscreenTab::~OffscreenTab() {
  DVLOG(1) << "Destroying OffscreenTab for start_url=" << start_url_.spec();
}

void OffscreenTab::Start(const GURL& start_url,
                         const gfx::Size& initial_size,
                         const std::string& optional_presentation_id) {
  DCHECK(start_time_.is_null());
  start_url_ = start_url;
  DVLOG(1) << "Starting OffscreenTab with initial size of "
           << initial_size.ToString() << " for start_url=" << start_url_.spec();

  // Create the WebContents to contain the off-screen tab's page.
  WebContents::CreateParams params(otr_profile_registration_->profile());
  if (!optional_presentation_id.empty())
    params.starting_sandbox_flags = blink::kPresentationReceiverSandboxFlags;

  offscreen_tab_web_contents_ = WebContents::Create(params);
  offscreen_tab_web_contents_->SetDelegate(this);
  WebContentsObserver::Observe(offscreen_tab_web_contents_.get());

  // Set initial size, if specified.
  if (!initial_size.IsEmpty())
    ResizeWebContents(offscreen_tab_web_contents_.get(),
                      gfx::Rect(initial_size));

  // Mute audio output.  When tab capture starts, the audio will be
  // automatically unmuted, but will be captured into the MediaStream.
  offscreen_tab_web_contents_->SetAudioMuted(true);

  if (!optional_presentation_id.empty()) {
    // This offscreen tab is a presentation created through the Presentation
    // API. https://www.w3.org/TR/presentation-api/
    //
    // Create a ReceiverPresentationServiceDelegateImpl associated with the
    // offscreen tab's WebContents.  The new instance will set up the necessary
    // infrastructure to allow controlling pages the ability to connect to the
    // offscreen tab.
    DVLOG(1) << "Register with ReceiverPresentationServiceDelegateImpl, "
             << "presentation_id=" << optional_presentation_id;
    media_router::ReceiverPresentationServiceDelegateImpl::CreateForWebContents(
        offscreen_tab_web_contents_.get(), optional_presentation_id);

    // Presentations are not allowed to perform top-level navigations after
    // initial load.  This is enforced through sandboxing flags, but we also
    // enforce it here.
    navigation_policy_ =
        std::make_unique<media_router::PresentationNavigationPolicy>();
  }

  // Navigate to the initial URL.
  content::NavigationController::LoadURLParams load_params(start_url_);
  load_params.should_replace_current_entry = true;
  load_params.should_clear_history_list = true;
  offscreen_tab_web_contents_->GetController().LoadURLWithParams(load_params);

  start_time_ = base::TimeTicks::Now();
  DieIfContentCaptureEnded();
}

void OffscreenTab::Close() {
  if (offscreen_tab_web_contents_)
    offscreen_tab_web_contents_->ClosePage();
}

void OffscreenTab::CloseContents(WebContents* source) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), source);
  // Javascript in the page called window.close().
  DVLOG(1) << "OffscreenTab for start_url=" << start_url_.spec() << " will die";
  owner_->DestroyTab(this);
  // |this| is no longer valid.
}

bool OffscreenTab::ShouldSuppressDialogs(WebContents* source) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), source);
  // Suppress all because there is no possible direct user interaction with
  // dialogs.
  // TODO(crbug.com/734191): This does not suppress window.print().
  return true;
}

bool OffscreenTab::ShouldFocusLocationBarByDefault(WebContents* source) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), source);
  // Indicate the location bar should be focused instead of the page, even
  // though there is no location bar.  This will prevent the page from
  // automatically receiving input focus, which should never occur since there
  // is not supposed to be any direct user interaction.
  return true;
}

bool OffscreenTab::ShouldFocusPageAfterCrash() {
  // Never focus the page.  Not even after a crash.
  return false;
}

void OffscreenTab::CanDownload(const GURL& url,
                               const std::string& request_method,
                               const base::Callback<void(bool)>& callback) {
  // Offscreen tab pages are not allowed to download files.
  callback.Run(false);
}

bool OffscreenTab::HandleContextMenu(const content::ContextMenuParams& params) {
  // Context menus should never be shown.  Do nothing, but indicate the context
  // menu was shown so that default implementation in libcontent does not
  // attempt to do so on its own.
  return true;
}

content::KeyboardEventProcessingResult OffscreenTab::PreHandleKeyboardEvent(
    WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), source);
  // Intercept and silence all keyboard events before they can be sent to the
  // renderer.
  return content::KeyboardEventProcessingResult::HANDLED;
}

bool OffscreenTab::PreHandleGestureEvent(WebContents* source,
                                         const blink::WebGestureEvent& event) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), source);
  // Intercept and silence all gesture events before they can be sent to the
  // renderer.
  return true;
}

bool OffscreenTab::CanDragEnter(
    WebContents* source,
    const content::DropData& data,
    blink::WebDragOperationsMask operations_allowed) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), source);
  // Halt all drag attempts onto the page since there should be no direct user
  // interaction with it.
  return false;
}

bool OffscreenTab::ShouldCreateWebContents(
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
  DCHECK_EQ(offscreen_tab_web_contents_.get(), web_contents);
  // Disallow creating separate WebContentses.  The WebContents implementation
  // uses this to spawn new windows/tabs, which is also not allowed for
  // offscreen tabs.
  return false;
}

bool OffscreenTab::EmbedsFullscreenWidget() const {
  // OffscreenTab will manage fullscreen widgets.
  return true;
}

void OffscreenTab::EnterFullscreenModeForTab(
    WebContents* contents,
    const GURL& origin,
    const blink::WebFullscreenOptions& options) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), contents);

  if (in_fullscreen_mode())
    return;

  non_fullscreen_size_ =
      contents->GetRenderWidgetHostView()->GetViewBounds().size();
  if (contents->IsBeingCaptured() && !contents->GetPreferredSize().IsEmpty())
    ResizeWebContents(contents, gfx::Rect(contents->GetPreferredSize()));
}

void OffscreenTab::ExitFullscreenModeForTab(WebContents* contents) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), contents);

  if (!in_fullscreen_mode())
    return;

  ResizeWebContents(contents, gfx::Rect(non_fullscreen_size_));
  non_fullscreen_size_ = gfx::Size();
}

bool OffscreenTab::IsFullscreenForTabOrPending(
    const WebContents* contents) const {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), contents);
  return in_fullscreen_mode();
}

blink::WebDisplayMode OffscreenTab::GetDisplayMode(
    const WebContents* contents) const {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), contents);
  return in_fullscreen_mode() ? blink::kWebDisplayModeFullscreen
                              : blink::kWebDisplayModeBrowser;
}

void OffscreenTab::RequestMediaAccessPermission(
      WebContents* contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(), contents);

  // This method is being called to check whether an extension is permitted to
  // capture the page.  Verify that the request is being made by the extension
  // that spawned this OffscreenTab.

  // Find the extension ID associated with the extension background page's
  // WebContents.
  content::BrowserContext* const extension_browser_context =
      owner_->extension_web_contents()->GetBrowserContext();
  const extensions::Extension* const extension =
      ProcessManager::Get(extension_browser_context)->
          GetExtensionForWebContents(owner_->extension_web_contents());
  const std::string extension_id = extension ? extension->id() : "";
  LOG_IF(DFATAL, extension_id.empty())
      << "Extension that started this OffscreenTab was not found.";

  // If verified, allow any tab capture audio/video devices that were requested.
  extensions::TabCaptureRegistry* const tab_capture_registry =
      extensions::TabCaptureRegistry::Get(extension_browser_context);
  content::MediaStreamDevices devices;
  if (tab_capture_registry && tab_capture_registry->VerifyRequest(
          request.render_process_id,
          request.render_frame_id,
          extension_id)) {
    if (request.audio_type == content::MEDIA_TAB_AUDIO_CAPTURE) {
      devices.push_back(content::MediaStreamDevice(
          content::MEDIA_TAB_AUDIO_CAPTURE, std::string(), std::string()));
    }
    if (request.video_type == content::MEDIA_TAB_VIDEO_CAPTURE) {
      devices.push_back(content::MediaStreamDevice(
          content::MEDIA_TAB_VIDEO_CAPTURE, std::string(), std::string()));
    }
  }

  DVLOG(2) << "Allowing " << devices.size()
           << " capture devices for OffscreenTab content.";

  callback.Run(devices, devices.empty() ? content::MEDIA_DEVICE_INVALID_STATE
                                        : content::MEDIA_DEVICE_OK,
               std::unique_ptr<content::MediaStreamUI>(nullptr));
}

bool OffscreenTab::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    content::MediaStreamType type) {
  DCHECK_EQ(offscreen_tab_web_contents_.get(),
            content::WebContents::FromRenderFrameHost(render_frame_host));
  return type == content::MEDIA_TAB_AUDIO_CAPTURE ||
      type == content::MEDIA_TAB_VIDEO_CAPTURE;
}

void OffscreenTab::DidShowFullscreenWidget() {
  if (!offscreen_tab_web_contents_->IsBeingCaptured() ||
      offscreen_tab_web_contents_->GetPreferredSize().IsEmpty())
    return;  // Do nothing, since no preferred size is specified.
  content::RenderWidgetHostView* const current_fs_view =
      offscreen_tab_web_contents_->GetFullscreenRenderWidgetHostView();
  if (current_fs_view)
    current_fs_view->SetSize(offscreen_tab_web_contents_->GetPreferredSize());
}

void OffscreenTab::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK(offscreen_tab_web_contents_.get());
  if (!navigation_policy_->AllowNavigation(navigation_handle)) {
    DVLOG(2) << "Closing because NavigationPolicy disallowed "
             << "StartNavigation to " << navigation_handle->GetURL().spec();
    Close();
  }
}

void OffscreenTab::DieIfContentCaptureEnded() {
  DCHECK(offscreen_tab_web_contents_.get());

  if (content_capture_was_detected_) {
    if (!offscreen_tab_web_contents_->IsBeingCaptured()) {
      DVLOG(2) << "Capture of OffscreenTab content has stopped for start_url="
               << start_url_.spec();
      owner_->DestroyTab(this);
      return;  // |this| is no longer valid.
    } else {
      DVLOG(3) << "Capture of OffscreenTab content continues for start_url="
               << start_url_.spec();
    }
  } else if (offscreen_tab_web_contents_->IsBeingCaptured()) {
    DVLOG(2) << "Capture of OffscreenTab content has started for start_url="
             << start_url_.spec();
    content_capture_was_detected_ = true;
  } else if (base::TimeTicks::Now() - start_time_ >
                 base::TimeDelta::FromSeconds(kMaxSecondsToWaitForCapture)) {
    // More than a minute has elapsed since this OffscreenTab was started and
    // content capture still hasn't started.  As a safety precaution, assume
    // that content capture is never going to start and die to free up
    // resources.
    LOG(WARNING) << "Capture of OffscreenTab content did not start "
                    "within timeout for start_url=" << start_url_.spec();
    owner_->DestroyTab(this);
    return;  // |this| is no longer valid.
  }

  // Schedule the timer to check again in a second.
  capture_poll_timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(kPollIntervalInSeconds),
      base::Bind(&OffscreenTab::DieIfContentCaptureEnded,
                 base::Unretained(this)));
}

void OffscreenTab::DieIfOriginalProfileDestroyed(Profile* profile) {
  DCHECK(profile == otr_profile_registration_->profile());
  owner_->DestroyTab(this);
}

}  // namespace extensions
