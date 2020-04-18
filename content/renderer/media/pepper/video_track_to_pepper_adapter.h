// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_VIDEO_VIDEO_TRACK_TO_PEPPER_ADAPTER_H_
#define CONTENT_RENDERER_MEDIA_VIDEO_VIDEO_TRACK_TO_PEPPER_ADAPTER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "media/base/video_frame.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"

namespace content {

class MediaStreamRegistryInterface;
class PpFrameReceiver;

// Interface used by a Pepper plugin to get captured frames from a video track.
class CONTENT_EXPORT FrameReaderInterface {
 public:
  // Got a new captured frame.
  virtual void GotFrame(const scoped_refptr<media::VideoFrame>& frame) = 0;

 protected:
  virtual ~FrameReaderInterface() {}
};

// VideoTrackToPepperAdapter is a glue class between MediaStreamVideoTrack and a
// Pepper plugin host.
class CONTENT_EXPORT VideoTrackToPepperAdapter {
 public:
  // |registry| is used to look up the media stream by url. If a NULL |registry|
  // is given, the global blink::WebMediaStreamRegistry will be used.
  explicit VideoTrackToPepperAdapter(MediaStreamRegistryInterface* registry);
  virtual ~VideoTrackToPepperAdapter();
  // Connects to the first video track in the MediaStream specified by |url| and
  // the received frames will be delivered via |reader|.
  // Returns true on success and false on failure.
  bool Open(const std::string& url, FrameReaderInterface* reader);
  // Closes |reader|'s connection with the video track, i.e. stops receiving
  // frames from the video track.
  // Returns true on success and false on failure.
  bool Close(FrameReaderInterface* reader);

 private:
  friend class VideoTrackToPepperAdapterTest;

  struct SourceInfo {
    SourceInfo(const blink::WebMediaStreamTrack& blink_track,
               FrameReaderInterface* reader);
    ~SourceInfo();

    std::unique_ptr<PpFrameReceiver> receiver_;
  };

  typedef std::map<FrameReaderInterface*, SourceInfo*> SourceInfoMap;

  blink::WebMediaStreamTrack GetFirstVideoTrack(const std::string& url);

  // Deliver VideoFrame to the MediaStreamVideoSink associated with
  // |reader|. For testing only.
  void DeliverFrameForTesting(FrameReaderInterface* reader,
                              const scoped_refptr<media::VideoFrame>& frame);

  MediaStreamRegistryInterface* const registry_;
  SourceInfoMap reader_to_receiver_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(VideoTrackToPepperAdapter);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_VIDEO_VIDEO_TRACK_TO_PEPPER_ADAPTER_H_
