// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_TRACK_OBSERVER_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_TRACK_OBSERVER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "content/common/content_export.h"
#include "third_party/webrtc/api/mediastreaminterface.h"

namespace content {

class CONTENT_EXPORT TrackObserver {
 public:
  typedef base::Callback<void(webrtc::MediaStreamTrackInterface::TrackState)>
      OnChangedCallback;

  TrackObserver(const scoped_refptr<base::SingleThreadTaskRunner>& main_thread,
                const scoped_refptr<webrtc::MediaStreamTrackInterface>& track);
  ~TrackObserver();

  void SetCallback(const OnChangedCallback& callback);

  const scoped_refptr<webrtc::MediaStreamTrackInterface>& track() const;

 private:
  class TrackObserverImpl;
  const scoped_refptr<TrackObserverImpl> observer_;
  DISALLOW_COPY_AND_ASSIGN(TrackObserver);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_TRACK_OBSERVER_H_
