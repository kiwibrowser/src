// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_ANDROID_BROWSER_MEDIA_PLAYER_MANAGER_H_
#define CONTENT_BROWSER_MEDIA_ANDROID_BROWSER_MEDIA_PLAYER_MANAGER_H_

#include <map>
#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "content/browser/android/content_video_view.h"
#include "content/common/content_export.h"
#include "ipc/ipc_message.h"
#include "media/base/android/media_player_android.h"
#include "media/base/android/media_player_manager.h"
#include "media/base/android/media_url_interceptor.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gl/android/scoped_java_surface.h"
#include "url/gurl.h"

struct MediaPlayerHostMsg_Initialize_Params;

namespace content {
#if !defined(USE_AURA)
class ContentViewCore;
#endif
class RenderFrameHost;
class WebContents;

// This class manages all the MediaPlayerAndroid objects.
// It receives control operations from the the render process, and forwards
// them to corresponding MediaPlayerAndroid object. Callbacks from
// MediaPlayerAndroid objects are converted to IPCs and then sent to the render
// process.
class CONTENT_EXPORT BrowserMediaPlayerManager
    : public media::MediaPlayerManager,
      public ContentVideoView::Client {
 public:
  // Permits embedders to provide an extended version of the class.
  typedef BrowserMediaPlayerManager* (*Factory)(RenderFrameHost*);
  static void RegisterFactory(Factory factory);

  // Permits embedders to handle custom urls.
  static void RegisterMediaUrlInterceptor(
      media::MediaUrlInterceptor* media_url_interceptor);

  // Returns a new instance using the registered factory.
  // Returns nullptr if no factory was registered.
  static BrowserMediaPlayerManager* Create(RenderFrameHost* rfh);

#if !defined(USE_AURA)
  ContentViewCore* GetContentViewCore() const;
#endif

  ~BrowserMediaPlayerManager() override;

  // ContentVideoView::Client implementation.
  void DidExitFullscreen(bool release_media_player) override;
  void SetVideoSurface(gl::ScopedJavaSurface surface) override;

  // Called when browser player wants the renderer media element to seek.
  // Any actual seek started by renderer will be handled by browser in OnSeek().
  void OnSeekRequest(int player_id, const base::TimeDelta& time_to_seek);

  WebContents* web_contents() const { return web_contents_; }

  // media::MediaPlayerManager overrides.
  void OnTimeUpdate(int player_id,
                    base::TimeDelta current_timestamp,
                    base::TimeTicks current_time_ticks) override;
  void OnMediaMetadataChanged(int player_id,
                              base::TimeDelta duration,
                              int width,
                              int height,
                              bool success) override;
  void OnPlaybackComplete(int player_id) override;
  void OnMediaInterrupted(int player_id) override;
  void OnBufferingUpdate(int player_id, int percentage) override;
  void OnSeekComplete(int player_id,
                      const base::TimeDelta& current_time) override;
  void OnError(int player_id, int error) override;
  void OnVideoSizeChanged(int player_id, int width, int height) override;

  media::MediaResourceGetter* GetMediaResourceGetter() override;
  media::MediaUrlInterceptor* GetMediaUrlInterceptor() override;
  media::MediaPlayerAndroid* GetFullscreenPlayer() override;
  media::MediaPlayerAndroid* GetPlayer(int player_id) override;
  bool RequestPlay(int player_id, base::TimeDelta duration,
                   bool has_audio) override;

  // Message handlers.
  virtual void OnEnterFullscreen(int player_id);
  virtual void OnInitialize(
      const MediaPlayerHostMsg_Initialize_Params& media_player_params);
  virtual void OnStart(int player_id);
  virtual void OnSeek(int player_id, const base::TimeDelta& time);
  virtual void OnPause(int player_id, bool is_media_related_action);
  virtual void OnSetVolume(int player_id, double volume);
  virtual void OnSetPoster(int player_id, const GURL& poster);
  virtual void OnSuspendAndReleaseResources(int player_id);
  virtual void OnDestroyPlayer(int player_id);
  virtual void OnRequestRemotePlayback(int player_id);
  virtual void OnRequestRemotePlaybackControl(int player_id);
  virtual void OnRequestRemotePlaybackStop(int player_id);
  virtual bool IsPlayingRemotely(int player_id);
  virtual void ReleaseFullscreenPlayer(media::MediaPlayerAndroid* player);

 protected:
  // Clients must use Create() or subclass constructor.
  explicit BrowserMediaPlayerManager(RenderFrameHost* render_frame_host);

  // Adds a given player to the list.  Not private to allow embedders
  // to extend the manager and still utilize the base player management.
  void AddPlayer(std::unique_ptr<media::MediaPlayerAndroid> player,
                 int delegate_id);

  // Removes the player with the specified id.
  void DestroyPlayer(int player_id);

  // Release resources associated to a player.
  virtual void ReleaseResources(int player_id);

  // Replaces a player with the specified id with a given MediaPlayerAndroid
  // object. This will also return the original MediaPlayerAndroid object that
  // was replaced.
  std::unique_ptr<media::MediaPlayerAndroid> SwapPlayer(
      int player_id,
      std::unique_ptr<media::MediaPlayerAndroid> player);

  // Called to request decoder resources. Returns true if the request is
  // permitted, or false otherwise. The manager object maintains a list
  // of active MediaPlayerAndroid objects and releases the inactive resources
  // when needed. If |temporary| is true, the request is short lived
  // and it will not be cleaned up when handling other requests.
  // On the contrary, requests with false |temporary| value are subject to
  // clean up if their players are idle.
  virtual bool RequestDecoderResources(int player_id, bool temporary);

  // MediaPlayerAndroid must call this to inform the manager that it has
  // released the decoder resources. This can be triggered by the
  // ReleasePlayer() call below, or when meta data is extracted, or when player
  // is stuck in an error.
  virtual void OnDecoderResourcesReleased(int player_id);

  int RoutingID();

  // Helper function to send messages to RenderFrameObserver.
  bool Send(IPC::Message* msg);

 private:
  // Constructs a MediaPlayerAndroid object.
  std::unique_ptr<media::MediaPlayerAndroid> CreateMediaPlayer(
      const MediaPlayerHostMsg_Initialize_Params& media_player_params,
      bool hide_url_log);

  // Instructs |player| to release its java player. This will not remove the
  // player from |players_|.
  void ReleasePlayer(media::MediaPlayerAndroid* player);

  RenderFrameHost* const render_frame_host_;

  // An array of managed players.
  std::vector<std::unique_ptr<media::MediaPlayerAndroid>> players_;

  typedef std::map<int, bool> ActivePlayerMap;
  // Players that have requested decoding resources. Even though resource is
  // requested, a player may be in a paused or error state and the manager
  // will release its resources later.
  ActivePlayerMap active_players_;

  // The fullscreen video view object or NULL if video is not played in
  // fullscreen.
  std::unique_ptr<ContentVideoView> video_view_;

  // Player ID of the fullscreen media player.
  int fullscreen_player_id_;

  // Whether the fullscreen player has been Release()-d.
  bool fullscreen_player_is_released_;

  WebContents* const web_contents_;

  // Object for retrieving resources media players.
  std::unique_ptr<media::MediaResourceGetter> media_resource_getter_;

  // Map of player IDs to delegate IDs for use with
  // MediaWebContentsObserverAndroid.
  std::map<int, int> player_id_to_delegate_id_map_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<BrowserMediaPlayerManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BrowserMediaPlayerManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_ANDROID_BROWSER_MEDIA_PLAYER_MANAGER_H_
