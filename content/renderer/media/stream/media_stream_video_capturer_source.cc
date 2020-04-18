// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/media_stream_video_capturer_source.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/debug/stack_trace.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/media_stream_request.h"
#include "content/public/renderer/render_frame.h"
#include "content/renderer/media/stream/media_stream_constraints_util.h"
#include "content/renderer/media/video_capture_impl_manager.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/limits.h"
#include "media/base/video_frame.h"
#include "media/capture/video_capturer_source.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace content {

namespace {

// LocalVideoCapturerSource is a delegate used by MediaStreamVideoCapturerSource
// for local video capture. It uses the Render singleton VideoCaptureImplManager
// to start / stop and receive I420 frames from Chrome's video capture
// implementation. This is a main Render thread only object.
class LocalVideoCapturerSource final : public media::VideoCapturerSource {
 public:
  explicit LocalVideoCapturerSource(int session_id);
  ~LocalVideoCapturerSource() override;

  // VideoCaptureDelegate Implementation.
  media::VideoCaptureFormats GetPreferredFormats() override;
  void StartCapture(const media::VideoCaptureParams& params,
                    const VideoCaptureDeliverFrameCB& new_frame_callback,
                    const RunningCallback& running_callback) override;
  void RequestRefreshFrame() override;
  void MaybeSuspend() override;
  void Resume() override;
  void StopCapture() override;

 private:
  void OnStateUpdate(VideoCaptureState state);

  // |session_id_| identifies the capture device used for this capture session.
  const media::VideoCaptureSessionId session_id_;

  VideoCaptureImplManager* const manager_;

  base::Closure release_device_cb_;

  // These two are valid between StartCapture() and StopCapture().
  // |running_call_back_| is run when capture is successfully started, and when
  // it is stopped or error happens.
  RunningCallback running_callback_;
  base::Closure stop_capture_cb_;

  // Bound to the main render thread.
  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<LocalVideoCapturerSource> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LocalVideoCapturerSource);
};

LocalVideoCapturerSource::LocalVideoCapturerSource(int session_id)
    : session_id_(session_id),
      manager_(RenderThreadImpl::current()->video_capture_impl_manager()),
      release_device_cb_(manager_->UseDevice(session_id_)),
      weak_factory_(this) {
  DCHECK(RenderThreadImpl::current());
}

LocalVideoCapturerSource::~LocalVideoCapturerSource() {
  DCHECK(thread_checker_.CalledOnValidThread());
  release_device_cb_.Run();
}

media::VideoCaptureFormats LocalVideoCapturerSource::GetPreferredFormats() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return media::VideoCaptureFormats();
}

void LocalVideoCapturerSource::StartCapture(
    const media::VideoCaptureParams& params,
    const VideoCaptureDeliverFrameCB& new_frame_callback,
    const RunningCallback& running_callback) {
  DCHECK(params.requested_format.IsValid());
  DCHECK(thread_checker_.CalledOnValidThread());
  running_callback_ = running_callback;

  stop_capture_cb_ =
      manager_->StartCapture(session_id_, params,
                             media::BindToCurrentLoop(base::Bind(
                                 &LocalVideoCapturerSource::OnStateUpdate,
                                 weak_factory_.GetWeakPtr())),
                             new_frame_callback);
}

void LocalVideoCapturerSource::RequestRefreshFrame() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (stop_capture_cb_.is_null())
    return;  // Do not request frames if the source is stopped.
  manager_->RequestRefreshFrame(session_id_);
}

void LocalVideoCapturerSource::MaybeSuspend() {
  DCHECK(thread_checker_.CalledOnValidThread());
  manager_->Suspend(session_id_);
}

void LocalVideoCapturerSource::Resume() {
  DCHECK(thread_checker_.CalledOnValidThread());
  manager_->Resume(session_id_);
}

void LocalVideoCapturerSource::StopCapture() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Immediately make sure we don't provide more frames.
  if (!stop_capture_cb_.is_null())
    base::ResetAndReturn(&stop_capture_cb_).Run();
}

void LocalVideoCapturerSource::OnStateUpdate(VideoCaptureState state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (running_callback_.is_null())
    return;
  switch (state) {
    case VIDEO_CAPTURE_STATE_STARTED:
      running_callback_.Run(true);
      break;

    case VIDEO_CAPTURE_STATE_STOPPING:
    case VIDEO_CAPTURE_STATE_STOPPED:
    case VIDEO_CAPTURE_STATE_ERROR:
    case VIDEO_CAPTURE_STATE_ENDED:
      release_device_cb_.Run();
      release_device_cb_ = manager_->UseDevice(session_id_);
      running_callback_.Run(false);
      break;

    case VIDEO_CAPTURE_STATE_STARTING:
    case VIDEO_CAPTURE_STATE_PAUSED:
    case VIDEO_CAPTURE_STATE_RESUMED:
      // Not applicable to reporting on device starts or errors.
      break;
  }
}

}  // namespace

MediaStreamVideoCapturerSource::MediaStreamVideoCapturerSource(
    const SourceStoppedCallback& stop_callback,
    std::unique_ptr<media::VideoCapturerSource> source)
    : source_(std::move(source)) {
  blink::WebLocalFrame* web_frame =
      blink::WebLocalFrame::FrameForCurrentContext();
  RenderFrame* render_frame = RenderFrame::FromWebFrame(web_frame);
  render_frame_id_ =
      render_frame ? render_frame->GetRoutingID() : MSG_ROUTING_NONE;
  media::VideoCaptureFormats preferred_formats = source_->GetPreferredFormats();
  if (!preferred_formats.empty())
    capture_params_.requested_format = preferred_formats.front();
  SetStopCallback(stop_callback);
}

MediaStreamVideoCapturerSource::MediaStreamVideoCapturerSource(
    int render_frame_id,
    const SourceStoppedCallback& stop_callback,
    const MediaStreamDevice& device,
    const media::VideoCaptureParams& capture_params)
    : render_frame_id_(render_frame_id),
      source_(new LocalVideoCapturerSource(device.session_id)),
      capture_params_(capture_params) {
  SetStopCallback(stop_callback);
  SetDevice(device);
  SetDeviceRotationDetection(true /* enabled */);
}

MediaStreamVideoCapturerSource::~MediaStreamVideoCapturerSource() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MediaStreamVideoCapturerSource::RequestRefreshFrame() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  source_->RequestRefreshFrame();
}

void MediaStreamVideoCapturerSource::OnHasConsumers(bool has_consumers) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (has_consumers)
    source_->Resume();
  else
    source_->MaybeSuspend();
}

void MediaStreamVideoCapturerSource::OnCapturingLinkSecured(bool is_secure) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RenderFrame* render_frame = RenderFrame::FromRoutingID(render_frame_id_);
  if (!render_frame)
    return;
  GetMediaStreamDispatcherHost(render_frame)
      ->SetCapturingLinkSecured(device().session_id, device().type, is_secure);
}

void MediaStreamVideoCapturerSource::StartSourceImpl(
    const VideoCaptureDeliverFrameCB& frame_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  state_ = STARTING;
  frame_callback_ = frame_callback;
  source_->StartCapture(
      capture_params_, frame_callback_,
      base::Bind(&MediaStreamVideoCapturerSource::OnRunStateChanged,
                 base::Unretained(this), capture_params_));
}

void MediaStreamVideoCapturerSource::StopSourceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  source_->StopCapture();
}

void MediaStreamVideoCapturerSource::StopSourceForRestartImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (state_ != STARTED) {
    OnStopForRestartDone(false);
    return;
  }
  state_ = STOPPING_FOR_RESTART;
  source_->StopCapture();

  // Force state update for nondevice sources, since they do not
  // automatically update state after StopCapture().
  if (device().type == MEDIA_NO_SERVICE)
    OnRunStateChanged(capture_params_, false);
}

void MediaStreamVideoCapturerSource::RestartSourceImpl(
    const media::VideoCaptureFormat& new_format) {
  DCHECK(new_format.IsValid());
  media::VideoCaptureParams new_capture_params = capture_params_;
  new_capture_params.requested_format = new_format;
  state_ = RESTARTING;
  source_->StartCapture(
      new_capture_params, frame_callback_,
      base::Bind(&MediaStreamVideoCapturerSource::OnRunStateChanged,
                 base::Unretained(this), new_capture_params));
}

base::Optional<media::VideoCaptureFormat>
MediaStreamVideoCapturerSource::GetCurrentFormat() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return capture_params_.requested_format;
}

base::Optional<media::VideoCaptureParams>
MediaStreamVideoCapturerSource::GetCurrentCaptureParams() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return capture_params_;
}

void MediaStreamVideoCapturerSource::OnRunStateChanged(
    const media::VideoCaptureParams& new_capture_params,
    bool is_running) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  switch (state_) {
    case STARTING:
      if (is_running) {
        state_ = STARTED;
        DCHECK(capture_params_ == new_capture_params);
        OnStartDone(MEDIA_DEVICE_OK);
      } else {
        state_ = STOPPED;
        OnStartDone(MEDIA_DEVICE_TRACK_START_FAILURE_VIDEO);
      }
      break;
    case STARTED:
      if (!is_running) {
        state_ = STOPPED;
        StopSource();
      }
      break;
    case STOPPING_FOR_RESTART:
      state_ = is_running ? STARTED : STOPPED;
      OnStopForRestartDone(!is_running);
      break;
    case RESTARTING:
      if (is_running) {
        state_ = STARTED;
        capture_params_ = new_capture_params;
      } else {
        state_ = STOPPED;
      }
      OnRestartDone(is_running);
      break;
    case STOPPED:
      break;
  }
}

const mojom::MediaStreamDispatcherHostPtr&
MediaStreamVideoCapturerSource::GetMediaStreamDispatcherHost(
    RenderFrame* render_frame) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!dispatcher_host_) {
    render_frame->GetRemoteInterfaces()->GetInterface(
        mojo::MakeRequest(&dispatcher_host_));
  }
  return dispatcher_host_;
}

}  // namespace content
