// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/android/media_player_renderer_client.h"

#include "base/callback_helpers.h"

namespace content {

MediaPlayerRendererClient::MediaPlayerRendererClient(
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
    media::MojoRenderer* mojo_renderer,
    media::ScopedStreamTextureWrapper stream_texture_wrapper,
    media::VideoRendererSink* sink)
    : mojo_renderer_(mojo_renderer),
      stream_texture_wrapper_(std::move(stream_texture_wrapper)),
      client_(nullptr),
      sink_(sink),
      media_task_runner_(std::move(media_task_runner)),
      compositor_task_runner_(std::move(compositor_task_runner)),
      weak_factory_(this) {}

MediaPlayerRendererClient::~MediaPlayerRendererClient() {
  // Clearing the STW's callback into |this| must happen first. Otherwise, the
  // underlying StreamTextureProxy can callback into OnFrameAvailable() on the
  // |compositor_task_runner_|, while we are destroying |this|.
  // See https://crbug.com/688466.
  stream_texture_wrapper_->ClearReceivedFrameCBOnAnyThread();
}

void MediaPlayerRendererClient::Initialize(
    media::MediaResource* media_resource,
    media::RendererClient* client,
    const media::PipelineStatusCB& init_cb) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(!init_cb_);

  client_ = client;
  init_cb_ = init_cb;

  // Initialize the StreamTexture using a 1x1 texture because we do not have
  // any size information from the MediaPlayer yet.
  // The size will be automatically updated in OnVideoNaturalSizeChange() once
  // we parse the media's metadata.
  // Unretained is safe here because |stream_texture_wrapper_| resets the
  // Closure it has before destroying itself on |compositor_task_runner_|,
  // and |this| is garanteed to live until the Closure has been reset.
  stream_texture_wrapper_->Initialize(
      base::Bind(&MediaPlayerRendererClient::OnFrameAvailable,
                 base::Unretained(this)),
      gfx::Size(1, 1), compositor_task_runner_,
      base::Bind(&MediaPlayerRendererClient::OnStreamTextureWrapperInitialized,
                 weak_factory_.GetWeakPtr(), media_resource));
}

void MediaPlayerRendererClient::OnStreamTextureWrapperInitialized(
    media::MediaResource* media_resource,
    bool success) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (!success) {
    base::ResetAndReturn(&init_cb_).Run(
        media::PipelineStatus::PIPELINE_ERROR_INITIALIZATION_FAILED);
    return;
  }

  mojo_renderer_->Initialize(
      media_resource, this,
      base::Bind(&MediaPlayerRendererClient::OnRemoteRendererInitialized,
                 weak_factory_.GetWeakPtr()));
}

void MediaPlayerRendererClient::OnScopedSurfaceRequested(
    const base::UnguessableToken& request_token) {
  DCHECK(request_token);
  stream_texture_wrapper_->ForwardStreamTextureForSurfaceRequest(request_token);
}

void MediaPlayerRendererClient::OnRemoteRendererInitialized(
    media::PipelineStatus status) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(!init_cb_.is_null());

  // TODO(tguilbert): Measure and smooth out the initialization's ordering to
  // have the lowest total initialization time.
  mojo_renderer_->InitiateScopedSurfaceRequest(
      base::Bind(&MediaPlayerRendererClient::OnScopedSurfaceRequested,
                 weak_factory_.GetWeakPtr()));

  base::ResetAndReturn(&init_cb_).Run(status);
}

void MediaPlayerRendererClient::SetCdm(
    media::CdmContext* cdm_context,
    const media::CdmAttachedCB& cdm_attached_cb) {
  NOTREACHED();
}

void MediaPlayerRendererClient::Flush(const base::Closure& flush_cb) {
  mojo_renderer_->Flush(flush_cb);
}

void MediaPlayerRendererClient::StartPlayingFrom(base::TimeDelta time) {
  mojo_renderer_->StartPlayingFrom(time);
}

void MediaPlayerRendererClient::SetPlaybackRate(double playback_rate) {
  mojo_renderer_->SetPlaybackRate(playback_rate);
}

void MediaPlayerRendererClient::SetVolume(float volume) {
  mojo_renderer_->SetVolume(volume);
}

base::TimeDelta MediaPlayerRendererClient::GetMediaTime() {
  return mojo_renderer_->GetMediaTime();
}

void MediaPlayerRendererClient::OnFrameAvailable() {
  DCHECK(compositor_task_runner_->BelongsToCurrentThread());
  sink_->PaintSingleFrame(stream_texture_wrapper_->GetCurrentFrame(), true);
}

void MediaPlayerRendererClient::OnError(media::PipelineStatus status) {
  client_->OnError(status);
}

void MediaPlayerRendererClient::OnEnded() {
  client_->OnEnded();
}

void MediaPlayerRendererClient::OnStatisticsUpdate(
    const media::PipelineStatistics& stats) {
  client_->OnStatisticsUpdate(stats);
}

void MediaPlayerRendererClient::OnBufferingStateChange(
    media::BufferingState state) {
  client_->OnBufferingStateChange(state);
}

void MediaPlayerRendererClient::OnWaitingForDecryptionKey() {
  client_->OnWaitingForDecryptionKey();
}

void MediaPlayerRendererClient::OnAudioConfigChange(
    const media::AudioDecoderConfig& config) {
  client_->OnAudioConfigChange(config);
}
void MediaPlayerRendererClient::OnVideoConfigChange(
    const media::VideoDecoderConfig& config) {
  client_->OnVideoConfigChange(config);
}

void MediaPlayerRendererClient::OnVideoNaturalSizeChange(
    const gfx::Size& size) {
  stream_texture_wrapper_->UpdateTextureSize(size);
  client_->OnVideoNaturalSizeChange(size);
}

void MediaPlayerRendererClient::OnVideoOpacityChange(bool opaque) {
  client_->OnVideoOpacityChange(opaque);
}

void MediaPlayerRendererClient::OnDurationChange(base::TimeDelta duration) {
  client_->OnDurationChange(duration);
}

}  // namespace content
