// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_MEDIA_CAST_RECEIVER_SESSION_H_
#define CHROME_RENDERER_MEDIA_CAST_RECEIVER_SESSION_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "chrome/renderer/media/cast_receiver_session_delegate.h"

namespace media {
class AudioCapturerSource;
struct VideoCaptureFormat;
class VideoCapturerSource;
}

namespace net {
class IPEndPoint;
}

namespace base {
class DictionaryValue;
}

// This a render thread object, all methods, construction and
// destruction must happen on the render thread.
class CastReceiverSession : public base::RefCounted<CastReceiverSession> {
 public:
  CastReceiverSession();

  typedef base::Callback<void(scoped_refptr<media::AudioCapturerSource>,
                              std::unique_ptr<media::VideoCapturerSource>)>
      StartCB;

  // Note that the cast receiver will start responding to
  // incoming network streams immediately, buffering input until
  // StartAudio/StartVideo is called.
  // Five first parameters are passed to cast receiver.
  // |start_callback| is called when initialization is done.
  // TODO(hubbe): Currently the audio component of the returned media
  // stream only the exact format that the sender is sending us.
  void Start(const media::cast::FrameReceiverConfig& audio_config,
             const media::cast::FrameReceiverConfig& video_config,
             const net::IPEndPoint& local_endpoint,
             const net::IPEndPoint& remote_endpoint,
             std::unique_ptr<base::DictionaryValue> options,
             const media::VideoCaptureFormat& capture_format,
             const StartCB& start_callback,
             const CastReceiverSessionDelegate::ErrorCallback& error_callback);

 private:
  class VideoCapturerSource;
  class AudioCapturerSource;
  friend class base::RefCounted<CastReceiverSession>;
  virtual ~CastReceiverSession();
  void StartAudio(scoped_refptr<CastReceiverAudioValve> audio_valve);

  void StartVideo(content::VideoCaptureDeliverFrameCB frame_callback);
  // Stop Video callbacks.
  // Note that this returns immediately, but callbacks do not stop immediately.
  void StopVideo();

  media::cast::FrameReceiverConfig audio_config_;
  media::cast::FrameReceiverConfig video_config_;
  std::unique_ptr<CastReceiverSessionDelegate> delegate_;
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  media::VideoCaptureFormat format_;

  DISALLOW_COPY_AND_ASSIGN(CastReceiverSession);
};

#endif // CHROME_RENDERER_MEDIA_CAST_RECEIVER_SESSION_H_
