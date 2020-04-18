// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/service/cast_renderer.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "chromecast/base/task_runner_impl.h"
#include "chromecast/media/base/audio_device_ids.h"
#include "chromecast/media/base/video_mode_switcher.h"
#include "chromecast/media/base/video_resolution_policy.h"
#include "chromecast/media/cdm/cast_cdm_context.h"
#include "chromecast/media/cma/base/balanced_media_task_runner_factory.h"
#include "chromecast/media/cma/base/cma_logging.h"
#include "chromecast/media/cma/base/demuxer_stream_adapter.h"
#include "chromecast/media/cma/pipeline/media_pipeline_impl.h"
#include "chromecast/media/cma/pipeline/video_pipeline_client.h"
#include "chromecast/public/media/media_pipeline_backend.h"
#include "chromecast/public/media/media_pipeline_device_params.h"
#include "chromecast/public/volume_control.h"
#include "media/audio/audio_device_description.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_log.h"
#include "media/base/media_resource.h"
#include "media/base/renderer_client.h"

namespace chromecast {
namespace media {

namespace {
// Maximum difference between audio frame PTS and video frame PTS
// for frames read from the DemuxerStream.
const base::TimeDelta kMaxDeltaFetcher(base::TimeDelta::FromMilliseconds(2000));

void VideoModeSwitchCompletionCb(const ::media::PipelineStatusCB& init_cb,
                                 bool success) {
  if (!success) {
    LOG(ERROR) << "Video mode switch failed.";
    init_cb.Run(::media::PIPELINE_ERROR_INITIALIZATION_FAILED);
    return;
  }
  VLOG(1) << "Video mode switched successfully.";
  init_cb.Run(::media::PIPELINE_OK);
}
}  // namespace

CastRenderer::CastRenderer(
    CmaBackendFactory* backend_factory,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const std::string& audio_device_id,
    VideoModeSwitcher* video_mode_switcher,
    VideoResolutionPolicy* video_resolution_policy,
    MediaResourceTracker* media_resource_tracker)
    : backend_factory_(backend_factory),
      task_runner_(task_runner),
      audio_device_id_(audio_device_id.empty()
                           ? ::media::AudioDeviceDescription::kDefaultDeviceId
                           : audio_device_id),
      video_mode_switcher_(video_mode_switcher),
      video_resolution_policy_(video_resolution_policy),
      media_resource_tracker_(media_resource_tracker),
      client_(nullptr),
      cast_cdm_context_(nullptr),
      media_task_runner_factory_(
          new BalancedMediaTaskRunnerFactory(kMaxDeltaFetcher)),
      weak_factory_(this) {
  DCHECK(backend_factory_);
  CMALOG(kLogControl) << __FUNCTION__ << ": " << this;

  if (video_resolution_policy_)
    video_resolution_policy_->AddObserver(this);
}

CastRenderer::~CastRenderer() {
  CMALOG(kLogControl) << __FUNCTION__ << ": " << this;
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (video_resolution_policy_)
    video_resolution_policy_->RemoveObserver(this);
}

void CastRenderer::Initialize(::media::MediaResource* media_resource,
                              ::media::RendererClient* client,
                              const ::media::PipelineStatusCB& init_cb) {
  CMALOG(kLogControl) << __FUNCTION__ << ": " << this;
  DCHECK(task_runner_->BelongsToCurrentThread());

  // Create pipeline backend.
  media_resource_usage_.reset(
      new MediaResourceTracker::ScopedUsage(media_resource_tracker_));
  backend_task_runner_.reset(new TaskRunnerImpl());
  // TODO(erickung): crbug.com/443956. Need to provide right LoadType.
  LoadType load_type = kLoadTypeMediaSource;
  MediaPipelineDeviceParams::MediaSyncType sync_type =
      (load_type == kLoadTypeMediaStream)
          ? MediaPipelineDeviceParams::kModeIgnorePts
          : MediaPipelineDeviceParams::kModeSyncPts;

  AudioContentType content_type;
  if (audio_device_id_ == kAlarmAudioDeviceId) {
    content_type = AudioContentType::kAlarm;
  } else if (audio_device_id_ == kTtsAudioDeviceId ||
             audio_device_id_ ==
                 ::media::AudioDeviceDescription::kCommunicationsDeviceId) {
    content_type = AudioContentType::kCommunication;
  } else {
    content_type = AudioContentType::kMedia;
  }
  MediaPipelineDeviceParams params(sync_type, backend_task_runner_.get(),
                                   content_type, audio_device_id_);

  if (audio_device_id_ == kTtsAudioDeviceId ||
      audio_device_id_ ==
          ::media::AudioDeviceDescription::kCommunicationsDeviceId) {
    load_type = kLoadTypeCommunication;
  } else if (audio_device_id_ == kPlatformAudioDeviceId) {
    load_type = kLoadTypeMediaStream;
  }

  auto backend = backend_factory_->CreateBackend(params);

  // Create pipeline.
  MediaPipelineClient pipeline_client;
  pipeline_client.error_cb =
      base::Bind(&CastRenderer::OnError, weak_factory_.GetWeakPtr());
  pipeline_client.buffering_state_cb = base::Bind(
      &CastRenderer::OnBufferingStateChange, weak_factory_.GetWeakPtr());
  pipeline_.reset(new MediaPipelineImpl());
  pipeline_->SetClient(pipeline_client);
  pipeline_->Initialize(load_type, std::move(backend));

  // TODO(servolk): Implement support for multiple streams. For now use the
  // first enabled audio and video streams to preserve the existing behavior.
  ::media::DemuxerStream* audio_stream =
      media_resource->GetFirstStream(::media::DemuxerStream::AUDIO);
  ::media::DemuxerStream* video_stream =
      media_resource->GetFirstStream(::media::DemuxerStream::VIDEO);

  // Initialize audio.
  if (audio_stream) {
    AvPipelineClient audio_client;
    audio_client.wait_for_key_cb = base::Bind(
        &CastRenderer::OnWaitingForDecryptionKey, weak_factory_.GetWeakPtr());
    audio_client.eos_cb = base::Bind(&CastRenderer::OnEnded,
                                     weak_factory_.GetWeakPtr(), STREAM_AUDIO);
    audio_client.playback_error_cb =
        base::Bind(&CastRenderer::OnError, weak_factory_.GetWeakPtr());
    audio_client.statistics_cb = base::Bind(&CastRenderer::OnStatisticsUpdate,
                                            weak_factory_.GetWeakPtr());
    std::unique_ptr<CodedFrameProvider> frame_provider(new DemuxerStreamAdapter(
        task_runner_, media_task_runner_factory_, audio_stream));

    ::media::PipelineStatus status =
        pipeline_->InitializeAudio(audio_stream->audio_decoder_config(),
                                   audio_client, std::move(frame_provider));
    if (status != ::media::PIPELINE_OK) {
      init_cb.Run(status);
      return;
    }
    audio_stream->EnableBitstreamConverter();
  }

  // Initialize video.
  if (video_stream) {
    VideoPipelineClient video_client;
    video_client.av_pipeline_client.wait_for_key_cb = base::Bind(
        &CastRenderer::OnWaitingForDecryptionKey, weak_factory_.GetWeakPtr());
    video_client.av_pipeline_client.eos_cb = base::Bind(
        &CastRenderer::OnEnded, weak_factory_.GetWeakPtr(), STREAM_VIDEO);
    video_client.av_pipeline_client.playback_error_cb =
        base::Bind(&CastRenderer::OnError, weak_factory_.GetWeakPtr());
    video_client.av_pipeline_client.statistics_cb = base::Bind(
        &CastRenderer::OnStatisticsUpdate, weak_factory_.GetWeakPtr());
    video_client.natural_size_changed_cb = base::Bind(
        &CastRenderer::OnVideoNaturalSizeChange, weak_factory_.GetWeakPtr());
    // TODO(alokp): Change MediaPipelineImpl API to accept a single config
    // after CmaRenderer is deprecated.
    std::vector<::media::VideoDecoderConfig> video_configs;
    video_configs.push_back(video_stream->video_decoder_config());
    std::unique_ptr<CodedFrameProvider> frame_provider(new DemuxerStreamAdapter(
        task_runner_, media_task_runner_factory_, video_stream));

    ::media::PipelineStatus status = pipeline_->InitializeVideo(
        video_configs, video_client, std::move(frame_provider));
    if (status != ::media::PIPELINE_OK) {
      init_cb.Run(status);
      return;
    }
    video_stream->EnableBitstreamConverter();
  }

  if (cast_cdm_context_) {
    pipeline_->SetCdm(cast_cdm_context_);
    cast_cdm_context_ = nullptr;
  }

  client_ = client;

  if (video_stream && video_mode_switcher_) {
    std::vector<::media::VideoDecoderConfig> video_configs;
    video_configs.push_back(video_stream->video_decoder_config());
    auto mode_switch_completion_cb =
        base::Bind(&CastRenderer::OnVideoInitializationFinished,
                   weak_factory_.GetWeakPtr(), init_cb);
    video_mode_switcher_->SwitchMode(
        video_configs, base::BindOnce(&VideoModeSwitchCompletionCb,
                                      mode_switch_completion_cb));
  } else if (video_stream) {
    // No mode switch needed.
    OnVideoInitializationFinished(init_cb, ::media::PIPELINE_OK);
  } else {
    init_cb.Run(::media::PIPELINE_OK);
  }
}

void CastRenderer::OnVideoInitializationFinished(
    const ::media::PipelineStatusCB& init_cb,
    ::media::PipelineStatus status) {
  init_cb.Run(status);
  if (status == ::media::PIPELINE_OK) {
    // Force compositor to treat video as opaque (needed for overlay codepath).
    OnVideoOpacityChange(true);
  }
}

void CastRenderer::SetCdm(::media::CdmContext* cdm_context,
                          const ::media::CdmAttachedCB& cdm_attached_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(cdm_context);

  auto* cast_cdm_context = static_cast<CastCdmContext*>(cdm_context);

  if (!pipeline_) {
    // If the pipeline has not yet been created in Initialize(), cache
    // |cast_cdm_context| and pass it in when Initialize() is called.
    cast_cdm_context_ = cast_cdm_context;
  } else {
    pipeline_->SetCdm(cast_cdm_context);
  }

  cdm_attached_cb.Run(true);
}

void CastRenderer::Flush(const base::Closure& flush_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  pipeline_->Flush(flush_cb);
}

void CastRenderer::StartPlayingFrom(base::TimeDelta time) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  eos_[STREAM_AUDIO] = !pipeline_->HasAudio();
  eos_[STREAM_VIDEO] = !pipeline_->HasVideo();
  pipeline_->StartPlayingFrom(time);
}

void CastRenderer::SetPlaybackRate(double playback_rate) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  pipeline_->SetPlaybackRate(playback_rate);
}

void CastRenderer::SetVolume(float volume) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  pipeline_->SetVolume(volume);
}

base::TimeDelta CastRenderer::GetMediaTime() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return pipeline_->GetMediaTime();
}

void CastRenderer::OnVideoResolutionPolicyChanged() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (!video_resolution_policy_)
    return;

  if (video_resolution_policy_->ShouldBlock(video_res_))
    OnError(::media::PIPELINE_ERROR_DECODE);
}

void CastRenderer::OnError(::media::PipelineStatus status) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  client_->OnError(status);
}

void CastRenderer::OnEnded(Stream stream) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!eos_[stream]);
  eos_[stream] = true;
  CMALOG(kLogControl) << __FUNCTION__ << ": eos_audio=" << eos_[STREAM_AUDIO]
                      << " eos_video=" << eos_[STREAM_VIDEO];
  if (eos_[STREAM_AUDIO] && eos_[STREAM_VIDEO])
    client_->OnEnded();
}

void CastRenderer::OnStatisticsUpdate(
    const ::media::PipelineStatistics& stats) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  client_->OnStatisticsUpdate(stats);
}

void CastRenderer::OnBufferingStateChange(::media::BufferingState state) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  // TODO(alokp): WebMediaPlayerImpl currently only handles HAVE_ENOUGH.
  // See WebMediaPlayerImpl::OnPipelineBufferingStateChanged,
  // http://crbug.com/144683.
  if (state == ::media::BUFFERING_HAVE_ENOUGH)
    client_->OnBufferingStateChange(state);
}

void CastRenderer::OnWaitingForDecryptionKey() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  client_->OnWaitingForDecryptionKey();
}

void CastRenderer::OnVideoNaturalSizeChange(const gfx::Size& size) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  client_->OnVideoNaturalSizeChange(size);

  video_res_ = size;
  OnVideoResolutionPolicyChanged();
}

void CastRenderer::OnVideoOpacityChange(bool opaque) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(opaque);
  client_->OnVideoOpacityChange(opaque);
}

}  // namespace media
}  // namespace chromecast
