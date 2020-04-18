// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_WATCH_TIME_REPORTER_H_
#define MEDIA_BLINK_WATCH_TIME_REPORTER_H_

#include <vector>

#include "base/callback.h"
#include "base/power_monitor/power_observer.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "media/base/audio_codecs.h"
#include "media/base/media_log.h"
#include "media/base/timestamp_constants.h"
#include "media/base/video_codecs.h"
#include "media/blink/media_blink_export.h"
#include "media/mojo/interfaces/media_metrics_provider.mojom.h"
#include "media/mojo/interfaces/watch_time_recorder.mojom.h"
#include "third_party/blink/public/platform/web_media_player.h"
#include "ui/gfx/geometry/size.h"
#include "url/origin.h"

namespace media {

// Class for monitoring and reporting watch time in response to various state
// changes during the playback of media. We record metrics for audio only
// playbacks as well as video only or audio+video playbacks of sufficient size.
//
// Watch time for our purposes is defined as the amount of elapsed media time.
// Any amount of elapsed time is reported to the WatchTimeRecorder, but only
// amounts above limits::kMinimumElapsedWatchTimeSecs are reported to UMA. Watch
// time is checked every 5 seconds from then on and reported to multiple
// buckets: All, MSE, SRC, EME, AC, and battery.
//
// Either of paused or muted is sufficient to stop watch time metric reports.
// Each of these has a hysteresis where if the state change is undone within 5
// seconds, the watch time will be counted as uninterrupted.
//
// There are both foreground and background buckets for watch time. E.g., when
// media goes into the background foreground collection stops and background
// collection starts. As with other events, there is hysteresis on change
// between the foreground and background.
//
// Similarly, there are both muted and unmuted buckets for watch time. E.g., if
// a playback is muted the unmuted collection stops and muted collection starts.
// As with other events, there is hysteresis between mute and unmute.
//
// Power events (on/off battery power), native controls changes, or display type
// changes have a similar hysteresis, but unlike the aforementioned properties,
// will not stop metric collection.
//
// Each seek event will result in a new watch time metric being started and the
// old metric finalized as accurately as possible.
class MEDIA_BLINK_EXPORT WatchTimeReporter : base::PowerObserver {
 public:
  using GetMediaTimeCB = base::RepeatingCallback<base::TimeDelta(void)>;

  // Constructor for the reporter; all requested metadata should be fully known
  // before attempting construction as incorrect values will result in the wrong
  // watch time metrics being reported.
  //
  // |properties| Properties describing the playback; these are considered
  // immutable over the lifetime of the reporter. If any of them change a new
  // WatchTimeReporter should be created with updated properties.
  //
  // |get_media_time_cb| must return the current playback time in terms of media
  // time, not wall clock time! Using media time instead of wall clock time
  // allows us to avoid a whole class of issues around clock changes during
  // suspend and resume.
  //
  // |provider| A provider of mojom::WatchTimeRecorder instances which will be
  // created and used to handle caching of metrics outside of the current
  // process.
  //
  // TODO(dalecurtis): Should we only report when rate == 1.0? Should we scale
  // the elapsed media time instead?
  WatchTimeReporter(mojom::PlaybackPropertiesPtr properties,
                    GetMediaTimeCB get_media_time_cb,
                    mojom::MediaMetricsProvider* provider,
                    scoped_refptr<base::SequencedTaskRunner> task_runner,
                    const base::TickClock* tick_clock = nullptr);
  ~WatchTimeReporter() override;

  // These methods are used to ensure that watch time is only reported for media
  // that is actually playing. They should be called whenever the media starts
  // or stops playing for any reason. If the media is currently hidden,
  // OnPlaying() will start background watch time reporting.
  void OnPlaying();
  void OnPaused();

  // This will immediately finalize any outstanding watch time reports and stop
  // the reporting timer. Clients should call OnPlaying() upon seek completion
  // to restart the reporting timer.
  void OnSeeking();

  // This method is used to ensure that watch time is only reported for media
  // that is actually audible to the user. It should be called whenever the
  // volume changes.
  //
  // Note: This does not catch all cases. E.g., headphones that are not being
  // listened too, or even OS level volume state.
  void OnVolumeChange(double volume);

  // These methods are used to ensure that watch time is only reported for media
  // that is actually visible to the user. They should be called when the media
  // is shown or hidden respectively. OnHidden() will start background watch
  // time reporting.
  void OnShown();
  void OnHidden();

  // Called when a playback ends in error.
  void OnError(PipelineStatus status);

  // Indicates a rebuffering event occurred during playback. When watch time is
  // finalized the total watch time for a given category will be divided by the
  // number of rebuffering events. Reset to zero after a finalize event.
  void OnUnderflow();

  // These methods are used to ensure that the watch time is reported relative
  // to whether the media is using native controls.
  void OnNativeControlsEnabled();
  void OnNativeControlsDisabled();

  // These methods are used to ensure that the watch time is reported relative
  // to the display type of the media.
  void OnDisplayTypeInline();
  void OnDisplayTypeFullscreen();
  void OnDisplayTypePictureInPicture();

  // Sets the audio and video decoder names for reporting. Similar to OnError(),
  // this value is always sent to the recorder regardless of whether we're
  // currently reporting watch time or not. Must only be set once.
  void SetAudioDecoderName(const std::string& name);
  void SetVideoDecoderName(const std::string& name);

  // Notifies the autoplay status of the playback. Must not be called multiple
  // times with different values.
  void SetAutoplayInitiated(bool autoplay_initiated);

  // Setup the reporting interval to be immediate to avoid spinning real time
  // within the unit test.
  void set_reporting_interval_for_testing() {
    reporting_interval_ = base::TimeDelta();
  }

  void set_is_on_battery_power_for_testing(bool on_battery_power) {
    is_on_battery_power_ = on_battery_power;
  }

  void OnPowerStateChangeForTesting(bool on_battery_power) {
    OnPowerStateChange(on_battery_power);
  }

 private:
  friend class WatchTimeReporterTest;

  // Internal constructor for marking background status.
  WatchTimeReporter(mojom::PlaybackPropertiesPtr properties,
                    bool is_background,
                    bool is_muted,
                    GetMediaTimeCB get_media_time_cb,
                    mojom::MediaMetricsProvider* provider,
                    scoped_refptr<base::SequencedTaskRunner> task_runner,
                    const base::TickClock* tick_clock);

  // base::PowerObserver implementation.
  //
  // We only observe power source changes. We don't need to observe suspend and
  // resume events because we report watch time in terms of elapsed media time
  // and not in terms of elapsed real time.
  void OnPowerStateChange(bool on_battery_power) override;

  bool ShouldReportWatchTime();
  void MaybeStartReportingTimer(base::TimeDelta start_timestamp);
  enum class FinalizeTime { IMMEDIATELY, ON_NEXT_UPDATE };
  void MaybeFinalizeWatchTime(FinalizeTime finalize_time);
  void UpdateWatchTime();
  void OnDisplayTypeChanged(blink::WebMediaPlayer::DisplayType display_type);

  // Initialized during construction.
  const mojom::PlaybackPropertiesPtr properties_;
  const bool is_background_;
  const bool is_muted_;
  const GetMediaTimeCB get_media_time_cb_;
  mojom::WatchTimeRecorderPtr recorder_;

  // The amount of time between each UpdateWatchTime(); this is the frequency by
  // which the watch times are updated. In the event of a process crash or kill
  // this is also the most amount of watch time that we might lose.
  base::TimeDelta reporting_interval_ = base::TimeDelta::FromSeconds(5);

  base::RepeatingTimer reporting_timer_;

  // Updated by the OnXXX() methods above.
  bool is_on_battery_power_ = false;
  bool is_playing_ = false;
  bool is_visible_ = true;
  bool has_native_controls_ = false;
  double volume_ = 1.0;
  int underflow_count_ = 0;
  std::vector<base::TimeDelta> pending_underflow_events_;

  blink::WebMediaPlayer::DisplayType display_type_ =
      blink::WebMediaPlayer::DisplayType::kInline;
  blink::WebMediaPlayer::DisplayType display_type_for_recording_ =
      blink::WebMediaPlayer::DisplayType::kInline;

  // The last media timestamp seen by UpdateWatchTime().
  base::TimeDelta last_media_timestamp_ = kNoTimestamp;
  base::TimeDelta last_media_power_timestamp_ = kNoTimestamp;
  base::TimeDelta last_media_controls_timestamp_ = kNoTimestamp;
  base::TimeDelta last_media_display_type_timestamp_ = kNoTimestamp;

  // The starting and ending timestamps used for reporting watch time.
  base::TimeDelta start_timestamp_;
  base::TimeDelta end_timestamp_ = kNoTimestamp;

  // Similar to the above but tracks watch time relative to whether or not
  // battery or AC power is being used.
  base::TimeDelta start_timestamp_for_power_;
  base::TimeDelta end_timestamp_for_power_ = kNoTimestamp;

  // Similar to the above but tracks watch time relative to whether or not
  // native controls are being used.
  base::TimeDelta start_timestamp_for_controls_;
  base::TimeDelta end_timestamp_for_controls_ = kNoTimestamp;

  // Similar to the above but tracks watch time relative to whether the display
  // type is inline, fullscreen or picture-in-picture.
  base::TimeDelta start_timestamp_for_display_type_;
  base::TimeDelta end_timestamp_for_display_type_ = kNoTimestamp;

  // Special case reporter for handling background video watch time. Configured
  // as an audio only WatchTimeReporter with |is_background_| set to true.
  std::unique_ptr<WatchTimeReporter> background_reporter_;

  // Similar to the above, but for muted audio+video watch time. Configured as
  // an audio+video WatchTimeReporter with |is_muted_| set to true.
  std::unique_ptr<WatchTimeReporter> muted_reporter_;

  DISALLOW_COPY_AND_ASSIGN(WatchTimeReporter);
};

}  // namespace media

#endif  // MEDIA_BLINK_WATCH_TIME_REPORTER_H_
