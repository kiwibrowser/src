// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_WEB_CONTENTS_OBSERVER_ANDROID_H_
#define CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_WEB_CONTENTS_OBSERVER_ANDROID_H_

#include <stdint.h>

#include <memory>
#include <unordered_map>

#include "base/macros.h"
#include "content/browser/media/media_web_contents_observer.h"
#include "content/common/content_export.h"

namespace media {
enum class MediaContentType;
}  // namespace media

namespace content {

class BrowserMediaPlayerManager;
class BrowserSurfaceViewManager;

// This class adds Android specific extensions to the MediaWebContentsObserver.
class CONTENT_EXPORT MediaWebContentsObserverAndroid
    : public MediaWebContentsObserver {
 public:
  explicit MediaWebContentsObserverAndroid(WebContents* web_contents);
  ~MediaWebContentsObserverAndroid() override;

  // Returns the android specific observer for a given web contents.
  static MediaWebContentsObserverAndroid* FromWebContents(
      WebContents* web_contents);

  // Gets one of the managers associated with the given |render_frame_host|.
  // Creates a new one if it doesn't exist. The caller doesn't own the
  // returned pointer.
  BrowserMediaPlayerManager* GetMediaPlayerManager(
      RenderFrameHost* render_frame_host);
  BrowserSurfaceViewManager* GetSurfaceViewManager(
      RenderFrameHost* render_frame_host);

  // Called by the WebContents when a tab has been closed but may still be
  // available for "undo" -- indicates that all media players (even audio only
  // players typically allowed background audio) bound to this WebContents must
  // be suspended.
  void SuspendAllMediaPlayers();

  // Initiates a synchronous MediaSession request for browser side players.
  //
  // TODO(dalecurtis): Delete this method once we're no longer using WMPA and
  // the BrowserMediaPlayerManagers.  Tracked by http://crbug.com/580626
  bool RequestPlay(RenderFrameHost* render_frame_host,
                   int delegate_id,
                   bool has_audio,
                   bool is_remote,
                   media::MediaContentType media_content_type);

  void DisconnectMediaSession(RenderFrameHost* render_frame_host,
                              int delegate_id);

  // MediaWebContentsObserver overrides.
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;
  bool OnMessageReceived(const IPC::Message& message,
                         RenderFrameHost* render_frame_host) override;

 private:
  // Helper functions to handle media player IPC messages. Returns whether the
  // |message| is handled in the function.
  bool OnMediaPlayerMessageReceived(const IPC::Message& message,
                                    RenderFrameHost* render_frame_host);

  bool OnSurfaceViewManagerMessageReceived(const IPC::Message& message,
                                     RenderFrameHost* render_frame_host);

  // Map from RenderFrameHost* to BrowserMediaPlayerManager.
  using MediaPlayerManagerMap =
      std::unordered_map<RenderFrameHost*,
                         std::unique_ptr<BrowserMediaPlayerManager>>;
  MediaPlayerManagerMap media_player_managers_;

  using SurfaceViewManagerMap =
      std::unordered_map<RenderFrameHost*,
                         std::unique_ptr<BrowserSurfaceViewManager>>;
  SurfaceViewManagerMap surface_view_managers_;

  DISALLOW_COPY_AND_ASSIGN(MediaWebContentsObserverAndroid);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_WEB_CONTENTS_OBSERVER_ANDROID_H_
