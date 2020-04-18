// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/media_session_impl.h"

#include <algorithm>

#include "base/numerics/ranges.h"
#include "content/browser/media/session/audio_focus_delegate.h"
#include "content/browser/media/session/media_session_controller.h"
#include "content/browser/media/session/media_session_player_observer.h"
#include "content/browser/media/session/media_session_service_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/media_session_observer.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "media/base/media_content_type.h"
#include "third_party/blink/public/platform/modules/mediasession/media_session.mojom.h"

#if defined(OS_ANDROID)
#include "content/browser/media/session/media_session_android.h"
#endif  // defined(OS_ANDROID)

namespace content {

using MediaSessionUserAction = MediaSessionUmaHelper::MediaSessionUserAction;

namespace {

const double kUnduckedVolumeMultiplier = 1.0;
const double kDefaultDuckingVolumeMultiplier = 0.2;

using MapRenderFrameHostToDepth = std::map<RenderFrameHost*, size_t>;

size_t ComputeFrameDepth(RenderFrameHost* rfh,
                         MapRenderFrameHostToDepth* map_rfh_to_depth) {
  DCHECK(rfh);
  size_t depth = 0;
  RenderFrameHost* current_frame = rfh;
  while (current_frame) {
    auto it = map_rfh_to_depth->find(current_frame);
    if (it != map_rfh_to_depth->end()) {
      depth += it->second;
      break;
    }
    ++depth;
    current_frame = current_frame->GetParent();
  }
  (*map_rfh_to_depth)[rfh] = depth;
  return depth;
}

MediaSessionUserAction MediaSessionActionToUserAction(
    blink::mojom::MediaSessionAction action) {
  switch (action) {
    case blink::mojom::MediaSessionAction::PLAY:
      return MediaSessionUserAction::Play;
    case blink::mojom::MediaSessionAction::PAUSE:
      return MediaSessionUserAction::Pause;
    case blink::mojom::MediaSessionAction::PREVIOUS_TRACK:
      return MediaSessionUserAction::PreviousTrack;
    case blink::mojom::MediaSessionAction::NEXT_TRACK:
      return MediaSessionUserAction::NextTrack;
    case blink::mojom::MediaSessionAction::SEEK_BACKWARD:
      return MediaSessionUserAction::SeekBackward;
    case blink::mojom::MediaSessionAction::SEEK_FORWARD:
      return MediaSessionUserAction::SeekForward;
  }
  NOTREACHED();
  return MediaSessionUserAction::Count;
}

}  // anonymous namespace

using MediaSessionSuspendedSource =
    MediaSessionUmaHelper::MediaSessionSuspendedSource;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(MediaSessionImpl);

MediaSessionImpl::PlayerIdentifier::PlayerIdentifier(
    MediaSessionPlayerObserver* observer,
    int player_id)
    : observer(observer), player_id(player_id) {}

bool MediaSessionImpl::PlayerIdentifier::operator==(
    const PlayerIdentifier& other) const {
  return this->observer == other.observer && this->player_id == other.player_id;
}

size_t MediaSessionImpl::PlayerIdentifier::Hash::operator()(
    const PlayerIdentifier& player_identifier) const {
  size_t hash = BASE_HASH_NAMESPACE::hash<MediaSessionPlayerObserver*>()(
      player_identifier.observer);
  hash += BASE_HASH_NAMESPACE::hash<int>()(player_identifier.player_id);
  return hash;
}

// static
MediaSession* MediaSession::Get(WebContents* web_contents) {
  return MediaSessionImpl::Get(web_contents);
}

// static
MediaSessionImpl* MediaSessionImpl::Get(WebContents* web_contents) {
  MediaSessionImpl* session = FromWebContents(web_contents);
  if (!session) {
    CreateForWebContents(web_contents);
    session = FromWebContents(web_contents);
    session->Initialize();
  }
  return session;
}

MediaSessionImpl::~MediaSessionImpl() {
  DCHECK(normal_players_.empty());
  DCHECK(pepper_players_.empty());
  DCHECK(one_shot_players_.empty());
  DCHECK(audio_focus_state_ == State::INACTIVE);
  for (auto& observer : observers_) {
    observer.MediaSessionDestroyed();
    observer.StopObserving();
  }
}

void MediaSessionImpl::WebContentsDestroyed() {
  // This should only work for tests. In production, all the players should have
  // already been removed before WebContents is destroyed.

  // TODO(zqzhang): refactor MediaSessionImpl, maybe move the interface used to
  // talk with AudioFocusManager out to a seperate class. The AudioFocusManager
  // unit tests then could mock the interface and abandon audio focus when
  // WebContents is destroyed. See https://crbug.com/651069
  normal_players_.clear();
  pepper_players_.clear();
  one_shot_players_.clear();
  AbandonSystemAudioFocusIfNeeded();
}

void MediaSessionImpl::RenderFrameDeleted(RenderFrameHost* rfh) {
  if (services_.count(rfh))
    OnServiceDestroyed(services_[rfh]);
}

void MediaSessionImpl::DidFinishNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  RenderFrameHost* rfh = navigation_handle->GetRenderFrameHost();
  if (services_.count(rfh))
    services_[rfh]->DidFinishNavigation();
}

void MediaSessionImpl::AddObserver(MediaSessionObserver* observer) {
  observers_.AddObserver(observer);
  NotifyAddedObserver(observer);
}

void MediaSessionImpl::RemoveObserver(MediaSessionObserver* observer) {
  observers_.RemoveObserver(observer);
}

void MediaSessionImpl::NotifyAddedObserver(MediaSessionObserver* observer) {
  observer->MediaSessionMetadataChanged(
      routed_service_ ? routed_service_->metadata() : base::nullopt);
  observer->MediaSessionActionsChanged(
      routed_service_ ? routed_service_->actions()
                      : std::set<blink::mojom::MediaSessionAction>());
  observer->MediaSessionStateChanged(IsControllable(), IsActuallyPaused());
}

void MediaSessionImpl::NotifyMediaSessionMetadataChange(
    const base::Optional<MediaMetadata>& metadata) {
  for (auto& observer : observers_)
    observer.MediaSessionMetadataChanged(metadata);
}

void MediaSessionImpl::NotifyMediaSessionActionsChange(
    const std::set<blink::mojom::MediaSessionAction>& actions) {
  for (auto& observer : observers_)
    observer.MediaSessionActionsChanged(actions);
}

bool MediaSessionImpl::AddPlayer(MediaSessionPlayerObserver* observer,
                                 int player_id,
                                 media::MediaContentType media_content_type) {
  if (media_content_type == media::MediaContentType::OneShot)
    return AddOneShotPlayer(observer, player_id);
  if (media_content_type == media::MediaContentType::Pepper)
    return AddPepperPlayer(observer, player_id);

  observer->OnSetVolumeMultiplier(player_id, GetVolumeMultiplier());

  AudioFocusManager::AudioFocusType required_audio_focus_type;
  if (media_content_type == media::MediaContentType::Persistent) {
    required_audio_focus_type = AudioFocusManager::AudioFocusType::Gain;
  } else {
    required_audio_focus_type =
        AudioFocusManager::AudioFocusType::GainTransientMayDuck;
  }

  // If the audio focus is already granted and is of type Content, there is
  // nothing to do. If it is granted of type Transient the requested type is
  // also transient, there is also nothing to do. Otherwise, the session needs
  // to request audio focus again.
  if (audio_focus_state_ == State::ACTIVE &&
      (audio_focus_type_ == AudioFocusManager::AudioFocusType::Gain ||
       audio_focus_type_ == required_audio_focus_type)) {
    normal_players_.insert(PlayerIdentifier(observer, player_id));
    return true;
  }

  State old_audio_focus_state = audio_focus_state_;
  RequestSystemAudioFocus(required_audio_focus_type);

  if (audio_focus_state_ != State::ACTIVE)
    return false;

  // The session should be reset if a player is starting while all players are
  // suspended.
  if (old_audio_focus_state != State::ACTIVE)
    normal_players_.clear();

  normal_players_.insert(PlayerIdentifier(observer, player_id));

  UpdateRoutedService();
  NotifyAboutStateChange();
  return true;
}

void MediaSessionImpl::RemovePlayer(MediaSessionPlayerObserver* observer,
                                    int player_id) {
  bool was_controllable = IsControllable();

  PlayerIdentifier identifier(observer, player_id);

  auto it = normal_players_.find(identifier);
  if (it != normal_players_.end())
    normal_players_.erase(it);

  it = pepper_players_.find(identifier);
  if (it != pepper_players_.end())
    pepper_players_.erase(it);

  it = one_shot_players_.find(identifier);
  if (it != one_shot_players_.end())
    one_shot_players_.erase(it);

  AbandonSystemAudioFocusIfNeeded();
  UpdateRoutedService();

  // The session may become controllable after removing a one-shot player.
  // However AbandonSystemAudioFocusIfNeeded will short-return and won't notify
  // about the state change.
  if (!was_controllable && IsControllable())
    NotifyAboutStateChange();
}

void MediaSessionImpl::RemovePlayers(MediaSessionPlayerObserver* observer) {
  bool was_controllable = IsControllable();

  for (auto it = normal_players_.begin(); it != normal_players_.end();) {
    if (it->observer == observer)
      normal_players_.erase(it++);
    else
      ++it;
  }

  for (auto it = pepper_players_.begin(); it != pepper_players_.end();) {
    if (it->observer == observer)
      pepper_players_.erase(it++);
    else
      ++it;
  }

  for (auto it = one_shot_players_.begin(); it != one_shot_players_.end();) {
    if (it->observer == observer)
      one_shot_players_.erase(it++);
    else
      ++it;
  }

  AbandonSystemAudioFocusIfNeeded();
  UpdateRoutedService();

  // The session may become controllable after removing a one-shot player.
  // However AbandonSystemAudioFocusIfNeeded will short-return and won't notify
  // about the state change.
  if (!was_controllable && IsControllable())
    NotifyAboutStateChange();
}

void MediaSessionImpl::RecordSessionDuck() {
  uma_helper_.RecordSessionSuspended(
      MediaSessionSuspendedSource::SystemTransientDuck);
}

void MediaSessionImpl::OnPlayerPaused(MediaSessionPlayerObserver* observer,
                                      int player_id) {
  // If a playback is completed, BrowserMediaPlayerManager will call
  // OnPlayerPaused() after RemovePlayer(). This is a workaround.
  // Also, this method may be called when a player that is not added
  // to this session (e.g. a silent video) is paused. MediaSessionImpl
  // should ignore the paused player for this case.
  PlayerIdentifier identifier(observer, player_id);
  if (!normal_players_.count(identifier) &&
      !pepper_players_.count(identifier) &&
      !one_shot_players_.count(identifier)) {
    return;
  }

  // If the player to be removed is a pepper player, or there is more than one
  // observer, remove the paused one from the session.
  if (pepper_players_.count(identifier) || normal_players_.size() != 1) {
    RemovePlayer(observer, player_id);
    return;
  }

  // If the player is a one-shot player, just remove it since it is not expected
  // to resume a one-shot player via resuming MediaSession.
  if (one_shot_players_.count(identifier)) {
    RemovePlayer(observer, player_id);
    return;
  }

  // Otherwise, suspend the session.
  DCHECK(IsActive());
  OnSuspendInternal(SuspendType::CONTENT, State::SUSPENDED);
}

void MediaSessionImpl::Resume(SuspendType suspend_type) {
  if (!IsSuspended())
    return;

  if (suspend_type == SuspendType::UI) {
    MediaSessionUmaHelper::RecordMediaSessionUserAction(
        MediaSessionUmaHelper::MediaSessionUserAction::PlayDefault);
  }

  // When the resume requests comes from another source than system, audio focus
  // must be requested.
  if (suspend_type != SuspendType::SYSTEM) {
    // Request audio focus again in case we lost it because another app started
    // playing while the playback was paused.
    State audio_focus_state = RequestSystemAudioFocus(audio_focus_type_)
                                  ? State::ACTIVE
                                  : State::INACTIVE;
    SetAudioFocusState(audio_focus_state);

    if (audio_focus_state_ != State::ACTIVE)
      return;
  }

  OnResumeInternal(suspend_type);
}

void MediaSessionImpl::Suspend(SuspendType suspend_type) {
  if (!IsActive())
    return;

  if (suspend_type == SuspendType::UI) {
    MediaSessionUmaHelper::RecordMediaSessionUserAction(
        MediaSessionUserAction::PauseDefault);
  }

  OnSuspendInternal(suspend_type, State::SUSPENDED);
}

void MediaSessionImpl::Stop(SuspendType suspend_type) {
  DCHECK(audio_focus_state_ != State::INACTIVE);
  DCHECK(suspend_type != SuspendType::CONTENT);
  DCHECK(!HasPepper());

  if (suspend_type == SuspendType::UI) {
    MediaSessionUmaHelper::RecordMediaSessionUserAction(
        MediaSessionUmaHelper::MediaSessionUserAction::StopDefault);
  }

  // TODO(mlamouri): merge the logic between UI and SYSTEM.
  if (suspend_type == SuspendType::SYSTEM) {
    OnSuspendInternal(suspend_type, State::INACTIVE);
    return;
  }

  if (audio_focus_state_ != State::SUSPENDED)
    OnSuspendInternal(suspend_type, State::SUSPENDED);

  DCHECK(audio_focus_state_ == State::SUSPENDED);
  normal_players_.clear();

  AbandonSystemAudioFocusIfNeeded();
}

void MediaSessionImpl::SeekForward(base::TimeDelta seek_time) {
  for (const auto& it : normal_players_)
    it.observer->OnSeekForward(it.player_id, seek_time);
}

void MediaSessionImpl::SeekBackward(base::TimeDelta seek_time) {
  for (const auto& it : normal_players_)
    it.observer->OnSeekBackward(it.player_id, seek_time);
}

bool MediaSessionImpl::IsControllable() const {
  // Only media session having focus Gain can be controllable unless it is
  // inactive. Also, the session will be uncontrollable if it contains one-shot
  // players.
  return audio_focus_state_ != State::INACTIVE &&
         audio_focus_type_ == AudioFocusManager::AudioFocusType::Gain &&
         one_shot_players_.empty();
}

bool MediaSessionImpl::IsActuallyPaused() const {
  if (routed_service_ && routed_service_->playback_state() ==
                             blink::mojom::MediaSessionPlaybackState::PLAYING) {
    return false;
  }

  return !IsActive();
}

void MediaSessionImpl::SetDuckingVolumeMultiplier(double multiplier) {
  ducking_volume_multiplier_ = base::ClampToRange(multiplier, 0.0, 1.0);
}

void MediaSessionImpl::StartDucking() {
  if (is_ducking_)
    return;
  is_ducking_ = true;
  UpdateVolumeMultiplier();
}

void MediaSessionImpl::StopDucking() {
  if (!is_ducking_)
    return;
  is_ducking_ = false;
  UpdateVolumeMultiplier();
}

void MediaSessionImpl::UpdateVolumeMultiplier() {
  for (const auto& it : normal_players_)
    it.observer->OnSetVolumeMultiplier(it.player_id, GetVolumeMultiplier());
  for (const auto& it : pepper_players_)
    it.observer->OnSetVolumeMultiplier(it.player_id, GetVolumeMultiplier());
}

double MediaSessionImpl::GetVolumeMultiplier() const {
  return is_ducking_ ? ducking_volume_multiplier_ : kUnduckedVolumeMultiplier;
}

bool MediaSessionImpl::IsActive() const {
  return audio_focus_state_ == State::ACTIVE;
}

bool MediaSessionImpl::IsSuspended() const {
  return audio_focus_state_ == State::SUSPENDED;
}

bool MediaSessionImpl::HasPepper() const {
  return !pepper_players_.empty();
}

std::unique_ptr<base::CallbackList<void(MediaSessionImpl::State)>::Subscription>
MediaSessionImpl::RegisterMediaSessionStateChangedCallbackForTest(
    const StateChangedCallback& cb) {
  return media_session_state_listeners_.Add(cb);
}

void MediaSessionImpl::SetDelegateForTests(
    std::unique_ptr<AudioFocusDelegate> delegate) {
  delegate_ = std::move(delegate);
}

MediaSessionUmaHelper* MediaSessionImpl::uma_helper_for_test() {
  return &uma_helper_;
}

void MediaSessionImpl::RemoveAllPlayersForTest() {
  normal_players_.clear();
  pepper_players_.clear();
  one_shot_players_.clear();
  AbandonSystemAudioFocusIfNeeded();
}

void MediaSessionImpl::OnSuspendInternal(SuspendType suspend_type,
                                         State new_state) {
  DCHECK(!HasPepper());

  DCHECK(new_state == State::SUSPENDED || new_state == State::INACTIVE);
  // UI suspend cannot use State::INACTIVE.
  DCHECK(suspend_type == SuspendType::SYSTEM || new_state == State::SUSPENDED);

  if (!one_shot_players_.empty())
    return;

  if (audio_focus_state_ != State::ACTIVE)
    return;

  switch (suspend_type) {
    case SuspendType::UI:
      uma_helper_.RecordSessionSuspended(MediaSessionSuspendedSource::UI);
      break;
    case SuspendType::SYSTEM:
      switch (new_state) {
        case State::SUSPENDED:
          uma_helper_.RecordSessionSuspended(
              MediaSessionSuspendedSource::SystemTransient);
          break;
        case State::INACTIVE:
          uma_helper_.RecordSessionSuspended(
              MediaSessionSuspendedSource::SystemPermanent);
          break;
        case State::ACTIVE:
          NOTREACHED();
          break;
      }
      break;
    case SuspendType::CONTENT:
      uma_helper_.RecordSessionSuspended(MediaSessionSuspendedSource::CONTENT);
      break;
  }

  SetAudioFocusState(new_state);
  suspend_type_ = suspend_type;

  if (suspend_type != SuspendType::CONTENT) {
    // SuspendType::CONTENT happens when the suspend action came from
    // the page in which case the player is already paused.
    // Otherwise, the players need to be paused.
    for (const auto& it : normal_players_)
      it.observer->OnSuspend(it.player_id);
  }

  for (const auto& it : pepper_players_)
    it.observer->OnSetVolumeMultiplier(it.player_id,
                                       ducking_volume_multiplier_);

  NotifyAboutStateChange();
}

void MediaSessionImpl::OnResumeInternal(SuspendType suspend_type) {
  if (suspend_type == SuspendType::SYSTEM && suspend_type_ != suspend_type)
    return;

  SetAudioFocusState(State::ACTIVE);

  for (const auto& it : normal_players_)
    it.observer->OnResume(it.player_id);

  for (const auto& it : pepper_players_)
    it.observer->OnSetVolumeMultiplier(it.player_id, GetVolumeMultiplier());

  NotifyAboutStateChange();
}

MediaSessionImpl::MediaSessionImpl(WebContents* web_contents)
    : WebContentsObserver(web_contents),
      audio_focus_state_(State::INACTIVE),
      audio_focus_type_(
          AudioFocusManager::AudioFocusType::GainTransientMayDuck),
      is_ducking_(false),
      ducking_volume_multiplier_(kDefaultDuckingVolumeMultiplier),
      routed_service_(nullptr) {
#if defined(OS_ANDROID)
  session_android_.reset(new MediaSessionAndroid(this));
#endif  // defined(OS_ANDROID)
}

void MediaSessionImpl::Initialize() {
  delegate_ = AudioFocusDelegate::Create(this);
}

bool MediaSessionImpl::RequestSystemAudioFocus(
    AudioFocusManager::AudioFocusType audio_focus_type) {
  bool result = delegate_->RequestAudioFocus(audio_focus_type);
  uma_helper_.RecordRequestAudioFocusResult(result);

  // Make sure we are unducked.
  if (result)
    StopDucking();

  // MediaSessionImpl must change its state & audio focus type AFTER requesting
  // audio focus.
  SetAudioFocusState(result ? State::ACTIVE : State::INACTIVE);
  audio_focus_type_ = audio_focus_type;
  return result;
}

void MediaSessionImpl::AbandonSystemAudioFocusIfNeeded() {
  if (audio_focus_state_ == State::INACTIVE || !normal_players_.empty() ||
      !pepper_players_.empty() || !one_shot_players_.empty()) {
    return;
  }
  delegate_->AbandonAudioFocus();
  is_ducking_ = false;

  SetAudioFocusState(State::INACTIVE);
  NotifyAboutStateChange();
}

void MediaSessionImpl::NotifyAboutStateChange() {
  media_session_state_listeners_.Notify(audio_focus_state_);

  bool is_actually_paused = IsActuallyPaused();
  for (auto& observer : observers_)
    observer.MediaSessionStateChanged(IsControllable(), is_actually_paused);
}

void MediaSessionImpl::SetAudioFocusState(State audio_focus_state) {
  if (audio_focus_state == audio_focus_state_)
    return;

  audio_focus_state_ = audio_focus_state;
  switch (audio_focus_state_) {
    case State::ACTIVE:
      uma_helper_.OnSessionActive();
      break;
    case State::SUSPENDED:
      uma_helper_.OnSessionSuspended();
      break;
    case State::INACTIVE:
      uma_helper_.OnSessionInactive();
      break;
  }
}

bool MediaSessionImpl::AddPepperPlayer(MediaSessionPlayerObserver* observer,
                                       int player_id) {
  bool success =
      RequestSystemAudioFocus(AudioFocusManager::AudioFocusType::Gain);
  DCHECK(success);

  pepper_players_.insert(PlayerIdentifier(observer, player_id));

  observer->OnSetVolumeMultiplier(player_id, GetVolumeMultiplier());

  NotifyAboutStateChange();
  return success;
}

bool MediaSessionImpl::AddOneShotPlayer(MediaSessionPlayerObserver* observer,
                                        int player_id) {
  if (!RequestSystemAudioFocus(AudioFocusManager::AudioFocusType::Gain))
    return false;

  one_shot_players_.insert(PlayerIdentifier(observer, player_id));
  NotifyAboutStateChange();

  return true;
}

// MediaSessionService-related methods

void MediaSessionImpl::OnServiceCreated(MediaSessionServiceImpl* service) {
  RenderFrameHost* rfh = service->GetRenderFrameHost();
  if (!rfh)
    return;

  services_[rfh] = service;
  UpdateRoutedService();
}

void MediaSessionImpl::OnServiceDestroyed(MediaSessionServiceImpl* service) {
  services_.erase(service->GetRenderFrameHost());
  if (routed_service_ == service) {
    routed_service_ = nullptr;
    UpdateRoutedService();
  }
}

void MediaSessionImpl::OnMediaSessionPlaybackStateChanged(
    MediaSessionServiceImpl* service) {
  if (service != routed_service_)
    return;
  NotifyAboutStateChange();
}

void MediaSessionImpl::OnMediaSessionMetadataChanged(
    MediaSessionServiceImpl* service) {
  if (service != routed_service_)
    return;

  NotifyMediaSessionMetadataChange(routed_service_->metadata());
}

void MediaSessionImpl::OnMediaSessionActionsChanged(
    MediaSessionServiceImpl* service) {
  if (service != routed_service_)
    return;

  NotifyMediaSessionActionsChange(routed_service_->actions());
}

void MediaSessionImpl::DidReceiveAction(
    blink::mojom::MediaSessionAction action) {
  MediaSessionUmaHelper::RecordMediaSessionUserAction(
      MediaSessionActionToUserAction(action));

  // Pause all players in non-routed frames if the action is PAUSE.
  //
  // This is the default PAUSE action handler per Media Session API spec. The
  // reason for pausing all players in all other sessions is to avoid the
  // players in other frames keep the session active so that the UI will always
  // show the pause button but it does not pause anything (as the routed frame
  // already pauses when responding to the PAUSE action while other frames does
  // not).
  //
  // TODO(zqzhang): Currently, this might not work well on desktop as Pepper and
  // OneShot players are not really suspended, so that the session is still
  // active after this. See https://crbug.com/619084 and
  // https://crbug.com/596516.
  if (blink::mojom::MediaSessionAction::PAUSE == action) {
    RenderFrameHost* rfh_of_routed_service =
        routed_service_ ? routed_service_->GetRenderFrameHost() : nullptr;
    for (const auto& player : normal_players_) {
      if (player.observer->render_frame_host() != rfh_of_routed_service)
        player.observer->OnSuspend(player.player_id);
    }
    for (const auto& player : pepper_players_) {
      if (player.observer->render_frame_host() != rfh_of_routed_service) {
        player.observer->OnSetVolumeMultiplier(player.player_id,
                                               ducking_volume_multiplier_);
      }
    }
    for (const auto& player : one_shot_players_) {
      if (player.observer->render_frame_host() != rfh_of_routed_service)
        player.observer->OnSuspend(player.player_id);
    }
  }

  if (!routed_service_)
    return;

  routed_service_->GetClient()->DidReceiveAction(action);
}

bool MediaSessionImpl::IsServiceActiveForRenderFrameHost(RenderFrameHost* rfh) {
  return services_.find(rfh) != services_.end();
}

void MediaSessionImpl::UpdateRoutedService() {
  MediaSessionServiceImpl* new_service = ComputeServiceForRouting();
  if (new_service == routed_service_)
    return;

  routed_service_ = new_service;
  if (routed_service_) {
    NotifyMediaSessionMetadataChange(routed_service_->metadata());
    NotifyMediaSessionActionsChange(routed_service_->actions());
  }
}

MediaSessionServiceImpl* MediaSessionImpl::ComputeServiceForRouting() {
  // The service selection strategy is: select a frame that has a playing/paused
  // player and has a corresponding MediaSessionService and return the
  // corresponding MediaSessionService. If multiple frames satisfy the criteria,
  // prefer the top-most frame.
  std::set<RenderFrameHost*> frames;
  for (const auto& player : normal_players_) {
    RenderFrameHost* frame = player.observer->render_frame_host();
    if (frame)
      frames.insert(frame);
  }

  for (const auto& player : one_shot_players_) {
    RenderFrameHost* frame = player.observer->render_frame_host();
    if (frame)
      frames.insert(frame);
  }

  for (const auto& player : pepper_players_) {
    RenderFrameHost* frame = player.observer->render_frame_host();
    if (frame)
      frames.insert(frame);
  }

  RenderFrameHost* best_frame = nullptr;
  size_t min_depth = std::numeric_limits<size_t>::max();
  std::map<RenderFrameHost*, size_t> map_rfh_to_depth;

  for (RenderFrameHost* frame : frames) {
    size_t depth = ComputeFrameDepth(frame, &map_rfh_to_depth);
    if (depth >= min_depth)
      continue;
    if (!IsServiceActiveForRenderFrameHost(frame))
      continue;
    best_frame = frame;
    min_depth = depth;
  }

  return best_frame ? services_[best_frame] : nullptr;
}

}  // namespace content
