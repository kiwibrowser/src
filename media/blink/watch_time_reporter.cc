// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/watch_time_reporter.h"

#include "base/power_monitor/power_monitor.h"
#include "media/base/watch_time_keys.h"

namespace media {

// The minimum width and height of videos to report watch time metrics for.
constexpr gfx::Size kMinimumVideoSize = gfx::Size(200, 140);

static bool IsOnBatteryPower() {
  if (base::PowerMonitor* pm = base::PowerMonitor::Get())
    return pm->IsOnBatteryPower();
  return false;
}

WatchTimeReporter::WatchTimeReporter(
    mojom::PlaybackPropertiesPtr properties,
    GetMediaTimeCB get_media_time_cb,
    mojom::MediaMetricsProvider* provider,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const base::TickClock* tick_clock)
    : WatchTimeReporter(std::move(properties),
                        false /* is_background */,
                        false /* is_muted */,
                        std::move(get_media_time_cb),
                        provider,
                        task_runner,
                        tick_clock) {}

WatchTimeReporter::WatchTimeReporter(
    mojom::PlaybackPropertiesPtr properties,
    bool is_background,
    bool is_muted,
    GetMediaTimeCB get_media_time_cb,
    mojom::MediaMetricsProvider* provider,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const base::TickClock* tick_clock)
    : properties_(std::move(properties)),
      is_background_(is_background),
      is_muted_(is_muted),
      get_media_time_cb_(std::move(get_media_time_cb)),
      reporting_timer_(tick_clock) {
  DCHECK(!get_media_time_cb_.is_null());
  DCHECK(properties_->has_audio || properties_->has_video);
  DCHECK_EQ(is_background, properties_->is_background);

  // The background reporter receives play/pause events instead of visibility
  // changes, so it must always be visible to function correctly.
  if (is_background_)
    DCHECK(is_visible_);

  // The muted reporter receives play/pause events instead of volume changes, so
  // its volume must always be audible to function correctly.
  if (is_muted_)
    DCHECK_EQ(volume_, 1.0);

  if (base::PowerMonitor* pm = base::PowerMonitor::Get())
    pm->AddObserver(this);

  provider->AcquireWatchTimeRecorder(properties_->Clone(),
                                     mojo::MakeRequest(&recorder_));

  reporting_timer_.SetTaskRunner(task_runner);

  // If this is a sub-reporter or we shouldn't report watch time, we're done. We
  // don't support muted+background reporting currently.
  if (is_background_ || is_muted_ || !ShouldReportWatchTime())
    return;

  // Background watch time is reported by creating an background only watch time
  // reporter which receives play when hidden and pause when shown. This avoids
  // unnecessary complexity inside the UpdateWatchTime() for handling this case.
  auto prop_copy = properties_.Clone();
  prop_copy->is_background = true;
  background_reporter_.reset(new WatchTimeReporter(
      std::move(prop_copy), true /* is_background */, false /* is_muted */,
      get_media_time_cb_, provider, task_runner, tick_clock));

  // Muted watch time is only reported for audio+video playback.
  if (!properties_->has_video || !properties_->has_audio)
    return;

  // Similar to the above, muted watch time is reported by creating a muted only
  // watch time reporter which receives play when muted and pause when audible.
  prop_copy = properties_.Clone();
  prop_copy->is_muted = true;
  muted_reporter_.reset(new WatchTimeReporter(
      std::move(prop_copy), false /* is_background */, true /* is_muted */,
      get_media_time_cb_, provider, task_runner, tick_clock));
}

WatchTimeReporter::~WatchTimeReporter() {
  background_reporter_.reset();
  muted_reporter_.reset();

  // This is our last chance, so finalize now if there's anything remaining.
  MaybeFinalizeWatchTime(FinalizeTime::IMMEDIATELY);
  if (base::PowerMonitor* pm = base::PowerMonitor::Get())
    pm->RemoveObserver(this);
}

void WatchTimeReporter::OnPlaying() {
  if (background_reporter_ && !is_visible_)
    background_reporter_->OnPlaying();
  if (muted_reporter_ && !volume_)
    muted_reporter_->OnPlaying();

  is_playing_ = true;
  MaybeStartReportingTimer(get_media_time_cb_.Run());
}

void WatchTimeReporter::OnPaused() {
  if (background_reporter_)
    background_reporter_->OnPaused();
  if (muted_reporter_)
    muted_reporter_->OnPaused();

  is_playing_ = false;
  MaybeFinalizeWatchTime(FinalizeTime::ON_NEXT_UPDATE);
}

void WatchTimeReporter::OnSeeking() {
  if (background_reporter_)
    background_reporter_->OnSeeking();
  if (muted_reporter_)
    muted_reporter_->OnSeeking();

  // Seek is a special case that does not have hysteresis, when this is called
  // the seek is imminent, so finalize the previous playback immediately.
  MaybeFinalizeWatchTime(FinalizeTime::IMMEDIATELY);
}

void WatchTimeReporter::OnVolumeChange(double volume) {
  if (background_reporter_)
    background_reporter_->OnVolumeChange(volume);

  // The muted reporter should never receive volume changes.
  DCHECK(!is_muted_);

  const double old_volume = volume_;
  volume_ = volume;

  // We're only interesting in transitions in and out of the muted state.
  if (!old_volume && volume) {
    if (muted_reporter_)
      muted_reporter_->OnPaused();
    MaybeStartReportingTimer(get_media_time_cb_.Run());
  } else if (old_volume && !volume_) {
    if (muted_reporter_ && is_playing_)
      muted_reporter_->OnPlaying();
    MaybeFinalizeWatchTime(FinalizeTime::ON_NEXT_UPDATE);
  }
}

void WatchTimeReporter::OnShown() {
  // The background reporter should never receive visibility changes.
  DCHECK(!is_background_);

  if (background_reporter_)
    background_reporter_->OnPaused();
  if (muted_reporter_)
    muted_reporter_->OnShown();

  is_visible_ = true;
  MaybeStartReportingTimer(get_media_time_cb_.Run());
}

void WatchTimeReporter::OnHidden() {
  // The background reporter should never receive visibility changes.
  DCHECK(!is_background_);

  if (background_reporter_ && is_playing_)
    background_reporter_->OnPlaying();
  if (muted_reporter_)
    muted_reporter_->OnHidden();

  is_visible_ = false;
  MaybeFinalizeWatchTime(FinalizeTime::ON_NEXT_UPDATE);
}

void WatchTimeReporter::OnError(PipelineStatus status) {
  // Since playback should have stopped by this point, go ahead and send the
  // error directly instead of on the next timer tick. It won't be recorded
  // until finalization anyways.
  recorder_->OnError(status);
  if (background_reporter_)
    background_reporter_->OnError(status);
  if (muted_reporter_)
    muted_reporter_->OnError(status);
}

void WatchTimeReporter::OnUnderflow() {
  if (background_reporter_)
    background_reporter_->OnUnderflow();
  if (muted_reporter_)
    muted_reporter_->OnUnderflow();

  if (!reporting_timer_.IsRunning())
    return;

  // In the event of a pending finalize, we don't want to count underflow events
  // that occurred after the finalize time. Yet if the finalize is canceled we
  // want to ensure they are all recorded.
  pending_underflow_events_.push_back(get_media_time_cb_.Run());
}

void WatchTimeReporter::OnNativeControlsEnabled() {
  if (muted_reporter_)
    muted_reporter_->OnNativeControlsEnabled();

  if (!reporting_timer_.IsRunning()) {
    has_native_controls_ = true;
    return;
  }

  if (end_timestamp_for_controls_ != kNoTimestamp) {
    end_timestamp_for_controls_ = kNoTimestamp;
    return;
  }

  end_timestamp_for_controls_ = get_media_time_cb_.Run();
  reporting_timer_.Start(FROM_HERE, reporting_interval_, this,
                         &WatchTimeReporter::UpdateWatchTime);
}

void WatchTimeReporter::OnNativeControlsDisabled() {
  if (muted_reporter_)
    muted_reporter_->OnNativeControlsDisabled();

  if (!reporting_timer_.IsRunning()) {
    has_native_controls_ = false;
    return;
  }

  if (end_timestamp_for_controls_ != kNoTimestamp) {
    end_timestamp_for_controls_ = kNoTimestamp;
    return;
  }

  end_timestamp_for_controls_ = get_media_time_cb_.Run();
  reporting_timer_.Start(FROM_HERE, reporting_interval_, this,
                         &WatchTimeReporter::UpdateWatchTime);
}

void WatchTimeReporter::OnDisplayTypeInline() {
  OnDisplayTypeChanged(blink::WebMediaPlayer::DisplayType::kInline);
}

void WatchTimeReporter::OnDisplayTypeFullscreen() {
  OnDisplayTypeChanged(blink::WebMediaPlayer::DisplayType::kFullscreen);
}

void WatchTimeReporter::OnDisplayTypePictureInPicture() {
  OnDisplayTypeChanged(blink::WebMediaPlayer::DisplayType::kPictureInPicture);
}

void WatchTimeReporter::SetAudioDecoderName(const std::string& name) {
  DCHECK(properties_->has_audio);
  recorder_->SetAudioDecoderName(name);
  if (background_reporter_)
    background_reporter_->SetAudioDecoderName(name);
  if (muted_reporter_)
    muted_reporter_->SetAudioDecoderName(name);
}

void WatchTimeReporter::SetVideoDecoderName(const std::string& name) {
  DCHECK(properties_->has_video);
  recorder_->SetVideoDecoderName(name);
  if (background_reporter_)
    background_reporter_->SetVideoDecoderName(name);
  if (muted_reporter_)
    muted_reporter_->SetVideoDecoderName(name);
}

void WatchTimeReporter::SetAutoplayInitiated(bool autoplay_initiated) {
  recorder_->SetAutoplayInitiated(autoplay_initiated);
  if (background_reporter_)
    background_reporter_->SetAutoplayInitiated(autoplay_initiated);
  if (muted_reporter_)
    muted_reporter_->SetAutoplayInitiated(autoplay_initiated);
}

void WatchTimeReporter::OnPowerStateChange(bool on_battery_power) {
  if (!reporting_timer_.IsRunning())
    return;

  // Defer changing |is_on_battery_power_| until the next watch time report to
  // avoid momentary power changes from affecting the results.
  if (is_on_battery_power_ != on_battery_power) {
    end_timestamp_for_power_ = get_media_time_cb_.Run();

    // Restart the reporting timer so the full hysteresis is afforded.
    reporting_timer_.Start(FROM_HERE, reporting_interval_, this,
                           &WatchTimeReporter::UpdateWatchTime);
    return;
  }

  end_timestamp_for_power_ = kNoTimestamp;
}

bool WatchTimeReporter::ShouldReportWatchTime() {
  // Report listen time or watch time for videos of sufficient size.
  return properties_->has_video
             ? (properties_->natural_size.height() >=
                    kMinimumVideoSize.height() &&
                properties_->natural_size.width() >= kMinimumVideoSize.width())
             : properties_->has_audio;
}

void WatchTimeReporter::MaybeStartReportingTimer(
    base::TimeDelta start_timestamp) {
  DCHECK_NE(start_timestamp, kInfiniteDuration);
  DCHECK_GE(start_timestamp, base::TimeDelta());

  // Don't start the timer if any of our state indicates we shouldn't; this
  // check is important since the various event handlers do not have to care
  // about the state of other events.
  //
  // TODO(dalecurtis): We should only consider |volume_| when there is actually
  // an audio track; requires updating lots of tests to fix.
  if (!ShouldReportWatchTime() || !is_playing_ || !volume_ || !is_visible_) {
    // If we reach this point the timer should already have been stopped or
    // there is a pending finalize in flight.
    DCHECK(!reporting_timer_.IsRunning() || end_timestamp_ != kNoTimestamp);
    return;
  }

  // If we haven't finalized the last watch time metrics yet, count this
  // playback as a continuation of the previous metrics.
  if (end_timestamp_ != kNoTimestamp) {
    DCHECK(reporting_timer_.IsRunning());
    end_timestamp_ = kNoTimestamp;
    return;
  }

  // Don't restart the timer if it's already running.
  if (reporting_timer_.IsRunning())
    return;

  underflow_count_ = 0;
  pending_underflow_events_.clear();
  last_media_timestamp_ = last_media_power_timestamp_ =
      last_media_controls_timestamp_ = end_timestamp_for_power_ =
          last_media_display_type_timestamp_ = end_timestamp_for_display_type_ =
              kNoTimestamp;
  is_on_battery_power_ = IsOnBatteryPower();
  display_type_for_recording_ = display_type_;
  start_timestamp_ = start_timestamp_for_power_ =
      start_timestamp_for_controls_ = start_timestamp_for_display_type_ =
          start_timestamp;
  reporting_timer_.Start(FROM_HERE, reporting_interval_, this,
                         &WatchTimeReporter::UpdateWatchTime);
}

void WatchTimeReporter::MaybeFinalizeWatchTime(FinalizeTime finalize_time) {
  // Don't finalize if the timer is already stopped.
  if (!reporting_timer_.IsRunning())
    return;

  // Don't trample an existing finalize; the first takes precedence.
  if (end_timestamp_ == kNoTimestamp) {
    end_timestamp_ = get_media_time_cb_.Run();
    DCHECK_NE(end_timestamp_, kInfiniteDuration);
    DCHECK_GE(end_timestamp_, base::TimeDelta());
  }

  if (finalize_time == FinalizeTime::IMMEDIATELY) {
    UpdateWatchTime();
    return;
  }

  // Always restart the timer when finalizing, so that we allow for the full
  // length of |kReportingInterval| to elapse for hysteresis purposes.
  DCHECK_EQ(finalize_time, FinalizeTime::ON_NEXT_UPDATE);
  reporting_timer_.Start(FROM_HERE, reporting_interval_, this,
                         &WatchTimeReporter::UpdateWatchTime);
}

void WatchTimeReporter::UpdateWatchTime() {
  DCHECK(ShouldReportWatchTime());

  const bool is_finalizing = end_timestamp_ != kNoTimestamp;
  const bool is_power_change_pending = end_timestamp_for_power_ != kNoTimestamp;
  const bool is_controls_change_pending =
      end_timestamp_for_controls_ != kNoTimestamp;
  const bool is_display_type_change_pending =
      end_timestamp_for_display_type_ != kNoTimestamp;

  // If we're finalizing the log, use the media time value at the time of
  // finalization.
  const base::TimeDelta current_timestamp =
      is_finalizing ? end_timestamp_ : get_media_time_cb_.Run();
  DCHECK_NE(current_timestamp, kInfiniteDuration);
  DCHECK_GE(current_timestamp, start_timestamp_);

  const base::TimeDelta elapsed = current_timestamp - start_timestamp_;

#define RECORD_WATCH_TIME(key, value)                                     \
  do {                                                                    \
    recorder_->RecordWatchTime(                                           \
        (properties_->has_video && properties_->has_audio)                \
            ? (is_background_                                             \
                   ? WatchTimeKey::kAudioVideoBackground##key             \
                   : (is_muted_ ? WatchTimeKey::kAudioVideoMuted##key     \
                                : WatchTimeKey::kAudioVideo##key))        \
            : properties_->has_video                                      \
                  ? (is_background_ ? WatchTimeKey::kVideoBackground##key \
                                    : WatchTimeKey::kVideo##key)          \
                  : (is_background_ ? WatchTimeKey::kAudioBackground##key \
                                    : WatchTimeKey::kAudio##key),         \
        value);                                                           \
  } while (0)

  // Only report watch time after some minimum amount has elapsed. Don't update
  // watch time if media time hasn't changed since the last run; this may occur
  // if a seek is taking some time to complete or the playback is stalled for
  // some reason.
  if (last_media_timestamp_ != current_timestamp) {
    last_media_timestamp_ = current_timestamp;

    if (elapsed > base::TimeDelta()) {
      RECORD_WATCH_TIME(All, elapsed);
      if (properties_->is_mse)
        RECORD_WATCH_TIME(Mse, elapsed);
      else
        RECORD_WATCH_TIME(Src, elapsed);

      if (properties_->is_eme)
        RECORD_WATCH_TIME(Eme, elapsed);

      if (properties_->is_embedded_media_experience)
        RECORD_WATCH_TIME(EmbeddedExperience, elapsed);
    }
  }

  if (last_media_power_timestamp_ != current_timestamp) {
    // We need a separate |last_media_power_timestamp_| since we don't always
    // base the last watch time calculation on the current timestamp.
    last_media_power_timestamp_ =
        is_power_change_pending ? end_timestamp_for_power_ : current_timestamp;

    // Record watch time using the last known value for |is_on_battery_power_|;
    // if there's a |pending_power_change_| use that to accurately finalize the
    // last bits of time in the previous bucket.
    DCHECK_GE(last_media_power_timestamp_, start_timestamp_for_power_);
    const base::TimeDelta elapsed_power =
        last_media_power_timestamp_ - start_timestamp_for_power_;

    // Again, only update watch time if any time has elapsed; we need to recheck
    // the elapsed time here since the power source can change anytime.
    if (elapsed_power > base::TimeDelta()) {
      if (is_on_battery_power_)
        RECORD_WATCH_TIME(Battery, elapsed_power);
      else
        RECORD_WATCH_TIME(Ac, elapsed_power);
    }
  }

// Similar to RECORD_WATCH_TIME but ignores background watch time.
#define RECORD_FOREGROUND_WATCH_TIME(key, value)                  \
  do {                                                            \
    DCHECK(!is_background_);                                      \
    recorder_->RecordWatchTime(                                   \
        (properties_->has_video && properties_->has_audio)        \
            ? (is_muted_ ? WatchTimeKey::kAudioVideoMuted##key    \
                         : WatchTimeKey::kAudioVideo##key)        \
            : properties_->has_audio ? WatchTimeKey::kAudio##key  \
                                     : WatchTimeKey::kVideo##key, \
        value);                                                   \
  } while (0)

  // Similar to the block above for controls.
  if (!is_background_ && last_media_controls_timestamp_ != current_timestamp) {
    last_media_controls_timestamp_ = is_controls_change_pending
                                         ? end_timestamp_for_controls_
                                         : current_timestamp;

    DCHECK_GE(last_media_controls_timestamp_, start_timestamp_for_controls_);
    const base::TimeDelta elapsed_controls =
        last_media_controls_timestamp_ - start_timestamp_for_controls_;

    if (elapsed_controls > base::TimeDelta()) {
      if (has_native_controls_)
        RECORD_FOREGROUND_WATCH_TIME(NativeControlsOn, elapsed_controls);
      else
        RECORD_FOREGROUND_WATCH_TIME(NativeControlsOff, elapsed_controls);
    }
  }

// Similar to RECORD_WATCH_TIME but ignores background and audio watch time.
#define RECORD_DISPLAY_WATCH_TIME(key, value)                  \
  do {                                                         \
    DCHECK(properties_->has_video);                            \
    DCHECK(!is_background_);                                   \
    recorder_->RecordWatchTime(                                \
        properties_->has_audio                                 \
            ? (is_muted_ ? WatchTimeKey::kAudioVideoMuted##key \
                         : WatchTimeKey::kAudioVideo##key)     \
            : WatchTimeKey::kVideo##key,                       \
        value);                                                \
  } while (0)

  // Similar to the block above for display type.
  if (!is_background_ && properties_->has_video &&
      last_media_display_type_timestamp_ != current_timestamp) {
    last_media_display_type_timestamp_ = is_display_type_change_pending
                                             ? end_timestamp_for_display_type_
                                             : current_timestamp;

    DCHECK_GE(last_media_display_type_timestamp_,
              start_timestamp_for_display_type_);
    const base::TimeDelta elapsed_display_type =
        last_media_display_type_timestamp_ - start_timestamp_for_display_type_;

    if (elapsed_display_type > base::TimeDelta()) {
      switch (display_type_for_recording_) {
        case blink::WebMediaPlayer::DisplayType::kInline:
          RECORD_DISPLAY_WATCH_TIME(DisplayInline, elapsed_display_type);
          break;
        case blink::WebMediaPlayer::DisplayType::kFullscreen:
          RECORD_DISPLAY_WATCH_TIME(DisplayFullscreen, elapsed_display_type);
          break;
        case blink::WebMediaPlayer::DisplayType::kPictureInPicture:
          RECORD_DISPLAY_WATCH_TIME(DisplayPictureInPicture,
                                    elapsed_display_type);
          break;
      }
    }
  }

#undef RECORD_WATCH_TIME
#undef RECORD_FOREGROUND_WATCH_TIME
#undef RECORD_DISPLAY_WATCH_TIME

  // Pass along any underflow events which have occurred since the last report.
  if (!pending_underflow_events_.empty()) {
    if (!is_finalizing) {
      // The maximum value here per period is ~5 events, so int cast is okay.
      underflow_count_ += static_cast<int>(pending_underflow_events_.size());
    } else {
      // Only count underflow events prior to finalize.
      for (auto& ts : pending_underflow_events_) {
        if (ts <= end_timestamp_)
          underflow_count_++;
      }
    }

    recorder_->UpdateUnderflowCount(underflow_count_);
    pending_underflow_events_.clear();
  }

  // Always send finalize, even if we don't currently have any data, it's
  // harmless to send since nothing will be logged if we've already finalized.
  if (is_finalizing) {
    recorder_->FinalizeWatchTime({});
  } else {
    std::vector<WatchTimeKey> keys_to_finalize;
    if (is_power_change_pending) {
      keys_to_finalize.insert(
          keys_to_finalize.end(),
          {WatchTimeKey::kAudioBattery, WatchTimeKey::kAudioAc,
           WatchTimeKey::kAudioBackgroundBattery,
           WatchTimeKey::kAudioBackgroundAc, WatchTimeKey::kAudioVideoBattery,
           WatchTimeKey::kAudioVideoAc,
           WatchTimeKey::kAudioVideoBackgroundBattery,
           WatchTimeKey::kAudioVideoBackgroundAc,
           WatchTimeKey::kAudioVideoMutedBattery,
           WatchTimeKey::kAudioVideoMutedAc, WatchTimeKey::kVideoBattery,
           WatchTimeKey::kVideoAc, WatchTimeKey::kVideoBackgroundAc,
           WatchTimeKey::kVideoBackgroundBattery});
    }

    if (is_controls_change_pending) {
      keys_to_finalize.insert(keys_to_finalize.end(),
                              {WatchTimeKey::kAudioNativeControlsOn,
                               WatchTimeKey::kAudioNativeControlsOff,
                               WatchTimeKey::kAudioVideoNativeControlsOn,
                               WatchTimeKey::kAudioVideoNativeControlsOff,
                               WatchTimeKey::kAudioVideoMutedNativeControlsOn,
                               WatchTimeKey::kAudioVideoMutedNativeControlsOff,
                               WatchTimeKey::kVideoNativeControlsOn,
                               WatchTimeKey::kVideoNativeControlsOff});
    }

    if (is_display_type_change_pending) {
      keys_to_finalize.insert(
          keys_to_finalize.end(),
          {WatchTimeKey::kAudioVideoDisplayFullscreen,
           WatchTimeKey::kAudioVideoDisplayInline,
           WatchTimeKey::kAudioVideoDisplayPictureInPicture,
           WatchTimeKey::kAudioVideoMutedDisplayFullscreen,
           WatchTimeKey::kAudioVideoMutedDisplayInline,
           WatchTimeKey::kAudioVideoMutedDisplayPictureInPicture,
           WatchTimeKey::kVideoDisplayFullscreen,
           WatchTimeKey::kVideoDisplayInline,
           WatchTimeKey::kVideoDisplayPictureInPicture});
    }

    if (!keys_to_finalize.empty())
      recorder_->FinalizeWatchTime(keys_to_finalize);
  }

  if (is_power_change_pending) {
    // Invert battery power status here instead of using the value returned by
    // the PowerObserver since there may be a pending OnPowerStateChange().
    is_on_battery_power_ = !is_on_battery_power_;

    start_timestamp_for_power_ = end_timestamp_for_power_;
    end_timestamp_for_power_ = kNoTimestamp;
  }

  if (is_controls_change_pending) {
    has_native_controls_ = !has_native_controls_;

    start_timestamp_for_controls_ = end_timestamp_for_controls_;
    end_timestamp_for_controls_ = kNoTimestamp;
  }

  if (is_display_type_change_pending) {
    display_type_for_recording_ = display_type_;

    start_timestamp_for_display_type_ = end_timestamp_for_display_type_;
    end_timestamp_for_display_type_ = kNoTimestamp;
  }

  // Stop the timer if this is supposed to be our last tick.
  if (is_finalizing) {
    end_timestamp_ = kNoTimestamp;
    underflow_count_ = 0;
    reporting_timer_.Stop();
  }
}

void WatchTimeReporter::OnDisplayTypeChanged(
    blink::WebMediaPlayer::DisplayType display_type) {
  if (muted_reporter_)
    muted_reporter_->OnDisplayTypeChanged(display_type);

  display_type_ = display_type;

  if (!reporting_timer_.IsRunning())
    return;

  if (display_type_for_recording_ == display_type_) {
    end_timestamp_for_display_type_ = kNoTimestamp;
    return;
  }

  end_timestamp_for_display_type_ = get_media_time_cb_.Run();
  reporting_timer_.Start(FROM_HERE, reporting_interval_, this,
                         &WatchTimeReporter::UpdateWatchTime);
}

}  // namespace media
