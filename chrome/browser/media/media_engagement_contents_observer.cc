// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_contents_observer.h"

#include <memory>

#include "base/metrics/histogram.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "chrome/browser/media/media_engagement_preloaded_list.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "chrome/browser/media/media_engagement_session.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "media/base/media_switches.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/autoplay.mojom.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif  // !defined(OS_ANDROID)

namespace {

void SendEngagementLevelToFrame(const url::Origin& origin,
                                content::RenderFrameHost* render_frame_host) {
  blink::mojom::AutoplayConfigurationClientAssociatedPtr client;
  render_frame_host->GetRemoteAssociatedInterfaces()->GetInterface(&client);
  client->AddAutoplayFlags(origin,
                           blink::mojom::kAutoplayFlagHighMediaEngagement);
}

}  // namespace.

// This is the minimum size (in px) of each dimension that a media
// element has to be in order to be determined significant.
const gfx::Size MediaEngagementContentsObserver::kSignificantSize =
    gfx::Size(200, 140);

const base::TimeDelta MediaEngagementContentsObserver::kMaxShortPlaybackTime =
    base::TimeDelta::FromSeconds(3);

const char* const
    MediaEngagementContentsObserver::kHistogramScoreAtPlaybackName =
        "Media.Engagement.ScoreAtPlayback";

const char* const MediaEngagementContentsObserver::
    kHistogramSignificantNotAddedFirstTimeName =
        "Media.Engagement.SignificantPlayers.PlayerNotAdded.FirstTime";

const char* const MediaEngagementContentsObserver::
    kHistogramSignificantNotAddedAfterFirstTimeName =
        "Media.Engagement.SignificantPlayers.PlayerNotAdded.AfterFirstTime";

const char* const
    MediaEngagementContentsObserver::kHistogramSignificantRemovedName =
        "Media.Engagement.SignificantPlayers.PlayerRemoved";

const int MediaEngagementContentsObserver::kMaxInsignificantPlaybackReason =
    static_cast<int>(MediaEngagementContentsObserver::
                         InsignificantPlaybackReason::kReasonMax);

const base::TimeDelta
    MediaEngagementContentsObserver::kSignificantMediaPlaybackTime =
        base::TimeDelta::FromSeconds(7);

MediaEngagementContentsObserver::MediaEngagementContentsObserver(
    content::WebContents* web_contents,
    MediaEngagementService* service)
    : WebContentsObserver(web_contents),
      service_(service),
      playback_timer_(new base::Timer(true, false)),
      task_runner_(nullptr) {}

MediaEngagementContentsObserver::~MediaEngagementContentsObserver() = default;

MediaEngagementContentsObserver::PlaybackTimer::PlaybackTimer(
    base::Clock* clock)
    : clock_(clock) {}

void MediaEngagementContentsObserver::PlaybackTimer::Start() {
  start_time_ = clock_->Now();
}

void MediaEngagementContentsObserver::PlaybackTimer::Stop() {
  recorded_time_ = Elapsed();
  start_time_.reset();
}

bool MediaEngagementContentsObserver::PlaybackTimer::IsRunning() const {
  return start_time_.has_value();
}

base::TimeDelta MediaEngagementContentsObserver::PlaybackTimer::Elapsed()
    const {
  base::Time now = clock_->Now();
  base::TimeDelta duration = now - start_time_.value_or(now);
  return recorded_time_ + duration;
}

void MediaEngagementContentsObserver::PlaybackTimer::Reset() {
  recorded_time_ = base::TimeDelta();
  start_time_.reset();
}

void MediaEngagementContentsObserver::WebContentsDestroyed() {
  RegisterAudiblePlayersWithSession();
  session_ = nullptr;

  ClearPlayerStates();
  service_->contents_observers_.erase(web_contents());
  delete this;
}

void MediaEngagementContentsObserver::ClearPlayerStates() {
  playback_timer_->Stop();
  player_states_.clear();
  significant_players_.clear();
}

void MediaEngagementContentsObserver::RegisterAudiblePlayersWithSession() {
  if (!session_)
    return;

  int32_t significant_players = 0;
  int32_t audible_players = 0;

  for (const auto& row : audible_players_) {
    const PlayerState& player_state = GetPlayerState(row.first);
    const base::TimeDelta elapsed = player_state.playback_timer->Elapsed();

    if (elapsed < kMaxShortPlaybackTime && player_state.reached_end_of_stream) {
      session_->RecordShortPlaybackIgnored(elapsed.InMilliseconds());
      continue;
    }

    significant_players += row.second.first;
    ++audible_players;
  }

  session_->RegisterAudiblePlayers(audible_players, significant_players);
  audible_players_.clear();
}

void MediaEngagementContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument() || navigation_handle->IsErrorPage()) {
    return;
  }

  RegisterAudiblePlayersWithSession();
  ClearPlayerStates();

  url::Origin new_origin = url::Origin::Create(navigation_handle->GetURL());
  if (session_ && session_->IsSameOriginWith(new_origin))
    return;

  // Only get the opener if the navigation originated from a link.
  content::WebContents* opener = nullptr;
  if (ui::PageTransitionCoreTypeIs(navigation_handle->GetPageTransition(),
                                   ui::PAGE_TRANSITION_LINK) ||
      ui::PageTransitionCoreTypeIs(navigation_handle->GetPageTransition(),
                                   ui::PAGE_TRANSITION_RELOAD)) {
    opener = GetOpener();
  }

  bool was_restored =
      navigation_handle->GetRestoreType() != content::RestoreType::NONE;
  session_ = GetOrCreateSession(new_origin, opener, was_restored);
}

MediaEngagementContentsObserver::PlayerState::PlayerState(base::Clock* clock)
    : playback_timer(new PlaybackTimer(clock)) {}

MediaEngagementContentsObserver::PlayerState::~PlayerState() = default;

MediaEngagementContentsObserver::PlayerState::PlayerState(PlayerState&&) =
    default;

MediaEngagementContentsObserver::PlayerState&
MediaEngagementContentsObserver::GetPlayerState(const MediaPlayerId& id) {
  auto state = player_states_.find(id);
  if (state != player_states_.end())
    return state->second;

  auto iter =
      player_states_.insert(std::make_pair(id, PlayerState(service_->clock_)));
  return iter.first->second;
}

void MediaEngagementContentsObserver::MediaStartedPlaying(
    const MediaPlayerInfo& media_player_info,
    const MediaPlayerId& media_player_id) {
  PlayerState& state = GetPlayerState(media_player_id);
  state.playing = true;
  state.has_audio = media_player_info.has_audio;
  state.has_video = media_player_info.has_video;

  // Reset the playback timer if we previously reached the end of the stream.
  if (state.reached_end_of_stream) {
    state.playback_timer->Reset();
    state.reached_end_of_stream = false;
  }
  state.playback_timer->Start();

  MaybeInsertRemoveSignificantPlayer(media_player_id);
  UpdatePlayerTimer(media_player_id);
  RecordEngagementScoreToHistogramAtPlayback(media_player_id);
}

void MediaEngagementContentsObserver::
    RecordEngagementScoreToHistogramAtPlayback(const MediaPlayerId& id) {
  if (!session_)
    return;

  PlayerState& state = GetPlayerState(id);
  if (!state.playing.value_or(false) || state.muted.value_or(true) ||
      !state.has_audio.value_or(false) || !state.has_video.value_or(false) ||
      state.score_recorded) {
    return;
  }

  int percentage =
      round(service_->GetEngagementScore(session_->origin().GetURL()) * 100);
  UMA_HISTOGRAM_PERCENTAGE(
      MediaEngagementContentsObserver::kHistogramScoreAtPlaybackName,
      percentage);
  state.score_recorded = true;
}

void MediaEngagementContentsObserver::MediaMutedStatusChanged(
    const MediaPlayerId& id,
    bool muted) {
  GetPlayerState(id).muted = muted;
  MaybeInsertRemoveSignificantPlayer(id);
  UpdatePlayerTimer(id);
  RecordEngagementScoreToHistogramAtPlayback(id);
}

void MediaEngagementContentsObserver::MediaResized(const gfx::Size& size,
                                                   const MediaPlayerId& id) {
  GetPlayerState(id).significant_size =
      (size.width() >= kSignificantSize.width() &&
       size.height() >= kSignificantSize.height());
  MaybeInsertRemoveSignificantPlayer(id);
  UpdatePlayerTimer(id);
}

void MediaEngagementContentsObserver::MediaStoppedPlaying(
    const MediaPlayerInfo& media_player_info,
    const MediaPlayerId& media_player_id,
    WebContentsObserver::MediaStoppedReason reason) {
  PlayerState& state = GetPlayerState(media_player_id);
  state.playing = false;
  state.reached_end_of_stream =
      reason == WebContentsObserver::MediaStoppedReason::kReachedEndOfStream;

  // Reset the playback timer if we finished playing.
  state.playback_timer->Stop();

  MaybeInsertRemoveSignificantPlayer(media_player_id);
  UpdatePlayerTimer(media_player_id);
}

void MediaEngagementContentsObserver::DidUpdateAudioMutingState(bool muted) {
  UpdatePageTimer();
}

std::vector<MediaEngagementContentsObserver::InsignificantPlaybackReason>
MediaEngagementContentsObserver::GetInsignificantPlayerReasons(
    const PlayerState& state) {
  std::vector<MediaEngagementContentsObserver::InsignificantPlaybackReason>
      reasons;

  if (state.muted.value_or(true)) {
    reasons.push_back(MediaEngagementContentsObserver::
                          InsignificantPlaybackReason::kAudioMuted);
  }

  if (!state.playing.value_or(false)) {
    reasons.push_back(MediaEngagementContentsObserver::
                          InsignificantPlaybackReason::kMediaPaused);
  }

  if (!state.significant_size.value_or(false) &&
      state.has_video.value_or(false)) {
    reasons.push_back(MediaEngagementContentsObserver::
                          InsignificantPlaybackReason::kFrameSizeTooSmall);
  }

  if (!state.has_audio.value_or(false)) {
    reasons.push_back(MediaEngagementContentsObserver::
                          InsignificantPlaybackReason::kNoAudioTrack);
  }

  return reasons;
}

bool MediaEngagementContentsObserver::IsPlayerStateComplete(
    const PlayerState& state) {
  return state.muted.has_value() && state.playing.has_value() &&
         state.has_audio.has_value() && state.has_video.has_value() &&
         (!state.has_video.value_or(false) ||
          state.significant_size.has_value());
}

void MediaEngagementContentsObserver::OnSignificantMediaPlaybackTimeForPlayer(
    const MediaPlayerId& id) {
  // Clear the timer.
  auto audible_row = audible_players_.find(id);
  audible_row->second.second = nullptr;

  // Check that the tab is not muted.
  if (web_contents()->IsAudioMuted() || !web_contents()->WasRecentlyAudible())
    return;

  // Record significant audible playback.
  audible_row->second.first = true;
}

void MediaEngagementContentsObserver::OnSignificantMediaPlaybackTimeForPage() {
  DCHECK(session_);

  if (session_->significant_playback_recorded())
    return;

  // Do not record significant playback if the tab did not make
  // a sound in the last two seconds.
  if (!web_contents()->WasRecentlyAudible())
    return;

  session_->RecordSignificantPlayback();
}

void MediaEngagementContentsObserver::RecordInsignificantReasons(
    std::vector<MediaEngagementContentsObserver::InsignificantPlaybackReason>
        reasons,
    const PlayerState& state,
    MediaEngagementContentsObserver::InsignificantHistogram histogram) {
  DCHECK(IsPlayerStateComplete(state));

  std::string histogram_name;
  switch (histogram) {
    case MediaEngagementContentsObserver::InsignificantHistogram::
        kPlayerRemoved:
      histogram_name =
          MediaEngagementContentsObserver::kHistogramSignificantRemovedName;
      break;
    case MediaEngagementContentsObserver::InsignificantHistogram::
        kPlayerNotAddedFirstTime:
      histogram_name = MediaEngagementContentsObserver::
          kHistogramSignificantNotAddedFirstTimeName;
      break;
    case MediaEngagementContentsObserver::InsignificantHistogram::
        kPlayerNotAddedAfterFirstTime:
      histogram_name = MediaEngagementContentsObserver::
          kHistogramSignificantNotAddedAfterFirstTimeName;
      break;
    default:
      NOTREACHED();
      break;
  }

  base::HistogramBase* base_histogram = base::LinearHistogram::FactoryGet(
      histogram_name, 1,
      MediaEngagementContentsObserver::kMaxInsignificantPlaybackReason,
      MediaEngagementContentsObserver::kMaxInsignificantPlaybackReason + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag);

  for (auto reason : reasons)
    base_histogram->Add(static_cast<int>(reason));

  base_histogram->Add(static_cast<int>(
      MediaEngagementContentsObserver::InsignificantPlaybackReason::kCount));
}

void MediaEngagementContentsObserver::MaybeInsertRemoveSignificantPlayer(
    const MediaPlayerId& id) {
  // If we have not received the whole player state yet then we can't be
  // significant and therefore we don't want to make a decision yet.
  PlayerState& state = GetPlayerState(id);
  if (!IsPlayerStateComplete(state))
    return;

  // If the player has an audio track, is un-muted and is playing then we should
  // add it to the audible players map.
  if (state.muted == false && state.playing == true &&
      state.has_audio == true &&
      audible_players_.find(id) == audible_players_.end()) {
    audible_players_[id] =
        std::make_pair(false, base::WrapUnique<base::Timer>(nullptr));
  }

  bool is_currently_significant =
      significant_players_.find(id) != significant_players_.end();
  std::vector<MediaEngagementContentsObserver::InsignificantPlaybackReason>
      reasons = GetInsignificantPlayerReasons(state);

  if (is_currently_significant) {
    if (!reasons.empty()) {
      // We are considered significant and we have reasons why we shouldn't
      // be, so we should make the player not significant.
      significant_players_.erase(id);
      RecordInsignificantReasons(reasons, state,
                                 MediaEngagementContentsObserver::
                                     InsignificantHistogram::kPlayerRemoved);
    }
  } else {
    if (reasons.empty()) {
      // We are not considered significant but we don't have any reasons
      // why we shouldn't be. Make the player significant.
      significant_players_.insert(id);
    } else if (state.reasons_recorded) {
      RecordInsignificantReasons(
          reasons, state,
          MediaEngagementContentsObserver::InsignificantHistogram::
              kPlayerNotAddedAfterFirstTime);
    } else {
      RecordInsignificantReasons(
          reasons, state,
          MediaEngagementContentsObserver::InsignificantHistogram::
              kPlayerNotAddedFirstTime);
      state.reasons_recorded = true;
    }
  }
}

void MediaEngagementContentsObserver::UpdatePlayerTimer(
    const MediaPlayerId& id) {
  UpdatePageTimer();

  // The player should be considered audible.
  auto audible_row = audible_players_.find(id);
  if (audible_row == audible_players_.end())
    return;

  // If we meet all the reqirements for being significant then start a timer.
  if (significant_players_.find(id) != significant_players_.end()) {
    if (audible_row->second.second)
      return;

    std::unique_ptr<base::Timer> new_timer =
        std::make_unique<base::Timer>(true, false);
    if (task_runner_)
      new_timer->SetTaskRunner(task_runner_);

    new_timer->Start(
        FROM_HERE,
        MediaEngagementContentsObserver::kSignificantMediaPlaybackTime,
        base::Bind(&MediaEngagementContentsObserver::
                       OnSignificantMediaPlaybackTimeForPlayer,
                   base::Unretained(this), id));

    audible_row->second.second = std::move(new_timer);
  } else if (audible_row->second.second) {
    // We no longer meet the requirements so we should get rid of the timer.
    audible_row->second.second = nullptr;
  }
}

bool MediaEngagementContentsObserver::AreConditionsMet() const {
  if (significant_players_.empty())
    return false;

  return !web_contents()->IsAudioMuted();
}

void MediaEngagementContentsObserver::UpdatePageTimer() {
  if (!session_ || session_->significant_playback_recorded())
    return;

  if (AreConditionsMet()) {
    if (playback_timer_->IsRunning())
      return;

    if (task_runner_)
      playback_timer_->SetTaskRunner(task_runner_);

    playback_timer_->Start(
        FROM_HERE,
        MediaEngagementContentsObserver::kSignificantMediaPlaybackTime,
        base::Bind(&MediaEngagementContentsObserver::
                       OnSignificantMediaPlaybackTimeForPage,
                   base::Unretained(this)));
  } else {
    if (!playback_timer_->IsRunning())
      return;
    playback_timer_->Stop();
  }
}

void MediaEngagementContentsObserver::SetTaskRunnerForTest(
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  task_runner_ = std::move(task_runner);
}

void MediaEngagementContentsObserver::ReadyToCommitNavigation(
    content::NavigationHandle* handle) {
  // TODO(beccahughes): Convert MEI API to using origin.
  // If the navigation is occuring in the main frame we should use the URL
  // provided by |handle| as the navigation has not committed yet. If the
  // navigation is in a sub frame then use the URL from the main frame.
  GURL url = handle->IsInMainFrame()
                 ? handle->GetURL()
                 : handle->GetWebContents()->GetLastCommittedURL();
  MediaEngagementScore score = service_->CreateEngagementScore(url);
  bool has_high_engagement = score.high_score();

  // If the preloaded feature flag is enabled and the number of visits is less
  // than the number of visits required to have an MEI score we should check the
  // global data.
  if (!has_high_engagement &&
      score.visits() < MediaEngagementScore::GetScoreMinVisits() &&
      base::FeatureList::IsEnabled(media::kPreloadMediaEngagementData)) {
    has_high_engagement =
        MediaEngagementPreloadedList::GetInstance()->CheckOriginIsPresent(
            url::Origin::Create(url));
  }

  // If we have high media engagement then we should send that to Blink.
  if (has_high_engagement) {
    SendEngagementLevelToFrame(url::Origin::Create(handle->GetURL()),
                               handle->GetRenderFrameHost());
  }
}

content::WebContents* MediaEngagementContentsObserver::GetOpener() const {
#if !defined(OS_ANDROID)
  for (auto* browser : *BrowserList::GetInstance()) {
    if (!browser->profile()->IsSameProfile(service_->profile()) ||
        browser->profile()->GetProfileType() !=
            service_->profile()->GetProfileType()) {
      continue;
    }

    int index =
        browser->tab_strip_model()->GetIndexOfWebContents(web_contents());
    if (index == TabStripModel::kNoTab)
      continue;

    // Whether or not the |opener| is null, this is the right tab strip.
    return browser->tab_strip_model()->GetOpenerOfWebContentsAt(index);
  }
#endif  // !defined(OS_ANDROID)

  return nullptr;
}

scoped_refptr<MediaEngagementSession>
MediaEngagementContentsObserver::GetOrCreateSession(
    const url::Origin& origin,
    content::WebContents* opener,
    bool was_restored) const {
  GURL url = origin.GetURL();
  if (!url.is_valid())
    return nullptr;

  if (!service_->ShouldRecordEngagement(url))
    return nullptr;

  MediaEngagementContentsObserver* opener_observer =
      service_->GetContentsObserverFor(opener);

  if (opener_observer && opener_observer->session_ &&
      opener_observer->session_->IsSameOriginWith(origin)) {
    return opener_observer->session_;
  }

  return new MediaEngagementSession(
      service_, origin,
      was_restored ? MediaEngagementSession::RestoreType::kRestored
                   : MediaEngagementSession::RestoreType::kNotRestored);
}
