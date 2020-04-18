// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/media_session_observer.h"

#include "content/browser/media/session/media_session_impl.h"

namespace content {

MediaSessionObserver::MediaSessionObserver(MediaSession* media_session)
    : media_session_(media_session) {
  if (media_session_)
    media_session_->AddObserver(this);
}

MediaSessionObserver::~MediaSessionObserver() {
  StopObserving();
}

MediaSession* MediaSessionObserver::media_session() const {
  return media_session_;
}

void MediaSessionObserver::StopObserving() {
  if (media_session_)
    media_session_->RemoveObserver(this);
  media_session_ = nullptr;
}

}  // namespace content
