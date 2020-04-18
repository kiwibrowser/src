// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_SESSION_MOCK_MEDIA_SESSION_OBSERVER_H_
#define CONTENT_BROWSER_MEDIA_SESSION_MOCK_MEDIA_SESSION_OBSERVER_H_

#include "content/public/browser/media_session_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace content {

class MockMediaSessionObserver : public MediaSessionObserver {
 public:
  MockMediaSessionObserver(MediaSession* media_session);
  ~MockMediaSessionObserver() override;

  MOCK_METHOD0(MediaSessionDestroyed, void());
  MOCK_METHOD2(MediaSessionStateChanged,
               void(bool is_controllable, bool is_suspended));
  MOCK_METHOD1(MediaSessionMetadataChanged,
               void(const base::Optional<MediaMetadata>& metadata));
  MOCK_METHOD1(MediaSessionActionsChanged,
               void(const std::set<blink::mojom::MediaSessionAction>& action));
};
}

#endif  // CONTENT_BROWSER_MEDIA_SESSION_MOCK_MEDIA_SESSION_OBSERVER_H_
