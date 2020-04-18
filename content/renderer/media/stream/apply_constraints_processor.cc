// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/apply_constraints_processor.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "base/task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/renderer/media/stream/media_stream_audio_source.h"
#include "content/renderer/media/stream/media_stream_constraints_util_audio.h"
#include "content/renderer/media/stream/media_stream_constraints_util_video_content.h"
#include "content/renderer/media/stream/media_stream_constraints_util_video_device.h"
#include "content/renderer/media/stream/media_stream_source.h"
#include "content/renderer/media/stream/media_stream_video_source.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_string.h"

namespace content {
namespace {

void RequestFailed(blink::WebApplyConstraintsRequest request,
                   const blink::WebString& constraint,
                   const blink::WebString& message) {
  request.RequestFailed(constraint, message);
}

void RequestSucceeded(blink::WebApplyConstraintsRequest request) {
  request.RequestSucceeded();
}

}  // namespace

ApplyConstraintsProcessor::ApplyConstraintsProcessor(
    MediaDevicesDispatcherCallback media_devices_dispatcher_cb,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : media_devices_dispatcher_cb_(std::move(media_devices_dispatcher_cb)),
      task_runner_(std::move(task_runner)),
      weak_factory_(this) {}

ApplyConstraintsProcessor::~ApplyConstraintsProcessor() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void ApplyConstraintsProcessor::ProcessRequest(
    const blink::WebApplyConstraintsRequest& request,
    base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!request_completed_cb_);
  DCHECK(current_request_.IsNull());
  DCHECK(!request.Track().IsNull());
  if (request.Track().Source().IsNull()) {
    CannotApplyConstraints(
        "Track has no source. ApplyConstraints not possible.");
    return;
  }
  request_completed_cb_ = std::move(callback);
  current_request_ = request;
  if (current_request_.Track().Source().GetType() ==
      blink::WebMediaStreamSource::kTypeVideo) {
    ProcessVideoRequest();
  } else {
    DCHECK_EQ(current_request_.Track().Source().GetType(),
              blink::WebMediaStreamSource::kTypeAudio);
    ProcessAudioRequest();
  }
}

void ApplyConstraintsProcessor::ProcessAudioRequest() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!current_request_.IsNull());
  DCHECK_EQ(current_request_.Track().Source().GetType(),
            blink::WebMediaStreamSource::kTypeAudio);
  DCHECK(request_completed_cb_);
  MediaStreamAudioSource* audio_source = GetCurrentAudioSource();
  if (!audio_source) {
    CannotApplyConstraints("The track is not connected to any source");
    return;
  }

  AudioCaptureSettings settings =
      SelectSettingsAudioCapture(audio_source, current_request_.Constraints());
  if (settings.HasValue()) {
    ApplyConstraintsSucceeded();
  } else {
    ApplyConstraintsFailed(settings.failed_constraint_name());
  }
}

void ApplyConstraintsProcessor::ProcessVideoRequest() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!current_request_.IsNull());
  DCHECK_EQ(current_request_.Track().Source().GetType(),
            blink::WebMediaStreamSource::kTypeVideo);
  DCHECK(request_completed_cb_);
  video_source_ = GetCurrentVideoSource();
  if (!video_source_) {
    CannotApplyConstraints("The track is not connected to any source");
    return;
  }

  const MediaStreamDevice& device_info = video_source_->device();
  if (device_info.type == MEDIA_DEVICE_VIDEO_CAPTURE) {
    ProcessVideoDeviceRequest();
  } else if (video_source_->GetCurrentFormat()) {
    // Non-device capture just requires adjusting track settings.
    FinalizeVideoRequest();
  } else {
    // It is impossible to enforce minimum constraints for sources that do not
    // provide the video format, so reject applyConstraints() in this case.
    CannotApplyConstraints("applyConstraints not supported for this track");
  }
}

void ApplyConstraintsProcessor::ProcessVideoDeviceRequest() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (AbortIfVideoRequestStateInvalid())
    return;

  // TODO(guidou): Support restarting the source even if there is more than
  // one track in the source. http://crbug.com/768205
  if (video_source_->NumTracks() > 1U) {
    FinalizeVideoRequest();
    return;
  }

  // It might be necessary to restart the video source. Before doing that,
  // check if the current format is the best format to satisfy the new
  // constraints. If this is the case, then the source does not need to be
  // restarted. To determine if the current format is the best, it is necessary
  // to know all the formats potentially supported by the source.
  GetMediaDevicesDispatcher()->GetAllVideoInputDeviceFormats(
      video_source_->device().id,
      base::BindOnce(&ApplyConstraintsProcessor::MaybeStopSourceForRestart,
                     weak_factory_.GetWeakPtr()));
}

void ApplyConstraintsProcessor::MaybeStopSourceForRestart(
    const media::VideoCaptureFormats& formats) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (AbortIfVideoRequestStateInvalid())
    return;

  VideoCaptureSettings settings = SelectVideoSettings(formats);
  if (!settings.HasValue()) {
    ApplyConstraintsFailed(settings.failed_constraint_name());
    return;
  }

  if (video_source_->GetCurrentFormat() == settings.Format()) {
    video_source_->ReconfigureTrack(GetCurrentVideoTrack(),
                                    settings.track_adapter_settings());
    ApplyConstraintsSucceeded();
  } else {
    video_source_->StopForRestart(
        base::BindOnce(&ApplyConstraintsProcessor::MaybeSourceStoppedForRestart,
                       weak_factory_.GetWeakPtr()));
  }
}

void ApplyConstraintsProcessor::MaybeSourceStoppedForRestart(
    MediaStreamVideoSource::RestartResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (AbortIfVideoRequestStateInvalid())
    return;

  if (result == MediaStreamVideoSource::RestartResult::IS_RUNNING) {
    FinalizeVideoRequest();
    return;
  }

  DCHECK_EQ(result, MediaStreamVideoSource::RestartResult::IS_STOPPED);
  GetMediaDevicesDispatcher()->GetAvailableVideoInputDeviceFormats(
      video_source_->device().id,
      base::BindOnce(&ApplyConstraintsProcessor::FindNewFormatAndRestart,
                     weak_factory_.GetWeakPtr()));
}

void ApplyConstraintsProcessor::FindNewFormatAndRestart(
    const media::VideoCaptureFormats& formats) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (AbortIfVideoRequestStateInvalid())
    return;

  VideoCaptureSettings settings = SelectVideoSettings(formats);
  DCHECK(video_source_->GetCurrentFormat());
  // |settings| should have a value. If it does not due to some unexpected
  // reason (perhaps a race with another renderer process), restart the source
  // with the old format.
  video_source_->Restart(
      settings.HasValue() ? settings.Format()
                          : *video_source_->GetCurrentFormat(),
      base::BindOnce(&ApplyConstraintsProcessor::MaybeSourceRestarted,
                     weak_factory_.GetWeakPtr()));
}

void ApplyConstraintsProcessor::MaybeSourceRestarted(
    MediaStreamVideoSource::RestartResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (AbortIfVideoRequestStateInvalid())
    return;

  if (result == MediaStreamVideoSource::RestartResult::IS_RUNNING) {
    FinalizeVideoRequest();
  } else {
    DCHECK_EQ(result, MediaStreamVideoSource::RestartResult::IS_STOPPED);
    CannotApplyConstraints("Source failed to restart");
    video_source_->StopSource();
  }
}

void ApplyConstraintsProcessor::FinalizeVideoRequest() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (AbortIfVideoRequestStateInvalid())
    return;

  DCHECK(video_source_->GetCurrentFormat());
  VideoCaptureSettings settings =
      SelectVideoSettings({*video_source_->GetCurrentFormat()});
  if (settings.HasValue()) {
    video_source_->ReconfigureTrack(GetCurrentVideoTrack(),
                                    settings.track_adapter_settings());
    ApplyConstraintsSucceeded();
  } else {
    ApplyConstraintsFailed(settings.failed_constraint_name());
  }
}

VideoCaptureSettings ApplyConstraintsProcessor::SelectVideoSettings(
    media::VideoCaptureFormats formats) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!current_request_.IsNull());
  DCHECK_EQ(current_request_.Track().Source().GetType(),
            blink::WebMediaStreamSource::kTypeVideo);
  DCHECK(request_completed_cb_);
  DCHECK_GT(formats.size(), 0U);

  blink::mojom::VideoInputDeviceCapabilitiesPtr device_capabilities =
      blink::mojom::VideoInputDeviceCapabilities::New();
  device_capabilities->device_id =
      current_request_.Track().Source().Id().Ascii();
  device_capabilities->group_id =
      current_request_.Track().Source().GroupId().Ascii();
  device_capabilities->facing_mode =
      GetCurrentVideoSource() ? GetCurrentVideoSource()->device().video_facing
                              : media::MEDIA_VIDEO_FACING_NONE;
  device_capabilities->formats = std::move(formats);

  DCHECK(video_source_->GetCurrentCaptureParams());
  VideoDeviceCaptureCapabilities video_capabilities;
  video_capabilities.power_line_capabilities.push_back(
      video_source_->GetCurrentCaptureParams()->power_line_frequency);
  video_capabilities.noise_reduction_capabilities.push_back(
      GetCurrentVideoTrack()->noise_reduction());
  video_capabilities.device_capabilities.push_back(
      std::move(device_capabilities));

  // Run SelectSettings using the track's current settings as the default
  // values. However, initialize |settings| with the default values as a
  // fallback in case GetSettings returns nothing and leaves |settings|
  // unmodified.
  blink::WebMediaStreamTrack::Settings settings;
  settings.width = MediaStreamVideoSource::kDefaultWidth;
  settings.height = MediaStreamVideoSource::kDefaultHeight;
  settings.frame_rate = MediaStreamVideoSource::kDefaultFrameRate;
  GetCurrentVideoTrack()->GetSettings(settings);

  return SelectSettingsVideoDeviceCapture(
      video_capabilities, current_request_.Constraints(), settings.width,
      settings.height, settings.frame_rate);
}

MediaStreamAudioSource* ApplyConstraintsProcessor::GetCurrentAudioSource() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!current_request_.Track().IsNull());
  return MediaStreamAudioSource::From(current_request_.Track().Source());
}

MediaStreamVideoTrack* ApplyConstraintsProcessor::GetCurrentVideoTrack() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  MediaStreamVideoTrack* track =
      MediaStreamVideoTrack::GetVideoTrack(current_request_.Track());
  DCHECK(track);
  return track;
}

MediaStreamVideoSource* ApplyConstraintsProcessor::GetCurrentVideoSource() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetCurrentVideoTrack()->source();
}

bool ApplyConstraintsProcessor::AbortIfVideoRequestStateInvalid() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!current_request_.IsNull());
  DCHECK_EQ(current_request_.Track().Source().GetType(),
            blink::WebMediaStreamSource::kTypeVideo);
  DCHECK(request_completed_cb_);
  if (GetCurrentVideoSource() != video_source_) {
    CannotApplyConstraints(
        "Track stopped or source changed. ApplyConstraints not possible.");
    return true;
  }
  return false;
}

void ApplyConstraintsProcessor::ApplyConstraintsSucceeded() {
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&ApplyConstraintsProcessor::CleanupRequest,
                     weak_factory_.GetWeakPtr(),
                     base::BindOnce(&RequestSucceeded, current_request_)));
}

void ApplyConstraintsProcessor::ApplyConstraintsFailed(
    const char* failed_constraint_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ApplyConstraintsProcessor::CleanupRequest,
          weak_factory_.GetWeakPtr(),
          base::BindOnce(
              &RequestFailed, current_request_,
              blink::WebString::FromASCII(failed_constraint_name),
              blink::WebString::FromASCII("Cannot satisfy constraints"))));
}

void ApplyConstraintsProcessor::CannotApplyConstraints(
    const blink::WebString& message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&ApplyConstraintsProcessor::CleanupRequest,
                                weak_factory_.GetWeakPtr(),
                                base::BindOnce(&RequestFailed, current_request_,
                                               blink::WebString(), message)));
}

void ApplyConstraintsProcessor::CleanupRequest(
    base::OnceClosure web_request_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!current_request_.IsNull());
  DCHECK(request_completed_cb_);
  base::ResetAndReturn(&request_completed_cb_).Run();
  std::move(web_request_callback).Run();
  current_request_.Reset();
  video_source_ = nullptr;
}

const blink::mojom::MediaDevicesDispatcherHostPtr&
ApplyConstraintsProcessor::GetMediaDevicesDispatcher() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return media_devices_dispatcher_cb_.Run();
}

}  // namespace content
