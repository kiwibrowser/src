// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_AUTOPLAY_UMA_HELPER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_AUTOPLAY_UMA_HELPER_H_

#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

#include <set>

namespace blink {

// These values are used for histograms. Do not reorder.
enum class AutoplaySource {
  // Autoplay comes from HTMLMediaElement `autoplay` attribute.
  kAttribute = 0,
  // Autoplay comes from `play()` method.
  kMethod = 1,
  // Used for checking dual source.
  kNumberOfSources = 2,
  // Both sources are used.
  kDualSource = 2,
  // This enum value must be last.
  kNumberOfUmaSources = 3,
};

// These values are used for histograms. Do not reorder.
enum class AutoplayUnmuteActionStatus {
  kFailure = 0,
  kSuccess = 1,
  kNumberOfStatus = 2,
};

// These values are used for histograms. Do not reorder.
enum AutoplayBlockedReason {
  kAutoplayBlockedReasonDataSaver = 0,
  kAutoplayBlockedReasonSetting = 1,
  kAutoplayBlockedReasonDataSaverAndSetting = 2,
  // Keey at the end.
  kAutoplayBlockedReasonMax = 3
};

enum class CrossOriginAutoplayResult {
  kAutoplayAllowed = 0,
  kAutoplayBlocked = 1,
  kPlayedWithGesture = 2,
  kUserPaused = 3,
  // Keep at the end.
  kNumberOfResults = 4,
};

class Document;
class ElementVisibilityObserver;
class HTMLMediaElement;

class CORE_EXPORT AutoplayUmaHelper : public EventListener,
                                      public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(AutoplayUmaHelper);

 public:
  static AutoplayUmaHelper* Create(HTMLMediaElement*);

  ~AutoplayUmaHelper() override;

  bool operator==(const EventListener&) const override;

  void ContextDestroyed(ExecutionContext*) override;

  void OnAutoplayInitiated(AutoplaySource);

  void RecordCrossOriginAutoplayResult(CrossOriginAutoplayResult);
  void RecordAutoplayUnmuteStatus(AutoplayUnmuteActionStatus);

  void VideoWillBeDrawnToCanvas();
  void DidMoveToNewDocument(Document& old_document);

  bool IsVisible() const { return is_visible_; }

  bool HasSource() const { return !sources_.empty(); }

  void Trace(blink::Visitor*) override;

 private:
  friend class MockAutoplayUmaHelper;

  // Called when source is initialized and loading starts.
  void OnLoadStarted();

  explicit AutoplayUmaHelper(HTMLMediaElement*);
  void handleEvent(ExecutionContext*, Event*) override;
  void HandlePlayingEvent();
  void HandlePauseEvent();
  virtual void HandleContextDestroyed();  // Make virtual for testing.

  void MaybeUnregisterContextDestroyedObserver();
  void MaybeUnregisterMediaElementPauseListener();

  void MaybeStartRecordingMutedVideoPlayMethodBecomeVisible();
  void MaybeStopRecordingMutedVideoPlayMethodBecomeVisible(bool is_visible);

  void MaybeStartRecordingMutedVideoOffscreenDuration();
  void MaybeStopRecordingMutedVideoOffscreenDuration();

  void MaybeRecordUserPausedAutoplayingCrossOriginVideo();

  void OnVisibilityChangedForMutedVideoOffscreenDuration(bool is_visibile);
  void OnVisibilityChangedForMutedVideoPlayMethodBecomeVisible(bool is_visible);

  bool ShouldListenToContextDestroyed() const;
  bool ShouldRecordUserPausedAutoplayingCrossOriginVideo() const;

  // The autoplay sources.
  std::set<AutoplaySource> sources_;

  // The media element this UMA helper is attached to. |element| owns |this|.
  Member<HTMLMediaElement> element_;

  // The observer is used to observe whether a muted video autoplaying by play()
  // method become visible at some point.
  // The UMA is pending for recording as long as this observer is non-null.
  Member<ElementVisibilityObserver>
      muted_video_play_method_visibility_observer_;

  // -----------------------------------------------------------------------
  // Variables used for recording the duration of autoplay muted video playing
  // offscreen.  The variables are valid when
  // |autoplayOffscrenVisibilityObserver| is non-null.
  // The recording stops whenever the playback pauses or the page is unloaded.

  // The starting time of autoplaying muted video.
  int64_t muted_video_autoplay_offscreen_start_time_ms_;

  // The duration an autoplaying muted video has been in offscreen.
  int64_t muted_video_autoplay_offscreen_duration_ms_;

  // Whether an autoplaying muted video is visible.
  bool is_visible_;

  std::set<CrossOriginAutoplayResult> recorded_cross_origin_autoplay_results_;

  // The observer is used to observer an autoplaying muted video changing it's
  // visibility, which is used for offscreen duration UMA.  The UMA is pending
  // for recording as long as this observer is non-null.
  Member<ElementVisibilityObserver>
      muted_video_offscreen_duration_visibility_observer_;

  double load_start_time_ms_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_AUTOPLAY_UMA_HELPER_H_
