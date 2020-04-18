// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/webmediaplayer_ms.h"

#include <stddef.h>
#include <limits>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/layers/video_frame_provider_client_impl.h"
#include "cc/layers/video_layer.h"
#include "content/child/child_process.h"
#include "content/public/renderer/media_stream_audio_renderer.h"
#include "content/public/renderer/media_stream_renderer_factory.h"
#include "content/public/renderer/media_stream_video_renderer.h"
#include "content/renderer/media/stream/media_stream_audio_track.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "content/renderer/media/stream/webmediaplayer_ms_compositor.h"
#include "content/renderer/media/web_media_element_source_utils.h"
#include "content/renderer/media/webrtc_logging.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_content_type.h"
#include "media/base/media_log.h"
#include "media/base/video_frame.h"
#include "media/base/video_rotation.h"
#include "media/base/video_types.h"
#include "media/blink/webmediaplayer_util.h"
#include "media/video/gpu_memory_buffer_video_frame_pool.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/public/platform/web_media_player_source.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace {

enum class RendererReloadAction {
  KEEP_RENDERER,
  REMOVE_RENDERER,
  NEW_RENDERER
};

bool IsPlayableTrack(const blink::WebMediaStreamTrack& track) {
  return !track.IsNull() && !track.Source().IsNull() &&
         track.Source().GetReadyState() !=
             blink::WebMediaStreamSource::kReadyStateEnded;
}

}  // namespace

namespace content {

#if defined(OS_WIN)
// Since we do not have native GMB support in Windows, using GMBs can cause a
// CPU regression. This is more apparent and can have adverse affects in lower
// resolution content which are defined by these thresholds, see
// https://crbug.com/835752.
// static
const gfx::Size WebMediaPlayerMS::kUseGpuMemoryBufferVideoFramesMinResolution =
    gfx::Size(1920, 1080);
#endif  // defined(OS_WIN)

// FrameDeliverer is responsible for delivering frames received on
// the IO thread by calling of EnqueueFrame() method of |compositor_|.
//
// It is created on the main thread, but methods should be called and class
// should be destructed on the IO thread.
class WebMediaPlayerMS::FrameDeliverer {
 public:
  FrameDeliverer(const base::WeakPtr<WebMediaPlayerMS>& player,
                 const MediaStreamVideoRenderer::RepaintCB& enqueue_frame_cb,
                 scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
                 scoped_refptr<base::TaskRunner> worker_task_runner,
                 media::GpuVideoAcceleratorFactories* gpu_factories)
      : last_frame_opaque_(true),
        last_frame_rotation_(media::VIDEO_ROTATION_0),
        received_first_frame_(false),
        main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
        player_(player),
        enqueue_frame_cb_(enqueue_frame_cb),
        media_task_runner_(media_task_runner),
        weak_factory_for_pool_(this),
        weak_factory_(this) {
    io_thread_checker_.DetachFromThread();

    if (gpu_factories && gpu_factories->ShouldUseGpuMemoryBuffersForVideoFrames(
                             true /* for_media_stream */)) {
      gpu_memory_buffer_pool_.reset(new media::GpuMemoryBufferVideoFramePool(
          media_task_runner, worker_task_runner, gpu_factories));
    }
  }

  ~FrameDeliverer() {
    DCHECK(io_thread_checker_.CalledOnValidThread());
    if (gpu_memory_buffer_pool_) {
      DropCurrentPoolTasks();
      media_task_runner_->DeleteSoon(FROM_HERE,
                                     gpu_memory_buffer_pool_.release());
    }
  }

  void OnVideoFrame(scoped_refptr<media::VideoFrame> frame) {
    DCHECK(io_thread_checker_.CalledOnValidThread());

// On Android, stop passing frames.
#if defined(OS_ANDROID)
    if (render_frame_suspended_)
      return;
#endif  // defined(OS_ANDROID)

    if (!gpu_memory_buffer_pool_) {
      EnqueueFrame(std::move(frame));
      return;
    }

#if defined(OS_WIN)
    const bool skip_creating_gpu_memory_buffer =
        frame->visible_rect().width() <
            kUseGpuMemoryBufferVideoFramesMinResolution.width() ||
        frame->visible_rect().height() <
            kUseGpuMemoryBufferVideoFramesMinResolution.height();
#else
    const bool skip_creating_gpu_memory_buffer = false;
#endif  // defined(OS_WIN)

    // If |render_frame_suspended_|, we can keep passing the frames to keep the
    // latest frame in compositor up to date. However, creating GMB backed
    // frames is unnecessary, because the frames are not going to be shown for
    // the time period.
    if (render_frame_suspended_ || skip_creating_gpu_memory_buffer) {
      EnqueueFrame(std::move(frame));
      // If there are any existing MaybeCreateHardwareFrame() calls, we do not
      // want those frames to be placed after the current one, so just drop
      // them.
      DropCurrentPoolTasks();
      return;
    }

    // |gpu_memory_buffer_pool_| deletion is going to be posted to
    // |media_task_runner_|. base::Unretained() usage is fine since
    // |gpu_memory_buffer_pool_| outlives the task.
    media_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &media::GpuMemoryBufferVideoFramePool::MaybeCreateHardwareFrame,
            base::Unretained(gpu_memory_buffer_pool_.get()), frame,
            media::BindToCurrentLoop(
                base::BindOnce(&FrameDeliverer::EnqueueFrame,
                               weak_factory_for_pool_.GetWeakPtr()))));
  }

  void SetRenderFrameSuspended(bool render_frame_suspended) {
    DCHECK(io_thread_checker_.CalledOnValidThread());
    render_frame_suspended_ = render_frame_suspended;
  }

  MediaStreamVideoRenderer::RepaintCB GetRepaintCallback() {
    return base::Bind(&FrameDeliverer::OnVideoFrame,
                      weak_factory_.GetWeakPtr());
  }

 private:
  friend class WebMediaPlayerMS;

  void EnqueueFrame(const scoped_refptr<media::VideoFrame>& frame) {
    DCHECK(io_thread_checker_.CalledOnValidThread());

    base::TimeTicks render_time;
    if (frame->metadata()->GetTimeTicks(
            media::VideoFrameMetadata::REFERENCE_TIME, &render_time)) {
      TRACE_EVENT1("media", "EnqueueFrame", "Ideal Render Instant",
                   render_time.ToInternalValue());
    } else {
      TRACE_EVENT0("media", "EnqueueFrame");
    }

    const bool is_opaque = media::IsOpaque(frame->format());
    media::VideoRotation video_rotation = media::VIDEO_ROTATION_0;
    ignore_result(frame->metadata()->GetRotation(
        media::VideoFrameMetadata::ROTATION, &video_rotation));

    if (!received_first_frame_) {
      received_first_frame_ = true;
      last_frame_opaque_ = is_opaque;
      last_frame_rotation_ = video_rotation;
      main_task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&WebMediaPlayerMS::OnFirstFrameReceived,
                                    player_, video_rotation, is_opaque));
    } else {
      if (last_frame_opaque_ != is_opaque) {
        last_frame_opaque_ = is_opaque;
        main_task_runner_->PostTask(
            FROM_HERE, base::BindOnce(&WebMediaPlayerMS::OnOpacityChanged,
                                      player_, is_opaque));
      }
      if (last_frame_rotation_ != video_rotation) {
        last_frame_rotation_ = video_rotation;
        main_task_runner_->PostTask(
            FROM_HERE, base::BindOnce(&WebMediaPlayerMS::OnRotationChanged,
                                      player_, video_rotation, is_opaque));
      }
    }

    enqueue_frame_cb_.Run(frame);
  }

  void DropCurrentPoolTasks() {
    DCHECK(io_thread_checker_.CalledOnValidThread());
    DCHECK(gpu_memory_buffer_pool_);

    if (!weak_factory_for_pool_.HasWeakPtrs())
      return;

    //  |gpu_memory_buffer_pool_| deletion is going to be posted to
    //  |media_task_runner_|. base::Unretained() usage is fine since
    //  |gpu_memory_buffer_pool_| outlives the task.
    media_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&media::GpuMemoryBufferVideoFramePool::Abort,
                       base::Unretained(gpu_memory_buffer_pool_.get())));
    weak_factory_for_pool_.InvalidateWeakPtrs();
  }

  bool last_frame_opaque_;
  media::VideoRotation last_frame_rotation_;
  bool received_first_frame_;
  bool render_frame_suspended_ = false;

  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  const base::WeakPtr<WebMediaPlayerMS> player_;
  const MediaStreamVideoRenderer::RepaintCB enqueue_frame_cb_;

  // Pool of GpuMemoryBuffers and resources used to create hardware frames.
  std::unique_ptr<media::GpuMemoryBufferVideoFramePool> gpu_memory_buffer_pool_;
  const scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;

  // Used for DCHECKs to ensure method calls are executed on the correct thread.
  base::ThreadChecker io_thread_checker_;

  base::WeakPtrFactory<FrameDeliverer> weak_factory_for_pool_;
  base::WeakPtrFactory<FrameDeliverer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FrameDeliverer);
};

WebMediaPlayerMS::WebMediaPlayerMS(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    media::WebMediaPlayerDelegate* delegate,
    std::unique_ptr<media::MediaLog> media_log,
    std::unique_ptr<MediaStreamRendererFactory> factory,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
    scoped_refptr<base::TaskRunner> worker_task_runner,
    media::GpuVideoAcceleratorFactories* gpu_factories,
    const blink::WebString& sink_id)
    : frame_(frame),
      network_state_(WebMediaPlayer::kNetworkStateEmpty),
      ready_state_(WebMediaPlayer::kReadyStateHaveNothing),
      buffered_(static_cast<size_t>(0)),
      client_(client),
      delegate_(delegate),
      delegate_id_(0),
      paused_(true),
      video_rotation_(media::VIDEO_ROTATION_0),
      media_log_(std::move(media_log)),
      renderer_factory_(std::move(factory)),
      io_task_runner_(io_task_runner),
      compositor_task_runner_(compositor_task_runner),
      media_task_runner_(media_task_runner),
      worker_task_runner_(worker_task_runner),
      gpu_factories_(gpu_factories),
      initial_audio_output_device_id_(sink_id.Utf8()),
      volume_(1.0),
      volume_multiplier_(1.0),
      should_play_upon_shown_(false) {
  DVLOG(1) << __func__;
  DCHECK(client);
  DCHECK(delegate_);
  delegate_id_ = delegate_->AddObserver(this);

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_CREATED));
}

WebMediaPlayerMS::~WebMediaPlayerMS() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!web_stream_.IsNull())
    web_stream_.RemoveObserver(this);

  // Destruct compositor resources in the proper order.
  get_client()->SetCcLayer(nullptr);
  if (video_layer_)
    video_layer_->StopUsingProvider();

  if (frame_deliverer_)
    io_task_runner_->DeleteSoon(FROM_HERE, frame_deliverer_.release());

  if (compositor_)
    compositor_->StopUsingProvider();

  if (video_frame_provider_)
    video_frame_provider_->Stop();

  if (audio_renderer_)
    audio_renderer_->Stop();

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_DESTROYED));

  delegate_->PlayerGone(delegate_id_);
  delegate_->RemoveObserver(delegate_id_);
}

void WebMediaPlayerMS::Load(LoadType load_type,
                            const blink::WebMediaPlayerSource& source,
                            CORSMode /*cors_mode*/) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  // TODO(acolwell): Change this to DCHECK_EQ(load_type, LoadTypeMediaStream)
  // once Blink-side changes land.
  DCHECK_NE(load_type, kLoadTypeMediaSource);
  web_stream_ = GetWebMediaStreamFromWebMediaPlayerSource(source);
  if (!web_stream_.IsNull())
    web_stream_.AddObserver(this);

  compositor_ = new WebMediaPlayerMSCompositor(
      compositor_task_runner_, io_task_runner_, web_stream_, AsWeakPtr());

  SetNetworkState(WebMediaPlayer::kNetworkStateLoading);
  SetReadyState(WebMediaPlayer::kReadyStateHaveNothing);
  std::string stream_id =
      web_stream_.IsNull() ? std::string() : web_stream_.Id().Utf8();
  media_log_->AddEvent(media_log_->CreateLoadEvent(stream_id));

  frame_deliverer_.reset(new WebMediaPlayerMS::FrameDeliverer(
      AsWeakPtr(),
      base::BindRepeating(&WebMediaPlayerMSCompositor::EnqueueFrame,
                          compositor_),
      media_task_runner_, worker_task_runner_, gpu_factories_));
  video_frame_provider_ = renderer_factory_->GetVideoRenderer(
      web_stream_,
      media::BindToCurrentLoop(
          base::Bind(&WebMediaPlayerMS::OnSourceError, AsWeakPtr())),
      frame_deliverer_->GetRepaintCallback(), io_task_runner_);

  RenderFrame* const frame = RenderFrame::FromWebFrame(frame_);

  int routing_id = MSG_ROUTING_NONE;
  GURL url = source.IsURL() ? GURL(source.GetAsURL()) : GURL();

  if (frame) {
    // Report UMA and RAPPOR metrics.
    media::ReportMetrics(load_type, url, frame_->GetSecurityOrigin(),
                         media_log_.get());
    routing_id = frame->GetRoutingID();
  }

  audio_renderer_ = renderer_factory_->GetAudioRenderer(
      web_stream_, routing_id, initial_audio_output_device_id_);

  if (!audio_renderer_)
    WebRtcLogMessage("Warning: Failed to instantiate audio renderer.");

  if (!video_frame_provider_ && !audio_renderer_) {
    SetNetworkState(WebMediaPlayer::kNetworkStateNetworkError);
    return;
  }

  if (audio_renderer_) {
    audio_renderer_->SetVolume(volume_);
    audio_renderer_->Start();

    // Store the ID of audio track being played in |current_video_track_id_|
    blink::WebVector<blink::WebMediaStreamTrack> audio_tracks;
    if (!web_stream_.IsNull()) {
      web_stream_.AudioTracks(audio_tracks);
      DCHECK_GT(audio_tracks.size(), 0U);
      current_audio_track_id_ = audio_tracks[0].Id();
    }
  }

  if (video_frame_provider_) {
    video_frame_provider_->Start();

    // Store the ID of video track being played in |current_video_track_id_|
    if (!web_stream_.IsNull()) {
      blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
      web_stream_.VideoTracks(video_tracks);
      DCHECK_GT(video_tracks.size(), 0U);
      current_video_track_id_ = video_tracks[0].Id();
    }
  }
  // When associated with an <audio> element, we don't want to wait for the
  // first video fram to become available as we do for <video> elements
  // (<audio> elements can also be assigned video tracks).
  // For more details, see crbug.com/738379
  if (audio_renderer_ &&
      (client_->IsAudioElement() || !video_frame_provider_)) {
    // This is audio-only mode.
    SetReadyState(WebMediaPlayer::kReadyStateHaveMetadata);
    SetReadyState(WebMediaPlayer::kReadyStateHaveEnoughData);
  }
}

void WebMediaPlayerMS::TrackAdded(const blink::WebMediaStreamTrack& track) {
  Reload();
}

void WebMediaPlayerMS::TrackRemoved(const blink::WebMediaStreamTrack& track) {
  Reload();
}

void WebMediaPlayerMS::ActiveStateChanged(bool is_active) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // The case when the stream becomes active is handled by TrackAdded().
  if (is_active)
    return;

  // This makes the media element elegible to be garbage collected. Otherwise,
  // the element will be considered active and will never be garbage
  // collected.
  SetNetworkState(kNetworkStateIdle);

  // Stop the audio renderer to free up resources that are not required for an
  // inactive stream. This is useful if the media element is not garbage
  // collected.
  // Note that the video renderer should not be stopped because the ended video
  // track is expected to produce a black frame after becoming inactive.
  if (audio_renderer_)
    audio_renderer_->Stop();
}

void WebMediaPlayerMS::Reload() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (web_stream_.IsNull())
    return;

  ReloadVideo();
  ReloadAudio();
}

void WebMediaPlayerMS::ReloadVideo() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!web_stream_.IsNull());
  blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
  // VideoTracks() is a getter.
  web_stream_.VideoTracks(video_tracks);

  RendererReloadAction renderer_action = RendererReloadAction::KEEP_RENDERER;
  if (video_tracks.IsEmpty()) {
    if (video_frame_provider_)
      renderer_action = RendererReloadAction::REMOVE_RENDERER;
    current_video_track_id_ = blink::WebString();
  } else if (video_tracks[0].Id() != current_video_track_id_ &&
             IsPlayableTrack(video_tracks[0])) {
    renderer_action = RendererReloadAction::NEW_RENDERER;
    current_video_track_id_ = video_tracks[0].Id();
  }

  switch (renderer_action) {
    case RendererReloadAction::NEW_RENDERER:
      if (video_frame_provider_)
        video_frame_provider_->Stop();

      SetNetworkState(kNetworkStateLoading);
      video_frame_provider_ = renderer_factory_->GetVideoRenderer(
          web_stream_,
          media::BindToCurrentLoop(
              base::Bind(&WebMediaPlayerMS::OnSourceError, AsWeakPtr())),
          frame_deliverer_->GetRepaintCallback(), io_task_runner_);
      DCHECK(video_frame_provider_);
      video_frame_provider_->Start();
      break;

    case RendererReloadAction::REMOVE_RENDERER:
      video_frame_provider_->Stop();
      video_frame_provider_ = nullptr;
      break;

    default:
      return;
  }

  DCHECK_NE(renderer_action, RendererReloadAction::KEEP_RENDERER);
  if (!paused_)
    delegate_->DidPlayerSizeChange(delegate_id_, NaturalSize());
}

void WebMediaPlayerMS::ReloadAudio() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!web_stream_.IsNull());
  RenderFrame* const frame = RenderFrame::FromWebFrame(frame_);
  if (!frame)
    return;

  blink::WebVector<blink::WebMediaStreamTrack> audio_tracks;
  // AudioTracks() is a getter.
  web_stream_.AudioTracks(audio_tracks);

  RendererReloadAction renderer_action = RendererReloadAction::KEEP_RENDERER;
  if (audio_tracks.IsEmpty()) {
    if (audio_renderer_)
      renderer_action = RendererReloadAction::REMOVE_RENDERER;
    current_audio_track_id_ = blink::WebString();
  } else if (audio_tracks[0].Id() != current_audio_track_id_ &&
             IsPlayableTrack(audio_tracks[0])) {
    renderer_action = RendererReloadAction::NEW_RENDERER;
    current_audio_track_id_ = audio_tracks[0].Id();
  }

  switch (renderer_action) {
    case RendererReloadAction::NEW_RENDERER:
      if (audio_renderer_)
        audio_renderer_->Stop();

      SetNetworkState(WebMediaPlayer::kNetworkStateLoading);
      audio_renderer_ = renderer_factory_->GetAudioRenderer(
          web_stream_, frame->GetRoutingID(), initial_audio_output_device_id_);

      // |audio_renderer_| can be null in tests.
      if (!audio_renderer_)
        break;

      audio_renderer_->SetVolume(volume_);
      audio_renderer_->Start();
      audio_renderer_->Play();
      break;

    case RendererReloadAction::REMOVE_RENDERER:
      audio_renderer_->Stop();
      audio_renderer_ = nullptr;
      break;

    default:
      break;
  }
}

void WebMediaPlayerMS::Play() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PLAY));
  if (!paused_)
    return;

  if (video_frame_provider_)
    video_frame_provider_->Resume();

  compositor_->StartRendering();

  if (audio_renderer_)
    audio_renderer_->Play();

  if (HasVideo())
    delegate_->DidPlayerSizeChange(delegate_id_, NaturalSize());

  // |delegate_| expects the notification only if there is at least one track
  // actually playing. A media stream might have none since tracks can be
  // removed from the stream.
  if (HasAudio() || HasVideo()) {
    // TODO(perkj, magjed): We use OneShot focus type here so that it takes
    // audio focus once it starts, and then will not respond to further audio
    // focus changes. See http://crbug.com/596516 for more details.
    delegate_->DidPlay(delegate_id_, HasVideo(), HasAudio(),
                       media::MediaContentType::OneShot);
  }

  delegate_->SetIdle(delegate_id_, false);
  paused_ = false;
}

void WebMediaPlayerMS::Pause() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  should_play_upon_shown_ = false;
  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PAUSE));
  if (paused_)
    return;

  if (video_frame_provider_)
    video_frame_provider_->Pause();

  compositor_->StopRendering();
  compositor_->ReplaceCurrentFrameWithACopy();

  if (audio_renderer_)
    audio_renderer_->Pause();

  delegate_->DidPause(delegate_id_);
  delegate_->SetIdle(delegate_id_, true);

  paused_ = true;
}

void WebMediaPlayerMS::Seek(double seconds) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void WebMediaPlayerMS::SetRate(double rate) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void WebMediaPlayerMS::SetVolume(double volume) {
  DVLOG(1) << __func__ << "(volume=" << volume << ")";
  DCHECK(thread_checker_.CalledOnValidThread());
  volume_ = volume;
  if (audio_renderer_.get())
    audio_renderer_->SetVolume(volume_ * volume_multiplier_);
  delegate_->DidPlayerMutedStatusChange(delegate_id_, volume == 0.0);
}

void WebMediaPlayerMS::EnterPictureInPicture(
    blink::WebMediaPlayer::PipWindowOpenedCallback callback) {
  // TODO(crbug.com/806249): Use Picture-in-Picture window size.
  std::move(callback).Run(this->NaturalSize());

  NOTIMPLEMENTED();
  // TODO(apacible): Implement after video in surfaces is supported for
  // WebMediaPlayerMS. See http://crbug/746182.
}

void WebMediaPlayerMS::ExitPictureInPicture(
    blink::WebMediaPlayer::PipWindowClosedCallback callback) {
  // TODO(crbug.com/806249): Run callback when Picture-in-Picture window closes.
  std::move(callback).Run();

  NOTIMPLEMENTED();
  // TODO(apacible): Implement after video in surfaces is supported for
  // WebMediaPlayerMS. See http://crbug/746182.
}

void WebMediaPlayerMS::RegisterPictureInPictureWindowResizeCallback(
    blink::WebMediaPlayer::PipWindowResizedCallback) {
  NOTIMPLEMENTED();
  // TODO(apacible): Implement after video in surfaces is supported for
  // WebMediaPlayerMS. See http://crbug/746182.
}

void WebMediaPlayerMS::SetSinkId(
    const blink::WebString& sink_id,
    blink::WebSetSinkIdCallbacks* web_callback) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
  const media::OutputDeviceStatusCB callback =
      media::ConvertToOutputDeviceStatusCB(web_callback);
  if (audio_renderer_) {
    audio_renderer_->SwitchOutputDevice(sink_id.Utf8(), callback);
  } else {
    callback.Run(media::OUTPUT_DEVICE_STATUS_ERROR_INTERNAL);
  }
}

void WebMediaPlayerMS::SetPreload(WebMediaPlayer::Preload preload) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

bool WebMediaPlayerMS::HasVideo() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return (video_frame_provider_.get() != nullptr);
}

bool WebMediaPlayerMS::HasAudio() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return (audio_renderer_.get() != nullptr);
}

blink::WebSize WebMediaPlayerMS::NaturalSize() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!video_frame_provider_)
    return blink::WebSize();

  if (video_rotation_ == media::VIDEO_ROTATION_90 ||
      video_rotation_ == media::VideoRotation::VIDEO_ROTATION_270) {
    const gfx::Size& current_size = compositor_->GetCurrentSize();
    return blink::WebSize(current_size.height(), current_size.width());
  }
  return blink::WebSize(compositor_->GetCurrentSize());
}

blink::WebSize WebMediaPlayerMS::VisibleRect() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  scoped_refptr<media::VideoFrame> video_frame =
      compositor_->GetCurrentFrameWithoutUpdatingStatistics();
  if (!video_frame)
    return blink::WebSize();

  const gfx::Rect& visible_rect = video_frame->visible_rect();
  if (video_rotation_ == media::VIDEO_ROTATION_90 ||
      video_rotation_ == media::VideoRotation::VIDEO_ROTATION_270) {
    return blink::WebSize(visible_rect.height(), visible_rect.width());
  }
  return blink::WebSize(visible_rect.width(), visible_rect.height());
}

bool WebMediaPlayerMS::Paused() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return paused_;
}

bool WebMediaPlayerMS::Seeking() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

double WebMediaPlayerMS::Duration() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return std::numeric_limits<double>::infinity();
}

double WebMediaPlayerMS::CurrentTime() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  const base::TimeDelta current_time = compositor_->GetCurrentTime();
  if (current_time.ToInternalValue() != 0)
    return current_time.InSecondsF();
  else if (audio_renderer_.get())
    return audio_renderer_->GetCurrentRenderTime().InSecondsF();
  return 0.0;
}

blink::WebMediaPlayer::NetworkState WebMediaPlayerMS::GetNetworkState() const {
  DVLOG(1) << __func__ << ", state:" << network_state_;
  DCHECK(thread_checker_.CalledOnValidThread());
  return network_state_;
}

blink::WebMediaPlayer::ReadyState WebMediaPlayerMS::GetReadyState() const {
  DVLOG(1) << __func__ << ", state:" << ready_state_;
  DCHECK(thread_checker_.CalledOnValidThread());
  return ready_state_;
}

blink::WebString WebMediaPlayerMS::GetErrorMessage() const {
  return blink::WebString::FromUTF8(media_log_->GetErrorMessage());
}

blink::WebTimeRanges WebMediaPlayerMS::Buffered() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return buffered_;
}

blink::WebTimeRanges WebMediaPlayerMS::Seekable() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return blink::WebTimeRanges();
}

bool WebMediaPlayerMS::DidLoadingProgress() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return true;
}

void WebMediaPlayerMS::Paint(blink::WebCanvas* canvas,
                             const blink::WebRect& rect,
                             cc::PaintFlags& flags,
                             int already_uploaded_id,
                             VideoFrameUploadMetadata* out_metadata) {
  DVLOG(3) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  const scoped_refptr<media::VideoFrame> frame =
      compositor_->GetCurrentFrameWithoutUpdatingStatistics();

  media::Context3D context_3d;
  if (frame && frame->HasTextures()) {
    auto* provider =
        RenderThreadImpl::current()->SharedMainThreadContextProvider().get();
    // GPU Process crashed.
    if (!provider)
      return;
    context_3d = media::Context3D(provider->ContextGL(), provider->GrContext());
    DCHECK(context_3d.gl);
  }
  const gfx::RectF dest_rect(rect.x, rect.y, rect.width, rect.height);
  video_renderer_.Paint(frame, canvas, dest_rect, flags, video_rotation_,
                        context_3d);
}

bool WebMediaPlayerMS::DidGetOpaqueResponseFromServiceWorker() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

bool WebMediaPlayerMS::HasSingleSecurityOrigin() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return true;
}

bool WebMediaPlayerMS::DidPassCORSAccessCheck() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return true;
}

double WebMediaPlayerMS::MediaTimeForTimeValue(double timeValue) const {
  return base::TimeDelta::FromSecondsD(timeValue).InSecondsF();
}

unsigned WebMediaPlayerMS::DecodedFrameCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return compositor_->total_frame_count();
}

unsigned WebMediaPlayerMS::DroppedFrameCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return compositor_->dropped_frame_count();
}

size_t WebMediaPlayerMS::AudioDecodedByteCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  NOTIMPLEMENTED();
  return 0;
}

size_t WebMediaPlayerMS::VideoDecodedByteCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  NOTIMPLEMENTED();
  return 0;
}

void WebMediaPlayerMS::OnFrameHidden() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // This method is called when the RenderFrame is sent to background or
  // suspended. During undoable tab closures OnHidden() may be called back to
  // back, so we can't rely on |render_frame_suspended_| being false here.
  if (frame_deliverer_) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&FrameDeliverer::SetRenderFrameSuspended,
                       base::Unretained(frame_deliverer_.get()), true));
  }

// On Android, substitute the displayed VideoFrame with a copy to avoid holding
// onto it unnecessarily.
#if defined(OS_ANDROID)
  if (!paused_)
    compositor_->ReplaceCurrentFrameWithACopy();
#endif  // defined(OS_ANDROID)
}

void WebMediaPlayerMS::OnFrameClosed() {
  DCHECK(thread_checker_.CalledOnValidThread());

// On Android, pause the video completely for this time period.
#if defined(OS_ANDROID)
  if (!paused_) {
    Pause();
    should_play_upon_shown_ = true;
  }

  delegate_->PlayerGone(delegate_id_);
#endif  // defined(OS_ANDROID)

  if (frame_deliverer_) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&FrameDeliverer::SetRenderFrameSuspended,
                       base::Unretained(frame_deliverer_.get()), true));
  }
}

void WebMediaPlayerMS::OnFrameShown() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (frame_deliverer_) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&FrameDeliverer::SetRenderFrameSuspended,
                       base::Unretained(frame_deliverer_.get()), false));
  }

// On Android, resume playback on visibility. play() clears
// |should_play_upon_shown_|.
#if defined(OS_ANDROID)
  if (should_play_upon_shown_)
    Play();
#endif  // defined(OS_ANDROID)
}

void WebMediaPlayerMS::OnIdleTimeout() {}

void WebMediaPlayerMS::OnPlay() {
  // TODO(perkj, magjed): It's not clear how WebRTC should work with an
  // MediaSession, until these issues are resolved, disable session controls.
  // http://crbug.com/595297.
}

void WebMediaPlayerMS::OnPause() {
  // TODO(perkj, magjed): See TODO in OnPlay().
}

void WebMediaPlayerMS::OnSeekForward(double seconds) {
  // TODO(perkj, magjed): See TODO in OnPlay().
}

void WebMediaPlayerMS::OnSeekBackward(double seconds) {
  // TODO(perkj, magjed): See TODO in OnPlay().
}

void WebMediaPlayerMS::OnVolumeMultiplierUpdate(double multiplier) {
  // TODO(perkj, magjed): See TODO in OnPlay().
}

void WebMediaPlayerMS::OnBecamePersistentVideo(bool value) {
  get_client()->OnBecamePersistentVideo(value);
}

void WebMediaPlayerMS::OnPictureInPictureModeEnded() {
  NOTIMPLEMENTED();
}

bool WebMediaPlayerMS::CopyVideoTextureToPlatformTexture(
    gpu::gles2::GLES2Interface* gl,
    unsigned target,
    unsigned int texture,
    unsigned internal_format,
    unsigned format,
    unsigned type,
    int level,
    bool premultiply_alpha,
    bool flip_y,
    int already_uploaded_id,
    VideoFrameUploadMetadata* out_metadata) {
  TRACE_EVENT0("media", "copyVideoTextureToPlatformTexture");
  DCHECK(thread_checker_.CalledOnValidThread());

  scoped_refptr<media::VideoFrame> video_frame =
      compositor_->GetCurrentFrameWithoutUpdatingStatistics();

  if (!video_frame.get() || !video_frame->HasTextures())
    return false;

  media::Context3D context_3d;
  auto* provider =
      RenderThreadImpl::current()->SharedMainThreadContextProvider().get();
  // GPU Process crashed.
  if (!provider)
    return false;
  context_3d = media::Context3D(provider->ContextGL(), provider->GrContext());
  DCHECK(context_3d.gl);
  return video_renderer_.CopyVideoFrameTexturesToGLTexture(
      context_3d, gl, video_frame.get(), target, texture, internal_format,
      format, type, level, premultiply_alpha, flip_y);
}

bool WebMediaPlayerMS::TexImageImpl(TexImageFunctionID functionID,
                                    unsigned target,
                                    gpu::gles2::GLES2Interface* gl,
                                    unsigned int texture,
                                    int level,
                                    int internalformat,
                                    unsigned format,
                                    unsigned type,
                                    int xoffset,
                                    int yoffset,
                                    int zoffset,
                                    bool flip_y,
                                    bool premultiply_alpha) {
  TRACE_EVENT0("media", "texImageImpl");
  DCHECK(thread_checker_.CalledOnValidThread());

  const scoped_refptr<media::VideoFrame> video_frame =
      compositor_->GetCurrentFrameWithoutUpdatingStatistics();
  if (!video_frame || !video_frame->IsMappable() ||
      video_frame->HasTextures() ||
      video_frame->format() != media::PIXEL_FORMAT_Y16) {
    return false;
  }

  if (functionID == kTexImage2D) {
    auto* provider =
        RenderThreadImpl::current()->SharedMainThreadContextProvider().get();
    // GPU Process crashed.
    if (!provider)
      return false;
    return media::PaintCanvasVideoRenderer::TexImage2D(
        target, texture, gl, provider->ContextCapabilities(), video_frame.get(),
        level, internalformat, format, type, flip_y, premultiply_alpha);
  } else if (functionID == kTexSubImage2D) {
    return media::PaintCanvasVideoRenderer::TexSubImage2D(
        target, gl, video_frame.get(), level, format, type, xoffset, yoffset,
        flip_y, premultiply_alpha);
  }
  return false;
}

void WebMediaPlayerMS::OnFirstFrameReceived(media::VideoRotation video_rotation,
                                            bool is_opaque) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
  SetReadyState(WebMediaPlayer::kReadyStateHaveMetadata);
  SetReadyState(WebMediaPlayer::kReadyStateHaveEnoughData);

  OnRotationChanged(video_rotation, is_opaque);
}

void WebMediaPlayerMS::OnOpacityChanged(bool is_opaque) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  // Opacity can be changed during the session without resetting
  // |video_layer_|.
  video_layer_->SetContentsOpaque(is_opaque);
}

void WebMediaPlayerMS::OnRotationChanged(media::VideoRotation video_rotation,
                                         bool is_opaque) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
  video_rotation_ = video_rotation;

  // Keep the old |video_layer_| alive until SetCcLayer() is called with a new
  // pointer, as it may use the pointer from the last call.
  auto new_video_layer =
      cc::VideoLayer::Create(compositor_.get(), video_rotation);
  new_video_layer->SetContentsOpaque(is_opaque);

  get_client()->SetCcLayer(new_video_layer.get());

  video_layer_ = std::move(new_video_layer);
}

void WebMediaPlayerMS::RepaintInternal() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
  get_client()->Repaint();
}

void WebMediaPlayerMS::OnSourceError() {
  DCHECK(thread_checker_.CalledOnValidThread());
  SetNetworkState(WebMediaPlayer::kNetworkStateFormatError);
  RepaintInternal();
}

void WebMediaPlayerMS::SetNetworkState(WebMediaPlayer::NetworkState state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  network_state_ = state;
  // Always notify to ensure client has the latest value.
  get_client()->NetworkStateChanged();
}

void WebMediaPlayerMS::SetReadyState(WebMediaPlayer::ReadyState state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ready_state_ = state;
  // Always notify to ensure client has the latest value.
  get_client()->ReadyStateChanged();
}

media::PaintCanvasVideoRenderer*
WebMediaPlayerMS::GetPaintCanvasVideoRenderer() {
  return &video_renderer_;
}

void WebMediaPlayerMS::ResetCanvasCache() {
  DCHECK(thread_checker_.CalledOnValidThread());
  video_renderer_.ResetCache();
}

void WebMediaPlayerMS::TriggerResize() {
  if (HasVideo())
    get_client()->SizeChanged();

  delegate_->DidPlayerSizeChange(delegate_id_, NaturalSize());
}

void WebMediaPlayerMS::SetGpuMemoryBufferVideoForTesting(
    media::GpuMemoryBufferVideoFramePool* gpu_memory_buffer_pool) {
  CHECK(frame_deliverer_);
  frame_deliverer_->gpu_memory_buffer_pool_.reset(gpu_memory_buffer_pool);
}

}  // namespace content
