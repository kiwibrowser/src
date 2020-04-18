// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_TRACK_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_TRACK_H_

#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "media/base/audio_parameters.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"

namespace content {

// MediaStreamTrack is a Chrome representation of blink::WebMediaStreamTrack.
// It is owned by blink::WebMediaStreamTrack as
// blink::WebMediaStreamTrack::ExtraData.
class CONTENT_EXPORT MediaStreamTrack
    : public blink::WebMediaStreamTrack::TrackData {
 public:
  explicit MediaStreamTrack(bool is_local_track);
  ~MediaStreamTrack() override;

  static MediaStreamTrack* GetTrack(const blink::WebMediaStreamTrack& track);

  virtual void SetEnabled(bool enabled) = 0;

  virtual void SetContentHint(
      blink::WebMediaStreamTrack::ContentHintType content_hint) = 0;

  // If |callback| is not null, it is invoked when the track has stopped.
  virtual void StopAndNotify(base::OnceClosure callback) = 0;

  void Stop() { StopAndNotify(base::OnceClosure()); }

  // TODO(hta): Make method pure virtual when all tracks have the method.
  void GetSettings(blink::WebMediaStreamTrack::Settings& settings) override {}

  bool is_local_track() const { return is_local_track_; }

 protected:
  const bool is_local_track_;

  // Used to DCHECK that we are called on Render main Thread.
  base::ThreadChecker main_render_thread_checker_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaStreamTrack);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_TRACK_H_
