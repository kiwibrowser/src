// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/mock_media_session_observer.h"

namespace content {

MockMediaSessionObserver::MockMediaSessionObserver(MediaSession* media_session)
    : MediaSessionObserver(media_session) {}

MockMediaSessionObserver::~MockMediaSessionObserver() = default;

}  // content
