// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_VIDEO_SINK_H_
#define CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_VIDEO_SINK_H_

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/common/media/video_capture.h"
#include "content/public/renderer/media_stream_sink.h"
#include "media/capture/video_capturer_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"

namespace content {

// MediaStreamVideoSink is an interface used for receiving video frames from a
// Video Stream Track or a Video Source. It should be extended by embedders,
// which connect/disconnect the sink implementation to a track to start/stop the
// flow of video frames.
//
// http://dev.w3.org/2011/webrtc/editor/getusermedia.html
// All methods calls will be done from the main render thread.
class CONTENT_EXPORT MediaStreamVideoSink : public MediaStreamSink {
 protected:
  MediaStreamVideoSink();
  ~MediaStreamVideoSink() override;

  // An implementation of MediaStreamVideoSink should call ConnectToTrack when
  // it is ready to receive data from a video track. Before the implementation
  // is destroyed, DisconnectFromTrack must be called. This MediaStreamVideoSink
  // base class holds a reference to the WebMediaStreamTrack until
  // DisconnectFromTrack is called.
  //
  // Calls to these methods must be done on the main render thread.
  // Note that |callback| for frame delivery happens on the IO thread.
  //
  // Warning: Calling DisconnectFromTrack does not immediately stop frame
  // delivery through the |callback|, since frames are being delivered on a
  // different thread.
  //
  // |is_sink_secure| indicates if this MediaStreamVideoSink is secure (i.e.
  // meets output protection requirement). Generally, this should be false
  // unless you know what you are doing.
  void ConnectToTrack(const blink::WebMediaStreamTrack& track,
                      const VideoCaptureDeliverFrameCB& callback,
                      bool is_sink_secure);
  void DisconnectFromTrack();

  // Returns the currently-connected track, or a null instance otherwise.
  const blink::WebMediaStreamTrack& connected_track() const {
    return connected_track_;
  }

 private:
  // Set by ConnectToTrack() and cleared by DisconnectFromTrack().
  blink::WebMediaStreamTrack connected_track_;
};


}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_VIDEO_SINK_H_
