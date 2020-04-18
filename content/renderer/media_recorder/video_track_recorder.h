// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_RECORDER_VIDEO_TRACK_RECORDER_H_
#define CONTENT_RENDERER_MEDIA_RECORDER_VIDEO_TRACK_RECORDER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "content/public/common/buildflags.h"
#include "content/public/renderer/media_stream_video_sink.h"
#include "media/muxers/webm_muxer.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace base {
class Thread;
}  // namespace base

namespace cc {
class PaintCanvas;
}  // namespace cc

namespace media {
class PaintCanvasVideoRenderer;
class VideoFrame;
}  // namespace media

namespace video_track_recorder {
#if defined(OS_ANDROID)
const int kVEAEncoderMinResolutionWidth = 176;
const int kVEAEncoderMinResolutionHeight = 144;
#else
const int kVEAEncoderMinResolutionWidth = 640;
const int kVEAEncoderMinResolutionHeight = 480;
#endif
}  // namespace video_track_recorder

namespace content {

// VideoTrackRecorder is a MediaStreamVideoSink that encodes the video frames
// received from a Stream Video Track. This class is constructed and used on a
// single thread, namely the main Render thread. This mirrors the other
// MediaStreamVideo* classes that are constructed/configured on Main Render
// thread but that pass frames on Render IO thread. It has an internal Encoder
// with its own threading subtleties, see the implementation file.
class CONTENT_EXPORT VideoTrackRecorder : public MediaStreamVideoSink {
 public:
  // Do not change the order of codecs; add new ones right before LAST.
  enum class CodecId {
    VP8,
    VP9,
#if BUILDFLAG(RTC_USE_H264)
    H264,
#endif
    LAST
  };

  using OnEncodedVideoCB =
      base::Callback<void(const media::WebmMuxer::VideoParameters& params,
                          std::unique_ptr<std::string> encoded_data,
                          std::unique_ptr<std::string> encoded_alpha,
                          base::TimeTicks capture_timestamp,
                          bool is_key_frame)>;
  using OnErrorCB = base::Closure;

  // Base class to describe a generic Encoder, encapsulating all actual encoder
  // (re)configurations, encoding and delivery of received frames. This class is
  // ref-counted to allow the MediaStreamVideoTrack to hold a reference to it
  // (via the callback that MediaStreamVideoSink passes along) and to jump back
  // and forth to an internal encoder thread. Moreover, this class:
  // - is created on its parent's thread (usually the main Render thread), that
  // is, |main_task_runner_|.
  // - receives VideoFrames on |origin_task_runner_| and runs OnEncodedVideoCB
  // on that thread as well. This task runner is cached on first frame arrival,
  // and is supposed to be the render IO thread (but this is not enforced);
  // - uses an internal |encoding_task_runner_| for actual encoder interactions,
  // namely configuration, encoding (which might take some time) and
  // destruction. This task runner can be passed on the creation. If nothing is
  // passed, a new encoding thread is created and used.
  class Encoder : public base::RefCountedThreadSafe<Encoder> {
   public:
    Encoder(const OnEncodedVideoCB& on_encoded_video_callback,
            int32_t bits_per_second,
            scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
            scoped_refptr<base::SingleThreadTaskRunner> encoding_task_runner =
                nullptr);

    // Start encoding |frame|, returning via |on_encoded_video_callback_|. This
    // call will also trigger an encode configuration upon first frame arrival
    // or parameter change, and an EncodeOnEncodingTaskRunner() to actually
    // encode the frame. If the |frame|'s data is not directly available (e.g.
    // it's a texture) then RetrieveFrameOnMainThread() is called, and if even
    // that fails, black frames are sent instead.
    void StartFrameEncode(const scoped_refptr<media::VideoFrame>& frame,
                          base::TimeTicks capture_timestamp);
    void RetrieveFrameOnMainThread(
        const scoped_refptr<media::VideoFrame>& video_frame,
        base::TimeTicks capture_timestamp);

    static void OnFrameEncodeCompleted(
        const VideoTrackRecorder::OnEncodedVideoCB& on_encoded_video_cb,
        const media::WebmMuxer::VideoParameters& params,
        std::unique_ptr<std::string> data,
        std::unique_ptr<std::string> alpha_data,
        base::TimeTicks capture_timestamp,
        bool keyframe);

    void SetPaused(bool paused);
    virtual bool CanEncodeAlphaChannel();

   protected:
    friend class base::RefCountedThreadSafe<Encoder>;
    friend class VideoTrackRecorderTest;

    virtual ~Encoder();

    virtual void EncodeOnEncodingTaskRunner(
        scoped_refptr<media::VideoFrame> frame,
        base::TimeTicks capture_timestamp) = 0;

    // Called when the frame reference is released after encode.
    void FrameReleased(const scoped_refptr<media::VideoFrame>& frame);

    // Used to shutdown properly on the same thread we were created.
    const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

    // Task runner where frames to encode and reply callbacks must happen.
    scoped_refptr<base::SingleThreadTaskRunner> origin_task_runner_;

    // Task runner where encoding interactions happen.
    scoped_refptr<base::SingleThreadTaskRunner> encoding_task_runner_;

    // Optional thread for encoding. Active for the lifetime of VpxEncoder.
    std::unique_ptr<base::Thread> encoding_thread_;

    // While |paused_|, frames are not encoded. Used only from
    // |encoding_thread_|.
    bool paused_;

    // This callback should be exercised on IO thread.
    const OnEncodedVideoCB on_encoded_video_callback_;

    // Target bitrate for video encoding. If 0, a standard bitrate is used.
    const int32_t bits_per_second_;

    // Number of frames that we keep the reference alive for encode.
    uint32_t num_frames_in_encode_;

    // Used to retrieve incoming opaque VideoFrames (i.e. VideoFrames backed by
    // textures). Created on-demand on |main_task_runner_|.
    std::unique_ptr<media::PaintCanvasVideoRenderer> video_renderer_;
    SkBitmap bitmap_;
    std::unique_ptr<cc::PaintCanvas> canvas_;

    DISALLOW_COPY_AND_ASSIGN(Encoder);
  };

  static CodecId GetPreferredCodecId();
  static bool CanUseAcceleratedEncoder(CodecId codec,
                                       size_t width,
                                       size_t height);

  VideoTrackRecorder(
      CodecId codec,
      const blink::WebMediaStreamTrack& track,
      const OnEncodedVideoCB& on_encoded_video_cb,
      int32_t bits_per_second,
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner);
  ~VideoTrackRecorder() override;

  void Pause();
  void Resume();

  void OnVideoFrameForTesting(const scoped_refptr<media::VideoFrame>& frame,
                              base::TimeTicks capture_time);
 private:
  friend class VideoTrackRecorderTest;
  void InitializeEncoder(CodecId codec,
                         const OnEncodedVideoCB& on_encoded_video_callback,
                         int32_t bits_per_second,
                         bool allow_vea_encoder,
                         const scoped_refptr<media::VideoFrame>& frame,
                         base::TimeTicks capture_time);
  void OnError();

  // Used to check that we are destroyed on the same thread we were created.
  THREAD_CHECKER(main_thread_checker_);

  // We need to hold on to the Blink track to remove ourselves on dtor.
  blink::WebMediaStreamTrack track_;

  // Inner class to encode using whichever codec is configured.
  scoped_refptr<Encoder> encoder_;

  base::Callback<void(bool allow_vea_encoder,
                      const scoped_refptr<media::VideoFrame>& frame,
                      base::TimeTicks capture_time)>
      initialize_encoder_callback_;

  // Used to track the paused state during the initialization process.
  bool paused_before_init_;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  base::WeakPtrFactory<VideoTrackRecorder> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoTrackRecorder);
};

}  // namespace content
#endif  // CONTENT_RENDERER_MEDIA_RECORDER_VIDEO_TRACK_RECORDER_H_
