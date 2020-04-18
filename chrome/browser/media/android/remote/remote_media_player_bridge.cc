// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/android/remote/remote_media_player_bridge.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/media/android/remote/remote_media_player_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "jni/RemoteMediaPlayerBridge_jni.h"
#include "media/base/android/media_common_android.h"
#include "media/base/android/media_resource_getter.h"
#include "media/base/timestamp_constants.h"
#include "net/base/escape.h"
#include "third_party/blink/public/platform/modules/remoteplayback/web_remote_playback_availability.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/android/java_bitmap.h"

using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertJavaStringToUTF8;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using base::android::AttachCurrentThread;
using content::BrowserThread;
using media::MediaPlayerAndroid;

namespace remote_media {

RemoteMediaPlayerBridge::RemoteMediaPlayerBridge(
    int player_id,
    const std::string& user_agent,
    RemoteMediaPlayerManager* manager)
    : MediaPlayerAndroid(player_id,
                         manager,
                         base::DoNothing(),
                         manager->GetLocalPlayer(player_id)->frame_url()),
      width_(0),
      height_(0),
      url_(manager->GetLocalPlayer(player_id)->GetUrl()),
      site_for_cookies_(
          manager->GetLocalPlayer(player_id)->GetSiteForCookies()),
      user_agent_(user_agent),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);
  ScopedJavaLocalRef<jstring> j_url_string;
  if (url_.is_valid()) {
    // Escape the URL to make it safe to use. Don't escape existing escape
    // sequences though.
    std::string escaped_url = net::EscapeExternalHandlerValue(url_.spec());
    // Create a Java String for the URL.
    j_url_string = ConvertUTF8ToJavaString(env, escaped_url);
  }
  ScopedJavaLocalRef<jstring> j_frame_url_string;
  GURL frameUrl = GetLocalPlayer()->frame_url();
  if (frameUrl.is_valid()) {
    // Create a Java String for the URL.
    j_frame_url_string = ConvertUTF8ToJavaString(env, frameUrl.spec());
  }
  java_bridge_.Reset(Java_RemoteMediaPlayerBridge_create(
      env, reinterpret_cast<intptr_t>(this), j_url_string, j_frame_url_string,
      ConvertUTF8ToJavaString(env, user_agent)));
}

RemoteMediaPlayerBridge::~RemoteMediaPlayerBridge() {
  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);
  Java_RemoteMediaPlayerBridge_destroy(env, java_bridge_);
  Release();
}

bool RemoteMediaPlayerBridge::HasVideo() const {
  NOTIMPLEMENTED();
  return true;
}

bool RemoteMediaPlayerBridge::HasAudio() const {
  NOTIMPLEMENTED();
  return true;
}

int RemoteMediaPlayerBridge::GetVideoWidth() {
  MediaPlayerAndroid* local_player = GetLocalPlayer();
  if (!local_player)
    return 0;
  return local_player->GetVideoWidth();
}

int RemoteMediaPlayerBridge::GetVideoHeight() {
  MediaPlayerAndroid* local_player = GetLocalPlayer();
  if (!local_player)
    return 0;
  return local_player->GetVideoHeight();
}

void RemoteMediaPlayerBridge::OnVideoSizeChanged(int width, int height) {
  width_ = width;
  height_ = height;
  MediaPlayerAndroid::OnVideoSizeChanged(width, height);
}

void RemoteMediaPlayerBridge::OnPlaybackComplete() {
  time_update_timer_.Stop();
  MediaPlayerAndroid::OnPlaybackComplete();
}

void RemoteMediaPlayerBridge::OnMediaInterrupted() {}

void RemoteMediaPlayerBridge::StartInternal() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  Java_RemoteMediaPlayerBridge_start(env, java_bridge_);
  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(media::kTimeUpdateInterval),
        this, &RemoteMediaPlayerBridge::OnTimeUpdateTimerFired);
  }
}

void RemoteMediaPlayerBridge::PauseInternal() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  Java_RemoteMediaPlayerBridge_pause(env, java_bridge_);
  time_update_timer_.Stop();
}

void RemoteMediaPlayerBridge::OnTimeUpdateTimerFired() {
  manager()->OnTimeUpdate(
      player_id(), GetCurrentTime(), base::TimeTicks::Now());
}

void RemoteMediaPlayerBridge::PauseLocal(JNIEnv* env,
                                         const JavaParamRef<jobject>& obj) {
  MediaPlayerAndroid* local_player = GetLocalPlayer();
  if (!local_player)
    return;
  local_player->Pause(true);
  static_cast<RemoteMediaPlayerManager*>(manager())->OnPaused(player_id());
}

jint RemoteMediaPlayerBridge::GetLocalPosition(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  MediaPlayerAndroid* local_player = GetLocalPlayer();
  if (!local_player)
    return 0;
  base::TimeDelta time = local_player->GetCurrentTime();
  return static_cast<jint>(time.InMilliseconds());
}

void RemoteMediaPlayerBridge::OnCastStarting(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& casting_message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  static_cast<RemoteMediaPlayerManager*>(manager())->SwitchToRemotePlayer(
      player_id(), ConvertJavaStringToUTF8(env, casting_message));
  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(media::kTimeUpdateInterval), this,
        &RemoteMediaPlayerBridge::OnTimeUpdateTimerFired);
  }
}

void RemoteMediaPlayerBridge::OnCastStarted(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  static_cast<RemoteMediaPlayerManager*>(manager())->OnRemotePlaybackStarted(
      player_id());
}

void RemoteMediaPlayerBridge::OnCastStopping(JNIEnv* env,
                                             const JavaParamRef<jobject>& obj) {
  static_cast<RemoteMediaPlayerManager*>(manager())
      ->SwitchToLocalPlayer(player_id());
}

void RemoteMediaPlayerBridge::OnSeekCompleted(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  OnSeekComplete();
}

void RemoteMediaPlayerBridge::Pause(bool is_media_related_action) {
  // Ignore the pause if it's not from an event that is explicitly telling
  // the video to pause. It's possible for Pause() to be called for other
  // reasons, such as freeing resources, etc. and during those times, the
  // remote video playback should not be paused.
  if (is_media_related_action) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    JNIEnv* env = AttachCurrentThread();
    Java_RemoteMediaPlayerBridge_pause(env, java_bridge_);
    time_update_timer_.Stop();
  }
}

void RemoteMediaPlayerBridge::SetVideoSurface(gl::ScopedJavaSurface surface) {
  // The surface is reset whenever the fullscreen view is destroyed or created.
  // Since the remote player doesn't use it, we forward it to the local player
  // for the time when user disconnects and resumes local playback
  // (see crbug.com/420690).
  MediaPlayerAndroid* local_player = GetLocalPlayer();
  if (!local_player)
    return;
  local_player->SetVideoSurface(std::move(surface));
}

void RemoteMediaPlayerBridge::OnPlaying(JNIEnv* env,
                                        const JavaParamRef<jobject>& obj) {
  static_cast<RemoteMediaPlayerManager *>(manager())->OnPlaying(player_id());
}

void RemoteMediaPlayerBridge::OnPaused(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  static_cast<RemoteMediaPlayerManager *>(manager())->OnPaused(player_id());
}

void RemoteMediaPlayerBridge::OnRouteUnselected(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  casting_message_.reset();
  static_cast<RemoteMediaPlayerManager *>(manager())->OnRemoteDeviceUnselected(
      player_id());
}

void RemoteMediaPlayerBridge::OnPlaybackFinished(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  static_cast<RemoteMediaPlayerManager *>(manager())->OnRemotePlaybackFinished(
      player_id());
}

void RemoteMediaPlayerBridge::OnRouteAvailabilityChanged(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    int availability) {
  static_cast<RemoteMediaPlayerManager *>(manager())->
      OnRouteAvailabilityChanged(
          player_id(),
          static_cast<blink::WebRemotePlaybackAvailability>(availability));
}

void RemoteMediaPlayerBridge::RequestRemotePlayback() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  MediaPlayerAndroid* local_player = GetLocalPlayer();
  if (!local_player)
    return;
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  Java_RemoteMediaPlayerBridge_requestRemotePlayback(
      env, java_bridge_, local_player->GetCurrentTime().InMilliseconds());
}

void RemoteMediaPlayerBridge::RequestRemotePlaybackControl() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  Java_RemoteMediaPlayerBridge_requestRemotePlaybackControl(env, java_bridge_);
}

void RemoteMediaPlayerBridge::RequestRemotePlaybackStop() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  Java_RemoteMediaPlayerBridge_requestRemotePlaybackStop(env, java_bridge_);
}

void RemoteMediaPlayerBridge::SetNativePlayer() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  Java_RemoteMediaPlayerBridge_setNativePlayer(env, java_bridge_);
}

void RemoteMediaPlayerBridge::OnPlayerCreated() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  Java_RemoteMediaPlayerBridge_onPlayerCreated(env, java_bridge_);
}

void RemoteMediaPlayerBridge::OnPlayerDestroyed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  Java_RemoteMediaPlayerBridge_onPlayerDestroyed(env, java_bridge_);
}

std::string RemoteMediaPlayerBridge::GetCastingMessage() {
  return casting_message_ ?
      *casting_message_ : std::string();
}

void RemoteMediaPlayerBridge::SetPosterBitmap(
    const std::vector<SkBitmap>& bitmaps) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  if (bitmaps.empty()) {
    Java_RemoteMediaPlayerBridge_setPosterBitmap(env, java_bridge_, nullptr);
  } else {
    ScopedJavaLocalRef<jobject> j_poster_bitmap;
    j_poster_bitmap = gfx::ConvertToJavaBitmap(&(bitmaps[0]));

    Java_RemoteMediaPlayerBridge_setPosterBitmap(env, java_bridge_,
                                                 j_poster_bitmap);
  }
}

void RemoteMediaPlayerBridge::Start() {
  StartInternal();
}

void RemoteMediaPlayerBridge::SeekTo(base::TimeDelta time) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO(aberent) Move the checks to the Java side.
  base::TimeDelta duration = GetDuration();

  if (time > duration)
    time = duration;

  // Seeking to an invalid position may cause media player to stuck in an
  // error state.
  if (time < base::TimeDelta()) {
    DCHECK_EQ(-1.0, time.InMillisecondsF());
    return;
  }

  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);
  int time_msec = static_cast<int>(time.InMilliseconds());
  Java_RemoteMediaPlayerBridge_seekTo(env, java_bridge_, time_msec);
}

void RemoteMediaPlayerBridge::Release() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  time_update_timer_.Stop();
  JNIEnv* env = AttachCurrentThread();
  Java_RemoteMediaPlayerBridge_release(env, java_bridge_);
  DetachListener();
}

void RemoteMediaPlayerBridge::UpdateEffectiveVolumeInternal(
    double effective_volume) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);
  Java_RemoteMediaPlayerBridge_setVolume(env, java_bridge_,
                                         GetEffectiveVolume());
}

base::TimeDelta RemoteMediaPlayerBridge::GetCurrentTime() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  return base::TimeDelta::FromMilliseconds(
      Java_RemoteMediaPlayerBridge_getCurrentPosition(env, java_bridge_));
}

base::TimeDelta RemoteMediaPlayerBridge::GetDuration() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  const int duration_ms =
      Java_RemoteMediaPlayerBridge_getDuration(env, java_bridge_);
  // Sometimes we can't get the duration remotely, but the local media player
  // knows it.
  // TODO (aberent) This is for YouTube. Remove it when the YouTube receiver is
  // fixed.
  if (duration_ms == 0) {
    MediaPlayerAndroid* local_player = GetLocalPlayer();
    if (!local_player)
      return media::kInfiniteDuration;
    return local_player->GetDuration();
  }
  return duration_ms < 0 ? media::kInfiniteDuration
                         : base::TimeDelta::FromMilliseconds(duration_ms);
}

bool RemoteMediaPlayerBridge::IsPlaying() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);
  jboolean result = Java_RemoteMediaPlayerBridge_isPlaying(env, java_bridge_);
  return result;
}

bool RemoteMediaPlayerBridge::CanPause() {
  return true;
}

bool RemoteMediaPlayerBridge::CanSeekForward() {
  return true;
}

bool RemoteMediaPlayerBridge::CanSeekBackward() {
  return true;
}

bool RemoteMediaPlayerBridge::IsPlayerReady() {
  return true;
}

GURL RemoteMediaPlayerBridge::GetUrl() {
  return url_;
}

GURL RemoteMediaPlayerBridge::GetSiteForCookies() {
  return site_for_cookies_;
}

void RemoteMediaPlayerBridge::Initialize() {
  cookies_.clear();
  media::MediaResourceGetter* resource_getter =
      manager()->GetMediaResourceGetter();
  resource_getter->GetCookies(
      url_, site_for_cookies_,
      base::Bind(&RemoteMediaPlayerBridge::OnCookiesRetrieved,
                 weak_factory_.GetWeakPtr()));
}

base::android::ScopedJavaLocalRef<jstring> RemoteMediaPlayerBridge::GetTitle(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  base::string16 title;
  auto* contents =
      static_cast<RemoteMediaPlayerManager*>(manager())->web_contents();
  if (contents)
    title = contents->GetTitle();
  return base::android::ConvertUTF16ToJavaString(env, title);
}

void RemoteMediaPlayerBridge::OnError(
    JNIEnv* env, const base::android::JavaParamRef<jobject>& obj) {
      // TODO(https://crbug.com/585379) implement some useful codes for remote
      // playback. None of the existing MediaPlayerAndroid codes are
      // relevant for remote playback.
      manager()->OnError(player_id(), MEDIA_ERROR_INVALID_CODE);
}

void RemoteMediaPlayerBridge::OnCancelledRemotePlaybackRequest(
    JNIEnv* env, const base::android::JavaParamRef<jobject>& obj) {
  static_cast<RemoteMediaPlayerManager*>(manager())
      ->OnCancelledRemotePlaybackRequest(player_id());
}


void RemoteMediaPlayerBridge::OnCookiesRetrieved(const std::string& cookies) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO(aberent) Do we need to retrieve auth credentials for basic
  // authentication? MediaPlayerBridge does.
  cookies_ = cookies;
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);
  Java_RemoteMediaPlayerBridge_setCookies(
      env, java_bridge_, ConvertUTF8ToJavaString(env, cookies));
}

MediaPlayerAndroid* RemoteMediaPlayerBridge::GetLocalPlayer() {
  return static_cast<RemoteMediaPlayerManager*>(manager())->GetLocalPlayer(
      player_id());
}

}  // namespace remote_media
