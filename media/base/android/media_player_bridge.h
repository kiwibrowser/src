// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_MEDIA_PLAYER_BRIDGE_H_
#define MEDIA_BASE_ANDROID_MEDIA_PLAYER_BRIDGE_H_

#include <jni.h>
#include <stdint.h>

#include <map>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "media/base/android/media_player_android.h"
#include "url/gurl.h"

namespace media {

class MediaPlayerManager;

// This class serves as a bridge between the native code and Android MediaPlayer
// Java class. For more information on Android MediaPlayer, check
// http://developer.android.com/reference/android/media/MediaPlayer.html
// The actual Android MediaPlayer instance is created lazily when Start(),
// Pause(), SeekTo() gets called. As a result, media information may not
// be available until one of those operations is performed. After that, we
// will cache those information in case the mediaplayer gets released.
// The class uses the corresponding MediaPlayerBridge Java class to talk to
// the Android MediaPlayer instance.
class MEDIA_EXPORT MediaPlayerBridge : public MediaPlayerAndroid {
 public:
  // Construct a MediaPlayerBridge object. This object needs to call |manager|'s
  // RequestMediaResources() before decoding the media stream. This allows
  // |manager| to track unused resources and free them when needed.
  // MediaPlayerBridge also forwards Android MediaPlayer callbacks to
  // the |manager| when needed.
  MediaPlayerBridge(
      int player_id,
      const GURL& url,
      const GURL& site_for_cookies,
      const std::string& user_agent,
      bool hide_url_log,
      MediaPlayerManager* manager,
      const OnDecoderResourcesReleasedCB& on_decoder_resources_released_cb,
      const GURL& frame_url,
      bool allow_credentials);
  ~MediaPlayerBridge() override;

  // Initialize this object and extract the metadata from the media.
  virtual void Initialize();

  // MediaPlayerAndroid implementation.
  void SetVideoSurface(gl::ScopedJavaSurface surface) override;
  void Start() override;
  void Pause(bool is_media_related_action) override;
  void SeekTo(base::TimeDelta timestamp) override;
  void Release() override;
  bool HasVideo() const override;
  bool HasAudio() const override;
  int GetVideoWidth() override;
  int GetVideoHeight() override;
  base::TimeDelta GetCurrentTime() override;
  base::TimeDelta GetDuration() override;
  bool IsPlaying() override;
  bool CanPause() override;
  bool CanSeekForward() override;
  bool CanSeekBackward() override;
  bool IsPlayerReady() override;
  GURL GetUrl() override;
  GURL GetSiteForCookies() override;

  void OnDidSetDataUriDataSource(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jboolean success);

 protected:
  void SetDuration(base::TimeDelta time);

  virtual void PendingSeekInternal(const base::TimeDelta& time);

  // Prepare the player for playback, asynchronously. When succeeds,
  // OnMediaPrepared() will be called. Otherwise, OnMediaError() will
  // be called with an error type.
  virtual void Prepare();

  // MediaPlayerAndroid implementation.
  void OnVideoSizeChanged(int width, int height) override;
  void OnMediaError(int error_type) override;
  void OnPlaybackComplete() override;
  void OnMediaInterrupted() override;
  void OnMediaPrepared() override;

  // Create the corresponding Java class instance.
  virtual void CreateJavaMediaPlayerBridge();

  // Get allowed operations from the player.
  virtual base::android::ScopedJavaLocalRef<jobject> GetAllowedOperations();

 private:
  friend class MediaPlayerBridgeTest;

  // MediaPlayerAndroid implementation
  void UpdateEffectiveVolumeInternal(double effective_volume) override;

  // Set the data source for the media player.
  void SetDataSource(const std::string& url);

  // Functions that implements media player control.
  void StartInternal();
  void PauseInternal();

  // Returns true if the Java MediaPlayerBridge's seekTo method is called
  bool SeekInternal(base::TimeDelta current_time, base::TimeDelta time);

  // Called when |time_update_timer_| fires.
  void OnTimeUpdateTimerFired();

  // Update allowed operations from the player.
  void UpdateAllowedOperations();

  // Callback function passed to |resource_getter_|. Called when the cookies
  // are retrieved.
  void OnCookiesRetrieved(const std::string& cookies);

  // Callback function passed to |resource_getter_|. Called when the auth
  // credentials are retrieved.
  void OnAuthCredentialsRetrieved(
      const base::string16& username, const base::string16& password);

  // Extract the media metadata from a url, asynchronously.
  // OnMediaMetadataExtracted() will be called when this call finishes.
  void ExtractMediaMetadata(const std::string& url);
  void OnMediaMetadataExtracted(base::TimeDelta duration, int width, int height,
                                bool success);

  // Returns true if a MediaUrlInterceptor registered by the embedder has
  // intercepted the url.
  bool InterceptMediaUrl(const std::string& url,
                         int* fd,
                         int64_t* offset,
                         int64_t* size);

  // Whether the player is prepared for playback.
  bool prepared_;

  // Pending play event while player is preparing.
  bool pending_play_;

  // Pending seek time while player is preparing.
  base::TimeDelta pending_seek_;

  // Whether a seek should be performed after preparing.
  bool should_seek_on_prepare_;

  // Url for playback.
  GURL url_;

  // First party url for cookies.
  GURL site_for_cookies_;

  // User agent string to be used for media player.
  const std::string user_agent_;

  // Hide url log from media player.
  bool hide_url_log_;

  // Stats about the media.
  base::TimeDelta duration_;
  int width_;
  int height_;

  // Meta data about actions can be taken.
  bool can_pause_;
  bool can_seek_forward_;
  bool can_seek_backward_;

  // Cookies for |url_|.
  std::string cookies_;

  // The surface object currently owned by the player.
  gl::ScopedJavaSurface surface_;

  // Java MediaPlayerBridge instance.
  base::android::ScopedJavaGlobalRef<jobject> j_media_player_bridge_;

  base::RepeatingTimer time_update_timer_;

  base::TimeDelta last_time_update_timestamp_;

  // Whether user credentials are allowed to be passed.
  bool allow_credentials_;

  // Helper variables for UMA reporting.

  // Whether the preparation for playback or the playback is currently going on.
  // This flag is set in Start() and cleared in Pause() and Release(). Used for
  // UMA reporting only.
  bool is_active_;

  // Whether there has been any errors in the active state.
  bool has_error_;

  // The flag is set if Start() has been called at least once.
  bool has_ever_started_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MediaPlayerBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerBridge);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_MEDIA_PLAYER_BRIDGE_H_
