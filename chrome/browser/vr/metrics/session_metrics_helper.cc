// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/metrics/session_metrics_helper.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "components/rappor/public/rappor_utils.h"
#include "components/ukm/content/source_url_recorder.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace vr {

namespace {

const void* const kSessionMetricsHelperDataKey = &kSessionMetricsHelperDataKey;

// minimum duration: 7 seconds for video, no minimum for headset/vr modes
// maximum gap: 7 seconds between videos.  no gap for headset/vr-modes
constexpr base::TimeDelta kMinimumVideoSessionDuration(
    base::TimeDelta::FromSecondsD(7));
constexpr base::TimeDelta kMaximumVideoSessionGap(
    base::TimeDelta::FromSecondsD(7));

constexpr base::TimeDelta kMinimumHeadsetSessionDuration(
    base::TimeDelta::FromSecondsD(0));
constexpr base::TimeDelta kMaximumHeadsetSessionGap(
    base::TimeDelta::FromSecondsD(0));

// We have several different session times that share code in SessionTimer.
// Unfortunately, when the actual timer histogram is processed in
// UMA_HISTOGRAM_CUSTOM_TIMES, there is a function-static variable initialized
// with the name of the event, and all histograms going through the same
// function must share the same event name.
// In order to work around this and use different names, a templated function
// is used to get different function-static variables for each histogram name.
// Ideally this could be templated by the event name, but unfortunately
// C++ doesn't allow templates by strings.  Instead we template by enum, and
// have a function that translates enum to string.  For each template
// instantiation, the inlined function will be optimized to just access the
// string we want to return.
enum SessionEventName {
  MODE_FULLSCREEN,
  MODE_BROWSER,
  MODE_WEBVR,
  SESSION_VR,
  MODE_FULLSCREEN_WITH_VIDEO,
  MODE_BROWSER_WITH_VIDEO,
  MODE_WEBVR_WITH_VIDEO,
  SESSION_VR_WITH_VIDEO,
  MODE_FULLSCREEN_DLA,
  MODE_BROWSER_DLA,
  MODE_WEBVR_DLA,
  SESSION_VR_DLA,
};

const char* HistogramNameFromSessionType(SessionEventName name) {
  // TODO(crbug.com/790682): Migrate all of these to the "VR." namespace.
  static constexpr char kVrSession[] = "VRSessionTime";
  static constexpr char kWebVr[] = "VRSessionTime.WebVR";
  static constexpr char kBrowser[] = "VRSessionTime.Browser";
  static constexpr char kFullscreen[] = "VRSessionTime.Fullscreen";
  static constexpr char kVrSessionVideo[] = "VRSessionVideoTime";
  static constexpr char kWebVrVideo[] = "VRSessionVideoTime.WebVR";
  static constexpr char kBrowserVideo[] = "VRSessionVideoTime.Browser";
  static constexpr char kFullscreenVideo[] = "VRSessionVideoTime.Fullscreen";
  static constexpr char kVrSessionDla[] = "VRSessionTimeFromDLA";
  static constexpr char kWebVrDla[] = "VRSessionTimeFromDLA.WebVR";
  static constexpr char kBrowserDla[] = "VRSessionTimeFromDLA.Browser";
  static constexpr char kFullscreenDla[] = "VRSessionTimeFromDLA.Fullscreen";

  switch (name) {
    case MODE_FULLSCREEN:
      return kFullscreen;
    case MODE_BROWSER:
      return kBrowser;
    case MODE_WEBVR:
      return kWebVr;
    case SESSION_VR:
      return kVrSession;
    case MODE_FULLSCREEN_WITH_VIDEO:
      return kFullscreenVideo;
    case MODE_BROWSER_WITH_VIDEO:
      return kBrowserVideo;
    case MODE_WEBVR_WITH_VIDEO:
      return kWebVrVideo;
    case SESSION_VR_WITH_VIDEO:
      return kVrSessionVideo;
    case MODE_FULLSCREEN_DLA:
      return kFullscreenDla;
    case MODE_BROWSER_DLA:
      return kBrowserDla;
    case MODE_WEBVR_DLA:
      return kWebVrDla;
    case SESSION_VR_DLA:
      return kVrSessionDla;
    default:
      NOTREACHED();
      return nullptr;
  }
}

void SendRapporEnteredMode(const GURL& origin, Mode mode) {
  switch (mode) {
    case Mode::kVrBrowsingFullscreen:
      rappor::SampleDomainAndRegistryFromGURL(rappor::GetDefaultService(),
                                              "VR.FullScreenMode", origin);
      break;
    default:
      break;
  }
}

void SendRapporEnteredVideoMode(const GURL& origin, Mode mode) {
  switch (mode) {
    case Mode::kVrBrowsingRegular:
      rappor::SampleDomainAndRegistryFromGURL(rappor::GetDefaultService(),
                                              "VR.Video.Browser", origin);
      break;
    case Mode::kWebXrVrPresentation:
      rappor::SampleDomainAndRegistryFromGURL(rappor::GetDefaultService(),
                                              "VR.Video.WebVR", origin);
      break;
    case Mode::kVrBrowsingFullscreen:
      rappor::SampleDomainAndRegistryFromGURL(
          rappor::GetDefaultService(), "VR.Video.FullScreenMode", origin);
      break;
    default:
      break;
  }
}

// Handles the lifetime of the helper which is attached to a WebContents.
class SessionMetricsHelperData : public base::SupportsUserData::Data {
 public:
  explicit SessionMetricsHelperData(
      SessionMetricsHelper* session_metrics_helper)
      : session_metrics_helper_(session_metrics_helper) {}

  ~SessionMetricsHelperData() override { delete session_metrics_helper_; }

  SessionMetricsHelper* get() const { return session_metrics_helper_; }

 private:
  SessionMetricsHelper* session_metrics_helper_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(SessionMetricsHelperData);
};

}  // namespace

template <SessionEventName SessionType>
class SessionTimerImpl : public SessionTimer {
 public:
  SessionTimerImpl(base::TimeDelta gap_time, base::TimeDelta minimum_duration) {
    maximum_session_gap_time_ = gap_time;
    minimum_duration_ = minimum_duration;
  }

  ~SessionTimerImpl() override { StopSession(false, base::Time::Now()); }

  void SendAccumulatedSessionTime() override {
    if (!accumulated_time_.is_zero()) {
      UMA_HISTOGRAM_CUSTOM_TIMES(HistogramNameFromSessionType(SessionType),
                                 accumulated_time_, base::TimeDelta(),
                                 base::TimeDelta::FromHours(5), 100);
    }
  }
};

void SessionTimer::StartSession(base::Time start_time) {
  // If the new start time is within the minimum session gap time from the last
  // stop, continue the previous session.
  // Otherwise, start a new session, sending the event for the last session.
  if (!stop_time_.is_null() &&
      start_time - stop_time_ <= maximum_session_gap_time_) {
    // Mark the previous segment as non-continuable, sending data and clearing
    // state.
    StopSession(false, stop_time_);
  }

  start_time_ = start_time;
}

void SessionTimer::StopSession(bool continuable, base::Time stop_time) {
  // first accumulate time from this segment of the session
  base::TimeDelta segment_duration =
      (start_time_.is_null() ? base::TimeDelta() : stop_time - start_time_);
  if (!segment_duration.is_zero() && segment_duration > minimum_duration_) {
    accumulated_time_ = accumulated_time_ + segment_duration;
  }

  if (continuable) {
    // if we are continuable, accumulate the current segment to the session, and
    // set stop_time_ so we may continue later
    accumulated_time_ = stop_time - start_time_ + accumulated_time_;
    stop_time_ = stop_time;
    start_time_ = base::Time();
  } else {
    // send the histogram now if we aren't continuable, clearing segment state
    SendAccumulatedSessionTime();

    // clear out start/stop/accumulated time
    start_time_ = base::Time();
    stop_time_ = base::Time();
    accumulated_time_ = base::TimeDelta();
  }
}

// static
SessionMetricsHelper* SessionMetricsHelper::FromWebContents(
    content::WebContents* web_contents) {
  if (!web_contents)
    return NULL;
  SessionMetricsHelperData* data = static_cast<SessionMetricsHelperData*>(
      web_contents->GetUserData(kSessionMetricsHelperDataKey));
  return data ? data->get() : NULL;
}
SessionMetricsHelper* SessionMetricsHelper::CreateForWebContents(
    content::WebContents* contents,
    Mode initial_mode,
    bool started_with_autopresentation) {
  // This is not leaked as the SessionMetricsHelperData will clean it up.
  return new SessionMetricsHelper(contents, initial_mode,
                                  started_with_autopresentation);
}

SessionMetricsHelper::SessionMetricsHelper(content::WebContents* contents,
                                           Mode initial_mode,
                                           bool started_with_autopresentation) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(contents);

  num_videos_playing_ = contents->GetCurrentlyPlayingVideoCount();
  is_fullscreen_ = contents->IsFullscreen();
  origin_ = contents->GetLastCommittedURL();

  session_timer_ = std::make_unique<SessionTimerImpl<SESSION_VR>>(
      kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);

  is_webvr_ = initial_mode == Mode::kWebXrVrPresentation;
  is_vr_enabled_ = initial_mode != Mode::kNoVr;
  started_with_autopresentation_ = started_with_autopresentation;

  if (started_with_autopresentation) {
    session_timer_ = std::make_unique<SessionTimerImpl<SESSION_VR_DLA>>(
        kMaximumVideoSessionGap, kMinimumVideoSessionDuration);
  } else {
    session_timer_ = std::make_unique<SessionTimerImpl<SESSION_VR>>(
        kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);
  }
  session_video_timer_ =
      std::make_unique<SessionTimerImpl<SESSION_VR_WITH_VIDEO>>(
          kMaximumVideoSessionGap, kMinimumVideoSessionDuration);

  Observe(contents);
  contents->SetUserData(kSessionMetricsHelperDataKey,
                        std::make_unique<SessionMetricsHelperData>(this));

  UpdateMode();
}

SessionMetricsHelper::~SessionMetricsHelper() = default;

void SessionMetricsHelper::UpdateMode() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Mode mode;
  if (!is_vr_enabled_) {
    mode = Mode::kNoVr;
  } else if (is_webvr_) {
    mode = Mode::kWebXrVrPresentation;
  } else {
    mode =
        is_fullscreen_ ? Mode::kVrBrowsingFullscreen : Mode::kVrBrowsingRegular;
  }

  if (mode != mode_)
    SetVrMode(mode);
}

void SessionMetricsHelper::RecordVrStartAction(VrStartAction action) {
  if (!page_session_tracker_ || mode_ == Mode::kNoVr) {
    pending_page_session_start_action_ = action;
  } else {
    LogVrStartAction(action);
  }
}

void SessionMetricsHelper::RecordPresentationStartAction(
    PresentationStartAction action) {
  if (!presentation_session_tracker_ || mode_ != Mode::kWebXrVrPresentation) {
    pending_presentation_start_action_ = action;
  } else {
    LogPresentationStartAction(action);
  }
}

void SessionMetricsHelper::ReportRequestPresent() {
  // If we're not in VR, log this as an entry into VR from 2D.
  if (mode_ == Mode::kNoVr) {
    RecordVrStartAction(VrStartAction::kPresentationRequest);
    RecordPresentationStartAction(
        PresentationStartAction::kRequestFrom2dBrowsing);
  } else {
    RecordPresentationStartAction(
        PresentationStartAction::kRequestFromVrBrowsing);
  }
}

void SessionMetricsHelper::LogVrStartAction(VrStartAction action) {
  DCHECK(page_session_tracker_);

  UMA_HISTOGRAM_ENUMERATION("XR.VRSession.StartAction", action);
  if (action == VrStartAction::kHeadsetActivation ||
      action == VrStartAction::kPresentationRequest) {
    page_session_tracker_->ukm_entry()->SetEnteredVROnPageReason(
        static_cast<int>(action));
  }
}

void SessionMetricsHelper::LogPresentationStartAction(
    PresentationStartAction action) {
  DCHECK(presentation_session_tracker_);

  UMA_HISTOGRAM_ENUMERATION("XR.WebXR.PresentationSession", action);

  presentation_session_tracker_->ukm_entry()->SetStartAction(action);
}

void SessionMetricsHelper::SetWebVREnabled(bool is_webvr_presenting) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  is_webvr_ = is_webvr_presenting;
  UpdateMode();
}

void SessionMetricsHelper::SetVRActive(bool is_vr_enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  is_vr_enabled_ = is_vr_enabled;
  UpdateMode();
}

void SessionMetricsHelper::RecordVoiceSearchStarted() {
  num_voice_search_started_++;
}

void SessionMetricsHelper::RecordUrlRequested(GURL url,
                                              NavigationMethod method) {
  last_requested_url_ = url;
  last_url_request_method_ = method;
}

void SessionMetricsHelper::SetVrMode(Mode new_mode) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_NE(new_mode, mode_);
  DCHECK(new_mode == Mode::kVrBrowsingRegular ||
         new_mode == Mode::kVrBrowsingFullscreen ||
         new_mode == Mode::kWebXrVrPresentation || new_mode == Mode::kNoVr);

  base::Time switch_time = base::Time::Now();

  if (mode_ == Mode::kWebXrVrPresentation) {
    OnExitPresentation();
  }

  // If we are switching out of VR, stop all the session timers and record.
  if (new_mode == Mode::kNoVr) {
    OnExitAllVr();
  }

  // Stop the previous mode timers, if any.
  if (mode_ != Mode::kNoVr) {
    if (num_videos_playing_ > 0)
      mode_video_timer_->StopSession(false, switch_time);

    mode_timer_->StopSession(false, switch_time);
  }

  // Set the new trackers and timers.
  if (new_mode == Mode::kVrBrowsingRegular) {
    OnEnterRegularBrowsing();
  }

  if (new_mode == Mode::kVrBrowsingFullscreen) {
    OnEnterFullscreenBrowsing();
  }

  if (new_mode == Mode::kWebXrVrPresentation) {
    OnEnterPresentation();
  }

  // If we are switching from no VR to any kind of VR, start the new VR session
  // timers.
  if (mode_ == Mode::kNoVr) {
    OnEnterAnyVr();
  }

  // Start the new mode timers.
  if (new_mode != Mode::kNoVr) {
    mode_timer_->StartSession(switch_time);
    if (num_videos_playing_ > 0) {
      mode_video_timer_->StartSession(switch_time);
      SendRapporEnteredVideoMode(origin_, new_mode);
    }

    SendRapporEnteredMode(origin_, new_mode);
  }

  mode_ = new_mode;
}

void SessionMetricsHelper::OnEnterAnyVr() {
  base::Time switch_time = base::Time::Now();
  session_timer_->StartSession(switch_time);
  num_session_video_playback_ = 0;
  num_session_navigation_ = 0;
  num_voice_search_started_ = 0;

  if (num_videos_playing_ > 0) {
    session_video_timer_->StartSession(switch_time);
    num_session_video_playback_ = num_videos_playing_;
  }

  page_session_tracker_ =
      std::make_unique<SessionTracker<ukm::builders::XR_PageSession>>(
          std::make_unique<ukm::builders::XR_PageSession>(
              ukm::GetSourceIdForWebContentsDocument(web_contents())));
  if (pending_page_session_start_action_) {
    LogVrStartAction(*pending_page_session_start_action_);
    pending_page_session_start_action_ = base::nullopt;
  }
}

void SessionMetricsHelper::OnExitAllVr() {
  base::Time switch_time = base::Time::Now();
  if (num_videos_playing_ > 0)
    session_video_timer_->StopSession(false, switch_time);

  session_timer_->StopSession(false, switch_time);

  UMA_HISTOGRAM_COUNTS_100("VRSessionVideoCount", num_session_video_playback_);
  UMA_HISTOGRAM_COUNTS_100("VRSessionNavigationCount", num_session_navigation_);
  UMA_HISTOGRAM_COUNTS_100("VR.Session.VoiceSearch.StartedCount",
                           num_voice_search_started_);

  // Do not assume page_session_tracker_ is set because it's possible that it
  // is null if DidStartNavigation has already submitted and cleared
  // page_session_tracker and DidFinishNavigation has not yet created the new
  // one.
  if (page_session_tracker_) {
    page_session_tracker_->SetSessionEnd(switch_time);
    page_session_tracker_->ukm_entry()->SetDuration(
        page_session_tracker_->GetRoundedDurationInSeconds());
    page_session_tracker_->RecordEntry();
    page_session_tracker_ = nullptr;
  }
}

void SessionMetricsHelper::OnEnterRegularBrowsing() {
  if (started_with_autopresentation_) {
    mode_timer_ = std::make_unique<SessionTimerImpl<MODE_BROWSER_DLA>>(
        kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);
  } else {
    mode_timer_ = std::make_unique<SessionTimerImpl<MODE_BROWSER>>(
        kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);
  }
  mode_video_timer_ =
      std::make_unique<SessionTimerImpl<MODE_BROWSER_WITH_VIDEO>>(
          kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);
}

void SessionMetricsHelper::OnEnterPresentation() {
  if (started_with_autopresentation_) {
    mode_timer_ = std::make_unique<SessionTimerImpl<MODE_WEBVR_DLA>>(
        kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);
  } else {
    mode_timer_ = std::make_unique<SessionTimerImpl<MODE_WEBVR>>(
        kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);
  }

  mode_video_timer_ = std::make_unique<SessionTimerImpl<MODE_WEBVR_WITH_VIDEO>>(
      kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);

  // If we are switching to WebVR presentation, start the new presentation
  // session.
  presentation_session_tracker_ = std::make_unique<
      SessionTracker<ukm::builders::XR_WebXR_PresentationSession>>(
      std::make_unique<ukm::builders::XR_WebXR_PresentationSession>(
          ukm::GetSourceIdForWebContentsDocument(web_contents())));

  if (!pending_presentation_start_action_) {
    pending_presentation_start_action_ = PresentationStartAction::kOther;
  }

  LogPresentationStartAction(*pending_presentation_start_action_);
  pending_presentation_start_action_ = base::nullopt;
}

void SessionMetricsHelper::OnExitPresentation() {
  // If we are switching off WebVR presentation, then the presentation session
  // is done. As with the page session, do not assume
  // presentation_session_tracker_ is valid.
  if (presentation_session_tracker_) {
    presentation_session_tracker_->SetSessionEnd(base::Time::Now());
    presentation_session_tracker_->ukm_entry()->SetDuration(
        presentation_session_tracker_->GetRoundedDurationInSeconds());
    presentation_session_tracker_->RecordEntry();
    presentation_session_tracker_ = nullptr;
  }
}

void SessionMetricsHelper::OnEnterFullscreenBrowsing() {
  if (started_with_autopresentation_) {
    mode_timer_ = std::make_unique<SessionTimerImpl<MODE_FULLSCREEN_DLA>>(
        kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);
  } else {
    mode_timer_ = std::make_unique<SessionTimerImpl<MODE_FULLSCREEN>>(
        kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);
  }

  mode_video_timer_ =
      std::make_unique<SessionTimerImpl<MODE_FULLSCREEN_WITH_VIDEO>>(
          kMaximumHeadsetSessionGap, kMinimumHeadsetSessionDuration);

  if (page_session_tracker_)
    page_session_tracker_->ukm_entry()->SetEnteredFullscreen(1);
}

void SessionMetricsHelper::MediaStartedPlaying(
    const MediaPlayerInfo& media_info,
    const MediaPlayerId&) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!media_info.has_video)
    return;

  if (num_videos_playing_ == 0) {
    // started playing video - start sessions
    base::Time start_time = base::Time::Now();

    if (mode_ != Mode::kNoVr) {
      session_video_timer_->StartSession(start_time);
      mode_video_timer_->StartSession(start_time);
      SendRapporEnteredVideoMode(origin_, mode_);
    }
  }

  num_videos_playing_++;
  num_session_video_playback_++;
}

void SessionMetricsHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& media_info,
    const MediaPlayerId&,
    WebContentsObserver::MediaStoppedReason reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!media_info.has_video)
    return;

  num_videos_playing_--;

  if (num_videos_playing_ == 0) {
    // stopped playing video - update existing video sessions
    base::Time stop_time = base::Time::Now();

    if (mode_ != Mode::kNoVr) {
      session_video_timer_->StopSession(true, stop_time);
      mode_video_timer_->StopSession(true, stop_time);
    }
  }
}

void SessionMetricsHelper::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (handle && handle->IsInMainFrame() && !handle->IsSameDocument()) {
    if (page_session_tracker_) {
      page_session_tracker_->SetSessionEnd(base::Time::Now());
      page_session_tracker_->ukm_entry()->SetDuration(
          page_session_tracker_->GetRoundedDurationInSeconds());
      page_session_tracker_->RecordEntry();
      page_session_tracker_ = nullptr;
    }

    if (presentation_session_tracker_) {
      presentation_session_tracker_->SetSessionEnd(base::Time::Now());
      presentation_session_tracker_->ukm_entry()->SetDuration(
          presentation_session_tracker_->GetRoundedDurationInSeconds());
      presentation_session_tracker_->RecordEntry();
      presentation_session_tracker_ = nullptr;
    }
  }
}

void SessionMetricsHelper::DidFinishNavigation(
    content::NavigationHandle* handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Counting the number of pages viewed is difficult - some websites load
  // new content dynamically without a navigation.  Others redirect several
  // times for a single navigation.
  // We look at the number of committed navigations in the main frame, which
  // will slightly overestimate pages viewed instead of trying to filter or
  // look at page loads, since those will underestimate on some pages, and
  // overestimate on others.
  if (handle && handle->HasCommitted() && handle->IsInMainFrame()) {
    origin_ = handle->GetURL();

    // Get the ukm::SourceId from the handle so that we don't wind up with a
    // wrong ukm::SourceId from this WebContentObserver perhaps executing after
    // another which changes the SourceId.
    page_session_tracker_ =
        std::make_unique<SessionTracker<ukm::builders::XR_PageSession>>(
            std::make_unique<ukm::builders::XR_PageSession>(
                ukm::GetSourceIdForWebContentsDocument(web_contents())));
    if (pending_page_session_start_action_) {
      LogVrStartAction(*pending_page_session_start_action_);
      pending_page_session_start_action_ = base::nullopt;
    }

    // Check that the completed navigation is indeed the one that was requested
    // by either voice or omnibox entry, in case the requested navigation was
    // incomplete when another was begun. Check against the first entry for the
    // navigation, as redirects might have changed what the URL looks like.
    if (last_requested_url_ == handle->GetRedirectChain().front()) {
      switch (last_url_request_method_) {
        case kOmniboxUrlEntry:
        case kOmniboxSuggestionSelected:
          page_session_tracker_->ukm_entry()->SetWasOmniboxNavigation(1);
          break;
        case kVoiceSearch:
          page_session_tracker_->ukm_entry()->SetWasVoiceSearchNavigation(1);
          break;
      }
    }
    last_requested_url_ = GURL();

    if (mode_ == Mode::kWebXrVrPresentation) {
      presentation_session_tracker_ = std::make_unique<
          SessionTracker<ukm::builders::XR_WebXR_PresentationSession>>(
          std::make_unique<ukm::builders::XR_WebXR_PresentationSession>(
              ukm::GetSourceIdForWebContentsDocument(web_contents())));
      if (pending_presentation_start_action_) {
        presentation_session_tracker_->ukm_entry()->SetStartAction(
            *pending_presentation_start_action_);
        pending_presentation_start_action_ = base::nullopt;
      }
    }

    num_session_navigation_++;
  }
}

void SessionMetricsHelper::DidToggleFullscreenModeForTab(
    bool entered_fullscreen,
    bool will_cause_resize) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  is_fullscreen_ = entered_fullscreen;
  UpdateMode();
}

}  // namespace vr
