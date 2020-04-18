// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_MEDIA_PLAYER_ANDROID_H_
#define MEDIA_BASE_ANDROID_MEDIA_PLAYER_ANDROID_H_

#include <jni.h>

#include <memory>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "media/base/android/media_player_listener.h"
#include "media/base/media_export.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/android/scoped_java_surface.h"
#include "url/gurl.h"

namespace media {

class ContentDecryptionModule;
class MediaPlayerManager;

// This class serves as the base class for different media player
// implementations on Android. Subclasses need to provide their own
// MediaPlayerAndroid::Create() implementation.
class MEDIA_EXPORT MediaPlayerAndroid {
 public:
  virtual ~MediaPlayerAndroid();

  // Error types for MediaErrorCB.
  enum MediaErrorType {
    MEDIA_ERROR_FORMAT,
    MEDIA_ERROR_DECODE,
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK,
    MEDIA_ERROR_INVALID_CODE,
    MEDIA_ERROR_SERVER_DIED,
  };

  static const double kDefaultVolumeMultiplier;

  // Callback when the player releases decoding resources.
  typedef base::Callback<void(int player_id)> OnDecoderResourcesReleasedCB;

  // Virtual destructor.
  // For most subclasses we can delete on the caller thread.
  virtual void DeleteOnCorrectThread();

  // Passing an external java surface object to the player.
  virtual void SetVideoSurface(gl::ScopedJavaSurface surface) = 0;

  // Start playing the media.
  virtual void Start() = 0;

  // Pause the media.
  virtual void Pause(bool is_media_related_action) = 0;

  // Seek to a particular position, based on renderer signaling actual seek
  // with MediaPlayerHostMsg_Seek. If eventual success, OnSeekComplete() will be
  // called.
  virtual void SeekTo(base::TimeDelta timestamp) = 0;

  // Release the player resources.
  virtual void Release() = 0;

  // Set the player volume, and take effect immediately.
  // The volume should be between 0.0 and 1.0.
  void SetVolume(double volume);

  // Set the player volume multiplier, and take effect immediately.
  // The volume should be between 0.0 and 1.0.
  void SetVolumeMultiplier(double volume_multiplier);

  // Get the media information from the player.
  virtual bool HasVideo() const = 0;
  virtual bool HasAudio() const = 0;
  virtual int GetVideoWidth() = 0;
  virtual int GetVideoHeight() = 0;
  virtual base::TimeDelta GetDuration() = 0;
  virtual base::TimeDelta GetCurrentTime() = 0;
  virtual bool IsPlaying() = 0;
  virtual bool CanPause() = 0;
  virtual bool CanSeekForward() = 0;
  virtual bool CanSeekBackward() = 0;
  virtual bool IsPlayerReady() = 0;
  virtual GURL GetUrl();
  virtual GURL GetSiteForCookies();

  // Associates the |cdm| with this player.
  virtual void SetCdm(const scoped_refptr<ContentDecryptionModule>& cdm);

  // Requests playback permission from MediaPlayerManager.
  // Overridden in MediaCodecPlayer to pass data between threads.
  virtual void RequestPermissionAndPostResult(base::TimeDelta duration,
                                              bool has_audio) {}

  // Overridden in MediaCodecPlayer to pass data between threads.
  virtual void OnMediaMetadataChanged(base::TimeDelta duration,
                                      const gfx::Size& video_size) {}

  // Overridden in MediaCodecPlayer to pass data between threads.
  virtual void OnTimeUpdate(base::TimeDelta current_timestamp,
                            base::TimeTicks current_time_ticks) {}

  int player_id() { return player_id_; }

  GURL frame_url() { return frame_url_; }

  // Attach/Detaches |listener_| for listening to all the media events. If
  // |j_media_player| is NULL, |listener_| only listens to the system media
  // events. Otherwise, it also listens to the events from |j_media_player|.
  void AttachListener(const base::android::JavaRef<jobject>& j_media_player);
  void DetachListener();

 protected:
  MediaPlayerAndroid(
      int player_id,
      MediaPlayerManager* manager,
      const OnDecoderResourcesReleasedCB& on_decoder_resources_released_cb,
      const GURL& frame_url);

  // TODO(qinmin): Simplify the MediaPlayerListener class to only listen to
  // media interrupt events. And have a separate child class to listen to all
  // the events needed by MediaPlayerBridge. http://crbug.com/422597.
  // MediaPlayerListener callbacks.
  virtual void OnVideoSizeChanged(int width, int height);
  virtual void OnMediaError(int error_type);
  virtual void OnBufferingUpdate(int percent);
  virtual void OnPlaybackComplete();
  virtual void OnMediaInterrupted();
  virtual void OnSeekComplete();
  virtual void OnMediaPrepared();

  double GetEffectiveVolume() const;
  void UpdateEffectiveVolume();

  // When destroying a subclassed object on a non-UI thread
  // it is still required to destroy the |listener_| related stuff
  // on the UI thread.
  void DestroyListenerOnUIThread();

  MediaPlayerManager* manager() { return manager_; }

  base::WeakPtr<MediaPlayerAndroid> WeakPtrForUIThread();

  OnDecoderResourcesReleasedCB on_decoder_resources_released_cb_;

 private:
  // Set the effective player volume, implemented by children classes.
  virtual void UpdateEffectiveVolumeInternal(double effective_volume) = 0;

  friend class MediaPlayerListener;

  // Player ID assigned to this player.
  int player_id_;

  // The player volume. Should be between 0.0 and 1.0.
  double volume_;

  // The player volume multiplier. Should be between 0.0 and 1.0.  This
  // should be a cached version of the MediaSession volume multiplier,
  // and should keep updated.
  double volume_multiplier_;

  // Resource manager for all the media players.
  MediaPlayerManager* manager_;

  // Url for the frame that contains this player.
  GURL frame_url_;

  // Listener object that listens to all the media player events.
  std::unique_ptr<MediaPlayerListener> listener_;

  // Weak pointer passed to |listener_| for callbacks.
  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MediaPlayerAndroid> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerAndroid);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_MEDIA_PLAYER_ANDROID_H_
