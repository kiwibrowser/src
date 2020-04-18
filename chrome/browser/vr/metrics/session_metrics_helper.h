// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_METRICS_SESSION_METRICS_HELPER_H_
#define CHROME_BROWSER_VR_METRICS_SESSION_METRICS_HELPER_H_

#include <memory>

#include "base/time/time.h"
#include "chrome/browser/vr/mode.h"
#include "chrome/browser/vr/ui_browser_interface.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "url/gurl.h"

namespace vr {

// This enum describes various ways a Chrome VR session started.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
// Ensure that this stays in sync with VRSessionStartAction in enums.xml
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.vr_shell
enum class VrStartAction : int {
  // The user activated a headset. For example, inserted phone in Daydream, or
  // put on an Occulus or Vive.
  kHeadsetActivation = 1,
  // The user triggered a presentation request on a page, probably by clicking
  // an enter VR button.
  kPresentationRequest = 2,
  // The user launched a deep linked app, probably from Daydream home.
  kDeepLinkedApp = 3,
  // Chrome VR was started by an intent from another app. Most likely the user
  // clicked the icon in Daydream home.
  kIntentLaunch = 4,
  kMaxValue = kIntentLaunch,
};

// The source of a request to enter XR Presentation.
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
// Ensure that this stays in sync with VRPresentationStartAction in enums.xml.
enum PresentationStartAction {
  // A catch all for methods of Presentation entry that are not otherwise
  // logged.
  kOther = 0,
  // The user triggered a presentation request on a page in 2D, probably by
  // clicking an enter VR button.
  kRequestFrom2dBrowsing = 1,
  // The user triggered a presentation request on a page in VR browsing,
  // probably by clicking an enter VR button.
  kRequestFromVrBrowsing = 2,
  // The user activated a headset on a page that listens for headset activations
  // and requests presentation.
  kHeadsetActivation = 3,
  // The user opened a deep linked app, probably from the Daydream homescreen.
  kDeepLinkedApp = 4,
  kMaxValue = kDeepLinkedApp,
};

// SessionTimer will monitor the time between calls to StartSession and
// StopSession.  It will combine multiple segments into a single session if they
// are sufficiently close in time.  It will also only include segments if they
// are sufficiently long.
// Because the session may be extended, the accumulated time is occasionally
// sent on destruction or when a new session begins.
class SessionTimer {
 public:
  virtual ~SessionTimer() {}

  void StartSession(base::Time start_time);
  void StopSession(bool continuable, base::Time stop_time);

 protected:
  SessionTimer() {}

  virtual void SendAccumulatedSessionTime() = 0;

  base::Time start_time_;
  base::Time stop_time_;
  base::TimeDelta accumulated_time_;

  // Config members.
  // Maximum time gap allowed between a StopSession and a StartSession before it
  // will be logged as a seperate session.
  base::TimeDelta maximum_session_gap_time_;

  // Minimum time between a StartSession and StopSession required before it is
  // added to the duration.
  base::TimeDelta minimum_duration_;

  DISALLOW_COPY_AND_ASSIGN(SessionTimer);
};

// SessionTracker tracks UKM data for sessions and sends the data upon request.
template <class T>
class SessionTracker {
 public:
  explicit SessionTracker(std::unique_ptr<T> entry)
      : ukm_entry_(std::move(entry)),
        start_time_(base::Time::Now()),
        stop_time_(base::Time::Now()) {}
  virtual ~SessionTracker() {}
  T* ukm_entry() { return ukm_entry_.get(); }
  void SetSessionEnd(base::Time stop_time) { stop_time_ = stop_time; }

  int GetRoundedDurationInSeconds() {
    if (start_time_ > stop_time_) {
      // Return negative one to indicate an invalid value was recorded.
      return -1;
    }

    base::TimeDelta duration = stop_time_ - start_time_;

    if (duration.InHours() > 1) {
      return duration.InHours() * 3600;
    } else if (duration.InMinutes() > 10) {
      return (duration.InMinutes() / 10) * 10 * 60;
    } else if (duration.InSeconds() > 60) {
      return duration.InMinutes() * 60;
    } else {
      return duration.InSeconds();
    }
  }

  void RecordEntry() {
    ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
    DCHECK(ukm_recorder);

    ukm_entry_->Record(ukm_recorder);
  }

 protected:
  std::unique_ptr<T> ukm_entry_;

  base::Time start_time_;
  base::Time stop_time_;

  DISALLOW_COPY_AND_ASSIGN(SessionTracker);
};

// This class is not thread-safe and must only be used from the main thread.
// This class tracks metrics for various kinds of sessions, including VR
// browsing sessions, WebXR presentation sessions, and others. It mainly tracks
// metrics that require state monitoring, such as durations, but also tracks
// data we want attached to that, such as number of videos watched and how the
// session was started.
class SessionMetricsHelper : public content::WebContentsObserver {
 public:
  // Returns the SessionMetricsHelper singleton if it has been created for the
  // WebContents.
  static SessionMetricsHelper* FromWebContents(content::WebContents* contents);
  static SessionMetricsHelper* CreateForWebContents(
      content::WebContents* contents,
      Mode initial_mode,
      bool started_with_autopresentation);

  ~SessionMetricsHelper() override;

  void SetWebVREnabled(bool is_webvr_presenting);
  void SetVRActive(bool is_vr_enabled);
  void RecordVoiceSearchStarted();
  void RecordUrlRequested(GURL url, NavigationMethod method);

  void RecordVrStartAction(VrStartAction action);
  void RecordPresentationStartAction(PresentationStartAction action);
  void ReportRequestPresent();

 private:
  SessionMetricsHelper(content::WebContents* contents,
                       Mode initial_mode,
                       bool started_with_autopresentation);

  // WebContentObserver
  void MediaStartedPlaying(const MediaPlayerInfo& media_info,
                           const MediaPlayerId&) override;
  void MediaStoppedPlaying(
      const MediaPlayerInfo& media_info,
      const MediaPlayerId&,
      WebContentsObserver::MediaStoppedReason reason) override;
  void DidStartNavigation(content::NavigationHandle* handle) override;
  void DidFinishNavigation(content::NavigationHandle* handle) override;
  void DidToggleFullscreenModeForTab(bool entered_fullscreen,
                                     bool will_cause_resize) override;

  void SetVrMode(Mode mode);
  void UpdateMode();

  void LogVrStartAction(VrStartAction action);
  void LogPresentationStartAction(PresentationStartAction action);

  void OnEnterAnyVr();
  void OnExitAllVr();
  void OnEnterRegularBrowsing();
  void OnEnterPresentation();
  void OnExitPresentation();
  void OnEnterFullscreenBrowsing();

  std::unique_ptr<SessionTimer> mode_video_timer_;
  std::unique_ptr<SessionTimer> session_video_timer_;
  std::unique_ptr<SessionTimer> mode_timer_;
  std::unique_ptr<SessionTimer> session_timer_;

  std::unique_ptr<SessionTracker<ukm::builders::XR_PageSession>>
      page_session_tracker_;
  std::unique_ptr<SessionTracker<ukm::builders::XR_WebXR_PresentationSession>>
      presentation_session_tracker_;

  Mode mode_ = Mode::kNoVr;

  // State that gets translated into the VR mode.
  bool is_fullscreen_ = false;
  bool is_webvr_ = false;
  bool is_vr_enabled_ = false;
  bool started_with_autopresentation_ = false;

  GURL last_requested_url_;
  NavigationMethod last_url_request_method_;

  base::Optional<VrStartAction> pending_page_session_start_action_;
  base::Optional<PresentationStartAction> pending_presentation_start_action_;

  int num_videos_playing_ = 0;
  int num_session_navigation_ = 0;
  int num_session_video_playback_ = 0;
  int num_voice_search_started_ = 0;

  GURL origin_;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_METRICS_SESSION_METRICS_HELPER_H_
