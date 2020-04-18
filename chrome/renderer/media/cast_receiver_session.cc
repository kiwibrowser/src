// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/cast_receiver_session.h"

#include <memory>

#include "base/location.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/renderer/media/cast_receiver_audio_valve.h"
#include "content/public/renderer/render_thread.h"
#include "media/base/audio_capturer_source.h"
#include "media/base/bind_to_current_loop.h"
#include "media/capture/video_capturer_source.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"

// This is a render thread object.
class CastReceiverSession::AudioCapturerSource :
    public media::AudioCapturerSource {
 public:
  AudioCapturerSource(
      const scoped_refptr<CastReceiverSession> cast_receiver_session);
  void Initialize(const media::AudioParameters& params,
                  CaptureCallback* callback) override;
  void Start() override;
  void Stop() override;
  void SetVolume(double volume) override;
  void SetAutomaticGainControl(bool enable) override;
  void SetOutputDeviceForAec(const std::string& output_device_id) override;

 private:
  ~AudioCapturerSource() override;
  const scoped_refptr<CastReceiverSession> cast_receiver_session_;
  scoped_refptr<CastReceiverAudioValve> audio_valve_;
};

// This is a render thread object.
class CastReceiverSession::VideoCapturerSource
    : public media::VideoCapturerSource {
 public:
  explicit VideoCapturerSource(
      const scoped_refptr<CastReceiverSession> cast_receiver_session);
 protected:
  media::VideoCaptureFormats GetPreferredFormats() override;
  void StartCapture(const media::VideoCaptureParams& params,
                    const VideoCaptureDeliverFrameCB& frame_callback,
                    const RunningCallback& running_callback) override;
  void StopCapture() override;
 private:
  const scoped_refptr<CastReceiverSession> cast_receiver_session_;
};

CastReceiverSession::CastReceiverSession()
    : delegate_(new CastReceiverSessionDelegate()),
      io_task_runner_(content::RenderThread::Get()->GetIOTaskRunner()) {}

CastReceiverSession::~CastReceiverSession() {
  // We should always be able to delete the object on the IO thread.
  CHECK(io_task_runner_->DeleteSoon(FROM_HERE, delegate_.release()));
}

void CastReceiverSession::Start(
    const media::cast::FrameReceiverConfig& audio_config,
    const media::cast::FrameReceiverConfig& video_config,
    const net::IPEndPoint& local_endpoint,
    const net::IPEndPoint& remote_endpoint,
    std::unique_ptr<base::DictionaryValue> options,
    const media::VideoCaptureFormat& capture_format,
    const StartCB& start_callback,
    const CastReceiverSessionDelegate::ErrorCallback& error_callback) {
  audio_config_ = audio_config;
  video_config_ = video_config;
  format_ = capture_format;
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&CastReceiverSessionDelegate::Start,
                                base::Unretained(delegate_.get()), audio_config,
                                video_config, local_endpoint, remote_endpoint,
                                std::move(options), format_,
                                media::BindToCurrentLoop(error_callback)));
  scoped_refptr<media::AudioCapturerSource> audio(
      new CastReceiverSession::AudioCapturerSource(this));
  std::unique_ptr<media::VideoCapturerSource> video(
      new CastReceiverSession::VideoCapturerSource(this));
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(start_callback, audio, std::move(video)));
}

void CastReceiverSession::StartAudio(
    scoped_refptr<CastReceiverAudioValve> audio_valve) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CastReceiverSessionDelegate::StartAudio,
                     base::Unretained(delegate_.get()), audio_valve));
}

void CastReceiverSession::StartVideo(
    content::VideoCaptureDeliverFrameCB frame_callback) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CastReceiverSessionDelegate::StartVideo,
                     base::Unretained(delegate_.get()), frame_callback));
}

void CastReceiverSession::StopVideo() {
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&CastReceiverSessionDelegate::StopVideo,
                                base::Unretained(delegate_.get())));
}

CastReceiverSession::VideoCapturerSource::VideoCapturerSource(
    const scoped_refptr<CastReceiverSession> cast_receiver_session)
    : cast_receiver_session_(cast_receiver_session) {
}

media::VideoCaptureFormats
CastReceiverSession::VideoCapturerSource::GetPreferredFormats() {
  media::VideoCaptureFormats formats;
  if (cast_receiver_session_->format_.IsValid())
    formats.push_back(cast_receiver_session_->format_);
  return formats;
}

void CastReceiverSession::VideoCapturerSource::StartCapture(
      const media::VideoCaptureParams& params,
      const VideoCaptureDeliverFrameCB& frame_callback,
      const RunningCallback& running_callback) {
  cast_receiver_session_->StartVideo(frame_callback);
  running_callback.Run(true);
}

void CastReceiverSession::VideoCapturerSource::StopCapture() {
  cast_receiver_session_->StopVideo();
}

CastReceiverSession::AudioCapturerSource::AudioCapturerSource(
    const scoped_refptr<CastReceiverSession> cast_receiver_session)
    : cast_receiver_session_(cast_receiver_session) {
}

CastReceiverSession::AudioCapturerSource::~AudioCapturerSource() {
  DCHECK(!audio_valve_);
}

void CastReceiverSession::AudioCapturerSource::Initialize(
    const media::AudioParameters& params,
    CaptureCallback* callback) {
  // TODO(hubbe): Consider converting the audio to whatever the caller wants.
  if (params.sample_rate() !=
      cast_receiver_session_->audio_config_.rtp_timebase ||
      params.channels() != cast_receiver_session_->audio_config_.channels) {
    callback->OnCaptureError(std::string());
    return;
  }
  audio_valve_ = new CastReceiverAudioValve(params, callback);
}

void CastReceiverSession::AudioCapturerSource::Start() {
  DCHECK(audio_valve_);
  cast_receiver_session_->StartAudio(audio_valve_);
}

void CastReceiverSession::AudioCapturerSource::Stop() {
  audio_valve_->Stop();
  audio_valve_ = nullptr;
}

void CastReceiverSession::AudioCapturerSource::SetVolume(double volume) {
  // not supported
}

void CastReceiverSession::AudioCapturerSource::SetAutomaticGainControl(
    bool enable) {
  // not supported
}

void CastReceiverSession::AudioCapturerSource::SetOutputDeviceForAec(
    const std::string& output_device_id) {
  // not supported
}
