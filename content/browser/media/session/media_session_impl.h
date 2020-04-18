// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_IMPL_H_
#define CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_IMPL_H_

#include <stddef.h>

#include <map>
#include <set>

#include "base/callback_list.h"
#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "content/browser/media/session/audio_focus_manager.h"
#include "content/browser/media/session/media_session_uma_helper.h"
#include "content/common/content_export.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/media_session_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/media_metadata.h"

#if defined(OS_ANDROID)
#include "base/android/scoped_java_ref.h"
#endif  // defined(OS_ANDROID)

class MediaSessionImplBrowserTest;

namespace media {
enum class MediaContentType;
}  // namespace media

namespace content {

class AudioFocusDelegate;
class AudioFocusManagerTest;
class MediaSessionImplServiceRoutingTest;
class MediaSessionImplStateObserver;
class MediaSessionImplVisibilityBrowserTest;
class MediaSessionObserver;
class MediaSessionPlayerObserver;
class MediaSessionServiceImpl;
class MediaSessionServiceImplBrowserTest;

#if defined(OS_ANDROID)
class MediaSessionAndroid;
#endif  // defined(OS_ANDROID)

// MediaSessionImpl is the implementation of MediaSession. It manages the media
// session and audio focus for a given WebContents. It is requesting the audio
// focus, pausing when requested by the system and dropping it on demand. The
// audio focus can be of two types: Transient or Content. A Transient audio
// focus will allow other players to duck instead of pausing and will be
// declared as temporary to the system. A Content audio focus will not be
// declared as temporary and will not allow other players to duck. If a given
// WebContents can only have one audio focus at a time, it will be Content in
// case of Transient and Content audio focus are both requested.
// TODO(thakis,mlamouri): MediaSessionImpl isn't CONTENT_EXPORT'd because it
// creates complicated build issues with WebContentsUserData being a
// non-exported template, see https://crbug.com/589840. As a result, the class
// uses CONTENT_EXPORT for methods that are being used from tests.
// CONTENT_EXPORT should be moved back to the class when the Windows build will
// work with it.
class MediaSessionImpl : public MediaSession,
                         public WebContentsObserver,
                         public WebContentsUserData<MediaSessionImpl> {
 public:
  enum class State { ACTIVE, SUSPENDED, INACTIVE };

  // Returns the MediaSessionImpl associated to this WebContents. Creates one if
  // none is currently available.
  CONTENT_EXPORT static MediaSessionImpl* Get(WebContents* web_contents);

  ~MediaSessionImpl() override;

#if defined(OS_ANDROID)
  static MediaSession* FromJavaMediaSession(
      const base::android::JavaRef<jobject>& j_media_session);
  MediaSessionAndroid* session_android() const {
    return session_android_.get();
  }
#endif  // defined(OS_ANDROID)

  void NotifyMediaSessionMetadataChange(
      const base::Optional<MediaMetadata>& metadata);
  void NotifyMediaSessionActionsChange(
      const std::set<blink::mojom::MediaSessionAction>& actions);

  // Adds the given player to the current media session. Returns whether the
  // player was successfully added. If it returns false, AddPlayer() should be
  // called again later.
  CONTENT_EXPORT bool AddPlayer(MediaSessionPlayerObserver* observer,
                                int player_id,
                                media::MediaContentType media_content_type);

  // Removes the given player from the current media session. Abandons audio
  // focus if that was the last player in the session.
  CONTENT_EXPORT void RemovePlayer(MediaSessionPlayerObserver* observer,
                                   int player_id);

  // Removes all the players associated with |observer|. Abandons audio focus if
  // these were the last players in the session.
  CONTENT_EXPORT void RemovePlayers(MediaSessionPlayerObserver* observer);

  // Record that the session was ducked.
  void RecordSessionDuck();

  // Called when a player is paused in the content.
  // If the paused player is the last player, we suspend the MediaSession.
  // Otherwise, the paused player will be removed from the MediaSession.
  CONTENT_EXPORT void OnPlayerPaused(MediaSessionPlayerObserver* observer,
                                     int player_id);

  // Resume the media session.
  // |type| represents the origin of the request.
  CONTENT_EXPORT void Resume(MediaSession::SuspendType suspend_type) override;

  // Suspend the media session.
  // |type| represents the origin of the request.
  CONTENT_EXPORT void Suspend(MediaSession::SuspendType suspend_type) override;

  // Stop the media session.
  // |type| represents the origin of the request.
  CONTENT_EXPORT void Stop(MediaSession::SuspendType suspend_type) override;

  // Seek the media session forward.
  CONTENT_EXPORT void SeekForward(base::TimeDelta seek_time) override;

  // Seek the media session backward.
  CONTENT_EXPORT void SeekBackward(base::TimeDelta seek_time) override;

  // Returns if the session can be controlled by Resume() and Suspend() calls
  // above.
  CONTENT_EXPORT bool IsControllable() const override;

  // Compute if the actual playback state is paused by combining the
  // MediaSessionService declared state and guessed state (audio_focus_state_).
  CONTENT_EXPORT bool IsActuallyPaused() const override;

  // Set the volume multiplier applied during ducking.
  CONTENT_EXPORT void SetDuckingVolumeMultiplier(double multiplier) override;

  // Let the media session start ducking such that the volume multiplier is
  // reduced.
  CONTENT_EXPORT void StartDucking() override;

  // Let the media session stop ducking such that the volume multiplier is
  // recovered.
  CONTENT_EXPORT void StopDucking() override;

  void AddObserver(MediaSessionObserver* observer) override;
  void RemoveObserver(MediaSessionObserver* observer) override;

  // Returns if the session is currently active.
  CONTENT_EXPORT bool IsActive() const;

  // Returns if the session is currently suspended.
  CONTENT_EXPORT bool IsSuspended() const;

  // Returns the audio focus type. The type is updated everytime after the
  // session requests audio focus.
  CONTENT_EXPORT AudioFocusManager::AudioFocusType audio_focus_type() const {
    return audio_focus_type_;
  }

  // Returns whether the session has Pepper instances.
  bool HasPepper() const;

  // WebContentsObserver implementation
  void WebContentsDestroyed() override;
  void RenderFrameDeleted(RenderFrameHost* rfh) override;
  void DidFinishNavigation(NavigationHandle* navigation_handle) override;

  // MediaSessionService-related methods

  // Called when a MediaSessionService is created, which registers itself to
  // this session.
  void OnServiceCreated(MediaSessionServiceImpl* service);
  // Called when a MediaSessionService is destroyed, which unregisters itself
  // from this session.
  void OnServiceDestroyed(MediaSessionServiceImpl* service);

  // Called when the playback state of a MediaSessionService has
  // changed. Will notify observers of media session state change.
  void OnMediaSessionPlaybackStateChanged(MediaSessionServiceImpl* service);

  // Called when the metadata of a MediaSessionService has changed. Will notify
  // observers if the service is currently routed.
  void OnMediaSessionMetadataChanged(MediaSessionServiceImpl* service);
  // Called when the actions of a MediaSessionService has changed. Will notify
  // observers if the service is currently routed.
  void OnMediaSessionActionsChanged(MediaSessionServiceImpl* service);

  // Called when a MediaSessionAction is received. The action will be forwarded
  // to blink::MediaSession corresponding to the current routed service.
  void DidReceiveAction(blink::mojom::MediaSessionAction action) override;

  // Requests audio focus to the AudioFocusDelegate.
  // Returns whether the request was granted.
  CONTENT_EXPORT bool RequestSystemAudioFocus(
      AudioFocusManager::AudioFocusType audio_focus_type);

 private:
  friend class content::WebContentsUserData<MediaSessionImpl>;
  friend class ::MediaSessionImplBrowserTest;
  friend class content::MediaSessionImplVisibilityBrowserTest;
  friend class content::AudioFocusManagerTest;
  friend class content::MediaSessionImplServiceRoutingTest;
  friend class content::MediaSessionImplStateObserver;
  friend class content::MediaSessionServiceImplBrowserTest;

  CONTENT_EXPORT void SetDelegateForTests(
      std::unique_ptr<AudioFocusDelegate> delegate);
  CONTENT_EXPORT void RemoveAllPlayersForTest();
  CONTENT_EXPORT MediaSessionUmaHelper* uma_helper_for_test();

  // Representation of a player for the MediaSessionImpl.
  struct PlayerIdentifier {
    PlayerIdentifier(MediaSessionPlayerObserver* observer, int player_id);
    PlayerIdentifier(const PlayerIdentifier&) = default;

    void operator=(const PlayerIdentifier&) = delete;
    bool operator==(const PlayerIdentifier& player_identifier) const;

    // Hash operator for base::hash_map<>.
    struct Hash {
      size_t operator()(const PlayerIdentifier& player_identifier) const;
    };

    MediaSessionPlayerObserver* observer;
    int player_id;
  };
  using PlayersMap = base::hash_set<PlayerIdentifier, PlayerIdentifier::Hash>;
  using StateChangedCallback = base::Callback<void(State)>;

  CONTENT_EXPORT explicit MediaSessionImpl(WebContents* web_contents);

  void Initialize();

  CONTENT_EXPORT void OnSuspendInternal(MediaSession::SuspendType suspend_type,
                                        State new_state);
  CONTENT_EXPORT void OnResumeInternal(MediaSession::SuspendType suspend_type);

  // To be called after a call to AbandonAudioFocus() in order request the
  // delegate to abandon the audio focus.
  CONTENT_EXPORT void AbandonSystemAudioFocusIfNeeded();

  // Notify all information that an observer needs to know when it's added.
  void NotifyAddedObserver(MediaSessionObserver* observer);

  // Notifies observers about the state change of the media session.
  void NotifyAboutStateChange();

  // Internal method that should be used instead of setting audio_focus_state_.
  // It sets audio_focus_state_ and notifies observers about the state change.
  void SetAudioFocusState(State audio_focus_state);

  // Update the volume multiplier when ducking state changes.
  void UpdateVolumeMultiplier();

  // Get the volume multiplier, which depends on whether the media session is
  // ducking.
  double GetVolumeMultiplier() const;

  // Registers a MediaSessionImpl state change callback.
  CONTENT_EXPORT std::unique_ptr<base::CallbackList<void(State)>::Subscription>
  RegisterMediaSessionStateChangedCallbackForTest(
      const StateChangedCallback& cb);

  CONTENT_EXPORT bool AddPepperPlayer(MediaSessionPlayerObserver* observer,
                                      int player_id);

  CONTENT_EXPORT bool AddOneShotPlayer(MediaSessionPlayerObserver* observer,
                                       int player_id);

  // MediaSessionService-related methods

  // Called when the routed service may have changed.
  void UpdateRoutedService();

  // Returns whether the frame |rfh| uses MediaSession API.
  bool IsServiceActiveForRenderFrameHost(RenderFrameHost* rfh);

  // Compute the MediaSessionService that should be routed, which will be used
  // to update |routed_service_|.
  CONTENT_EXPORT MediaSessionServiceImpl* ComputeServiceForRouting();

  std::unique_ptr<AudioFocusDelegate> delegate_;
  PlayersMap normal_players_;
  PlayersMap pepper_players_;
  PlayersMap one_shot_players_;

  State audio_focus_state_;
  MediaSession::SuspendType suspend_type_;
  AudioFocusManager::AudioFocusType audio_focus_type_;

  MediaSessionUmaHelper uma_helper_;

  // The ducking state of this media session. The initial value is |false|, and
  // is set to |true| after StartDucking(), and will be set to |false| after
  // StopDucking().
  bool is_ducking_;

  double ducking_volume_multiplier_;

  base::CallbackList<void(State)> media_session_state_listeners_;

  base::ObserverList<MediaSessionObserver> observers_;

#if defined(OS_ANDROID)
  std::unique_ptr<MediaSessionAndroid> session_android_;
#endif  // defined(OS_ANDROID)

  // MediaSessionService-related fields
  using ServicesMap = std::map<RenderFrameHost*, MediaSessionServiceImpl*>;

  // The collection of all managed services (non-owned pointers). The services
  // are owned by RenderFrameHost and should be registered on creation and
  // unregistered on destroy.
  ServicesMap services_;
  // The currently routed service (non-owned pointer).
  MediaSessionServiceImpl* routed_service_;

  DISALLOW_COPY_AND_ASSIGN(MediaSessionImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_IMPL_H_
