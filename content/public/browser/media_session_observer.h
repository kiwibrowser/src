// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_MEDIA_SESSION_OBSERVER_H_
#define CONTENT_PUBLIC_BROWSER_MEDIA_SESSION_OBSERVER_H_

#include <set>

#include "base/macros.h"
#include "base/optional.h"
#include "content/common/content_export.h"
#include "content/public/common/media_metadata.h"

namespace blink {
namespace mojom {
enum class MediaSessionAction;
}  // namespace mojom
}  // namespace blink

namespace content {

class MediaSession;

// The observer for observing MediaSession events.
class CONTENT_EXPORT MediaSessionObserver {
 public:
  // Gets the observed MediaSession. Will return null when the session is
  // destroyed.
  MediaSession* media_session() const;

  // Called when the observed MediaSession is being destroyed. Give subclass a
  // chance to clean up. media_session() will return nullptr after this method
  // is called.
  virtual void MediaSessionDestroyed() {}

  // Called when the observed MediaSession has changed its state.
  virtual void MediaSessionStateChanged(bool is_controllable,
                                        bool is_suspended) {}

  // Called when the observed MediaSession has changed metadata.
  virtual void MediaSessionMetadataChanged(
      const base::Optional<MediaMetadata>& metadata) {}

  // Called when the media session action list has changed.
  virtual void MediaSessionActionsChanged(
      const std::set<blink::mojom::MediaSessionAction>& action) {}

 protected:
  // Create a MediaSessionObserver and start observing a session.
  explicit MediaSessionObserver(MediaSession* media_session);
  // Destruct a MediaSessionObserver and remove it from the session if it's
  // still observing.
  virtual ~MediaSessionObserver();

 private:
  friend class MediaSessionImpl;

  void StopObserving();

  // Weak pointer to MediaSession
  MediaSession* media_session_;

  DISALLOW_COPY_AND_ASSIGN(MediaSessionObserver);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_MEDIA_SESSION_OBSERVER_H_
