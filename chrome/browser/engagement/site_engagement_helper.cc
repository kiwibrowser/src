// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/engagement/site_engagement_helper.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/prerender/prerender_contents.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

int g_seconds_to_pause_engagement_detection = 10;
int g_seconds_delay_after_navigation = 10;
int g_seconds_delay_after_media_starts = 10;
int g_seconds_delay_after_show = 5;

}  // anonymous namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SiteEngagementService::Helper);

// static
void SiteEngagementService::Helper::SetSecondsBetweenUserInputCheck(
    int seconds) {
  g_seconds_to_pause_engagement_detection = seconds;
}

// static
void SiteEngagementService::Helper::SetSecondsTrackingDelayAfterNavigation(
    int seconds) {
  g_seconds_delay_after_navigation = seconds;
}

// static
void SiteEngagementService::Helper::SetSecondsTrackingDelayAfterShow(
    int seconds) {
  g_seconds_delay_after_show = seconds;
}

SiteEngagementService::Helper::~Helper() {
  service_->HelperDeleted(this);
  if (web_contents()) {
    input_tracker_.Stop();
    media_tracker_.Stop();
  }
}

void SiteEngagementService::Helper::OnEngagementLevelChanged(
    const GURL& url,
    blink::mojom::EngagementLevel level) {
  web_contents()->ForEachFrame(base::BindRepeating(
      &SiteEngagementService::Helper::SendEngagementLevelToFramesMatchingOrigin,
      base::Unretained(this), url::Origin::Create(url), level));
}

SiteEngagementService::Helper::PeriodicTracker::PeriodicTracker(
    SiteEngagementService::Helper* helper)
    : helper_(helper), pause_timer_(new base::Timer(true, false)) {}

SiteEngagementService::Helper::PeriodicTracker::~PeriodicTracker() {}

void SiteEngagementService::Helper::PeriodicTracker::Start(
    base::TimeDelta initial_delay) {
  StartTimer(initial_delay);
}

void SiteEngagementService::Helper::PeriodicTracker::Pause() {
  TrackingStopped();
  StartTimer(
      base::TimeDelta::FromSeconds(g_seconds_to_pause_engagement_detection));
}

void SiteEngagementService::Helper::PeriodicTracker::Stop() {
  TrackingStopped();
  pause_timer_->Stop();
}

bool SiteEngagementService::Helper::PeriodicTracker::IsTimerRunning() {
  return pause_timer_->IsRunning();
}

void SiteEngagementService::Helper::PeriodicTracker::SetPauseTimerForTesting(
    std::unique_ptr<base::Timer> timer) {
  pause_timer_ = std::move(timer);
}

void SiteEngagementService::Helper::PeriodicTracker::StartTimer(
    base::TimeDelta delay) {
  pause_timer_->Start(
      FROM_HERE, delay,
      base::Bind(
          &SiteEngagementService::Helper::PeriodicTracker::TrackingStarted,
          base::Unretained(this)));
}

SiteEngagementService::Helper::InputTracker::InputTracker(
    SiteEngagementService::Helper* helper,
    content::WebContents* web_contents)
    : PeriodicTracker(helper),
      content::WebContentsObserver(web_contents),
      is_tracking_(false) {}

void SiteEngagementService::Helper::InputTracker::TrackingStarted() {
  is_tracking_ = true;
}

void SiteEngagementService::Helper::InputTracker::TrackingStopped() {
  is_tracking_ = false;
}

// Record that there was some user input, and defer handling of the input event.
// Once the timer finishes running, the callbacks detecting user input will be
// registered again.
void SiteEngagementService::Helper::InputTracker::DidGetUserInteraction(
    const blink::WebInputEvent::Type type) {
  // Only respond to raw key down to avoid multiple triggering on a single input
  // (e.g. keypress is a key down then key up).
  if (!is_tracking_)
    return;

  // This switch has a default NOTREACHED case because it will not test all
  // of the values of the WebInputEvent::Type enum (hence it won't require the
  // compiler verifying that all cases are covered).
  switch (type) {
    case blink::WebInputEvent::kRawKeyDown:
      helper()->RecordUserInput(SiteEngagementService::ENGAGEMENT_KEYPRESS);
      break;
    case blink::WebInputEvent::kMouseDown:
      helper()->RecordUserInput(SiteEngagementService::ENGAGEMENT_MOUSE);
      break;
    case blink::WebInputEvent::kTouchStart:
      helper()->RecordUserInput(
          SiteEngagementService::ENGAGEMENT_TOUCH_GESTURE);
      break;
    case blink::WebInputEvent::kGestureScrollBegin:
      helper()->RecordUserInput(SiteEngagementService::ENGAGEMENT_SCROLL);
      break;
    case blink::WebInputEvent::kUndefined:
      // Explicitly ignore browser-initiated navigation input.
      break;
    default:
      NOTREACHED();
  }
  Pause();
}

SiteEngagementService::Helper::MediaTracker::MediaTracker(
    SiteEngagementService::Helper* helper,
    content::WebContents* web_contents)
    : PeriodicTracker(helper), content::WebContentsObserver(web_contents) {}

SiteEngagementService::Helper::MediaTracker::~MediaTracker() {}

void SiteEngagementService::Helper::MediaTracker::TrackingStarted() {
  if (!active_media_players_.empty()) {
    // TODO(dominickn): Consider treating OCCLUDED tabs like HIDDEN tabs when
    // computing engagement score. They are currently treated as VISIBLE tabs to
    // preserve old behavior.
    helper()->RecordMediaPlaying(web_contents()->GetVisibility() ==
                                 content::Visibility::HIDDEN);
  }

  Pause();
}

void SiteEngagementService::Helper::MediaTracker::DidFinishNavigation(
    content::NavigationHandle* handle) {
  // Ignore subframe navigation to avoid clearing main frame active media
  // players when they navigate.
  if (!handle->HasCommitted() || !handle->IsInMainFrame() ||
      handle->IsSameDocument()) {
    return;
  }

  // Media stops playing on navigation, so clear our state.
  active_media_players_.clear();
}

void SiteEngagementService::Helper::MediaTracker::MediaStartedPlaying(
    const MediaPlayerInfo& media_info,
    const MediaPlayerId& id) {
  // Only begin engagement detection when media actually starts playing.
  active_media_players_.push_back(id);
  if (!IsTimerRunning())
    Start(base::TimeDelta::FromSeconds(g_seconds_delay_after_media_starts));
}

void SiteEngagementService::Helper::MediaTracker::MediaStoppedPlaying(
    const MediaPlayerInfo& media_info,
    const MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason reason) {
  active_media_players_.erase(std::remove(active_media_players_.begin(),
                                          active_media_players_.end(), id),
                              active_media_players_.end());
}

SiteEngagementService::Helper::Helper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      input_tracker_(this, web_contents),
      media_tracker_(this, web_contents),
      service_(SiteEngagementService::Get(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()))) {
  service_->HelperCreated(this);
}

void SiteEngagementService::Helper::RecordUserInput(
    SiteEngagementService::EngagementType type) {
  TRACE_EVENT0("SiteEngagement", "RecordUserInput");
  content::WebContents* contents = web_contents();
  if (contents)
    service_->HandleUserInput(contents, type);
}

void SiteEngagementService::Helper::RecordMediaPlaying(bool is_hidden) {
  content::WebContents* contents = web_contents();
  if (contents)
    service_->HandleMediaPlaying(contents, is_hidden);
}

void SiteEngagementService::Helper::SendEngagementLevelToFramesMatchingOrigin(
    const url::Origin& origin,
    blink::mojom::EngagementLevel level,
    content::RenderFrameHost* render_frame_host) {
  if (origin == render_frame_host->GetLastCommittedOrigin()) {
    SendEngagementLevelToFrame(origin, level, render_frame_host);
  }
}

void SiteEngagementService::Helper::SendEngagementLevelToFrame(
    const url::Origin& origin,
    blink::mojom::EngagementLevel level,
    content::RenderFrameHost* render_frame_host) {
  blink::mojom::EngagementClientAssociatedPtr client;
  render_frame_host->GetRemoteAssociatedInterfaces()->GetInterface(&client);
  client->SetEngagementLevel(origin, level);
}

void SiteEngagementService::Helper::DidFinishNavigation(
    content::NavigationHandle* handle) {
  // Ignore uncommitted, non main-frame, same page, or error page navigations.
  if (!handle->HasCommitted() || !handle->IsInMainFrame() ||
      handle->IsSameDocument() || handle->IsErrorPage()) {
    return;
  }

  input_tracker_.Stop();
  media_tracker_.Stop();

  // Ignore prerender loads. This means that prerenders will not receive
  // navigation engagement. The implications are as follows:
  //
  // - Instant search prerenders from the omnibox trigger DidFinishNavigation
  //   twice: once for the prerender, and again when the page swaps in. The
  //   second trigger has transition GENERATED and receives navigation
  //   engagement.
  // - Prerenders initiated by <link rel="prerender"> (e.g. search results) are
  //   always assigned the LINK transition, which is ignored for navigation
  //   engagement.
  //
  // Prerenders trigger WasShown() when they are swapped in, so input engagement
  // will activate even if navigation engagement is not scored.
  if (prerender::PrerenderContents::FromWebContents(web_contents()) != nullptr)
    return;

  service_->HandleNavigation(web_contents(), handle->GetPageTransition());

  input_tracker_.Start(
      base::TimeDelta::FromSeconds(g_seconds_delay_after_navigation));
}

void SiteEngagementService::Helper::ReadyToCommitNavigation(
    content::NavigationHandle* handle) {
  if (service_->ShouldRecordEngagement(handle->GetURL())) {
    // Don't bother sending the engagement if we wouldn't have recorded any for
    // the URL. These will have NONE engagement by default.
    SendEngagementLevelToFrame(url::Origin::Create(handle->GetURL()),
                               service_->GetEngagementLevel(handle->GetURL()),
                               handle->GetRenderFrameHost());
  }
}

void SiteEngagementService::Helper::OnVisibilityChanged(
    content::Visibility visibility) {
  // TODO(fdoray): Once the page visibility API [1] treats hidden and occluded
  // documents the same way, consider stopping |input_tracker_| when
  // |visibility| is OCCLUDED. https://crbug.com/668690
  // [1] https://developer.mozilla.org/en-US/docs/Web/API/Page_Visibility_API
  if (visibility == content::Visibility::HIDDEN) {
    input_tracker_.Stop();
  } else {
    // Start a timer to track input if it isn't already running and input isn't
    // already being tracked.
    if (!input_tracker_.IsTimerRunning() && !input_tracker_.is_tracking()) {
      input_tracker_.Start(
          base::TimeDelta::FromSeconds(g_seconds_delay_after_show));
    }
  }
}
