// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_score.h"

#include <utility>

#include "base/metrics/field_trial_params.h"
#include "chrome/browser/engagement/site_engagement_metrics.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "media/base/media_switches.h"

const char MediaEngagementScore::kVisitsKey[] = "visits";
const char MediaEngagementScore::kMediaPlaybacksKey[] = "mediaPlaybacks";
const char MediaEngagementScore::kLastMediaPlaybackTimeKey[] =
    "lastMediaPlaybackTime";
const char MediaEngagementScore::kHasHighScoreKey[] = "hasHighScore";
const char MediaEngagementScore::kAudiblePlaybacksKey[] = "audiblePlaybacks";
const char MediaEngagementScore::kSignificantPlaybacksKey[] =
    "significantPlaybacks";
const char MediaEngagementScore::kVisitsWithMediaTagKey[] =
    "visitsWithMediaTag";
const char MediaEngagementScore::kHighScoreChanges[] = "highScoreChanges";

const char MediaEngagementScore::kScoreMinVisitsParamName[] = "min_visits";
const char MediaEngagementScore::kHighScoreLowerThresholdParamName[] =
    "lower_threshold";
const char MediaEngagementScore::kHighScoreUpperThresholdParamName[] =
    "upper_threshold";

namespace {

const int kScoreMinVisitsParamDefault = 20;
const double kHighScoreLowerThresholdParamDefault = 0.2;
const double kHighScoreUpperThresholdParamDefault = 0.3;

std::unique_ptr<base::DictionaryValue> GetMediaEngagementScoreDictForSettings(
    const HostContentSettingsMap* settings,
    const GURL& origin_url) {
  if (!settings)
    return std::make_unique<base::DictionaryValue>();

  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(settings->GetWebsiteSetting(
          origin_url, origin_url, CONTENT_SETTINGS_TYPE_MEDIA_ENGAGEMENT,
          content_settings::ResourceIdentifier(), nullptr));

  if (value.get())
    return value;
  return std::make_unique<base::DictionaryValue>();
}

}  // namespace

// static.
double MediaEngagementScore::GetHighScoreLowerThreshold() {
  return base::GetFieldTrialParamByFeatureAsDouble(
      media::kMediaEngagementBypassAutoplayPolicies,
      kHighScoreLowerThresholdParamName, kHighScoreLowerThresholdParamDefault);
}

// static.
double MediaEngagementScore::GetHighScoreUpperThreshold() {
  return base::GetFieldTrialParamByFeatureAsDouble(
      media::kMediaEngagementBypassAutoplayPolicies,
      kHighScoreUpperThresholdParamName, kHighScoreUpperThresholdParamDefault);
}

// static.
int MediaEngagementScore::GetScoreMinVisits() {
  return base::GetFieldTrialParamByFeatureAsInt(
      media::kMediaEngagementBypassAutoplayPolicies, kScoreMinVisitsParamName,
      kScoreMinVisitsParamDefault);
}

MediaEngagementScore::MediaEngagementScore(base::Clock* clock,
                                           const GURL& origin,
                                           HostContentSettingsMap* settings)
    : MediaEngagementScore(
          clock,
          origin,
          GetMediaEngagementScoreDictForSettings(settings, origin)) {
  settings_map_ = settings;
}

MediaEngagementScore::MediaEngagementScore(
    base::Clock* clock,
    const GURL& origin,
    std::unique_ptr<base::DictionaryValue> score_dict,
    HostContentSettingsMap* settings)
    : MediaEngagementScore(clock, origin, std::move(score_dict)) {
  settings_map_ = settings;
}

MediaEngagementScore::MediaEngagementScore(
    base::Clock* clock,
    const GURL& origin,
    std::unique_ptr<base::DictionaryValue> score_dict)
    : origin_(origin), clock_(clock), score_dict_(score_dict.release()) {
  if (!score_dict_)
    return;

  score_dict_->GetInteger(kVisitsKey, &visits_);
  score_dict_->GetInteger(kMediaPlaybacksKey, &media_playbacks_);
  score_dict_->GetBoolean(kHasHighScoreKey, &is_high_);
  score_dict_->GetInteger(kAudiblePlaybacksKey, &audible_playbacks_);
  score_dict_->GetInteger(kSignificantPlaybacksKey, &significant_playbacks_);
  score_dict_->GetInteger(kVisitsWithMediaTagKey, &visits_with_media_tag_);
  score_dict_->GetInteger(kHighScoreChanges, &high_score_changes_);

  double internal_time;
  if (score_dict_->GetDouble(kLastMediaPlaybackTimeKey, &internal_time))
    last_media_playback_time_ = base::Time::FromInternalValue(internal_time);

  // Recalculate the total score and high bit. If the high bit changed we
  // should commit this. This should only happen if we change the threshold
  // or if we have data from before the bit was introduced.
  bool was_high = is_high_;
  Recalculate();
  if (is_high_ != was_high) {
    if (settings_map_) {
      Commit();
    } else {
      // Update the internal dictionary for testing.
      score_dict_->SetBoolean(kHasHighScoreKey, is_high_);
    }
  }
}

// TODO(beccahughes): Add typemap.
media::mojom::MediaEngagementScoreDetailsPtr
MediaEngagementScore::GetScoreDetails() const {
  return media::mojom::MediaEngagementScoreDetails::New(
      origin_, actual_score(), visits(), media_playbacks(),
      last_media_playback_time().ToJsTime(), high_score(), audible_playbacks(),
      significant_playbacks(), high_score_changes());
}

MediaEngagementScore::~MediaEngagementScore() = default;

MediaEngagementScore::MediaEngagementScore(MediaEngagementScore&&) = default;
MediaEngagementScore& MediaEngagementScore::operator=(MediaEngagementScore&&) =
    default;

void MediaEngagementScore::Commit() {
  DCHECK(settings_map_);
  if (!UpdateScoreDict())
    return;

  settings_map_->SetWebsiteSettingDefaultScope(
      origin_, GURL(), CONTENT_SETTINGS_TYPE_MEDIA_ENGAGEMENT,
      content_settings::ResourceIdentifier(), std::move(score_dict_));
}

void MediaEngagementScore::IncrementMediaPlaybacks() {
  SetMediaPlaybacks(media_playbacks() + 1);
  last_media_playback_time_ = clock_->Now();
}

bool MediaEngagementScore::UpdateScoreDict() {
  int stored_visits = 0;
  int stored_media_playbacks = 0;
  double stored_last_media_playback_internal = 0;
  bool is_high = false;
  int stored_audible_playbacks = 0;
  int stored_significant_playbacks = 0;
  int stored_visits_with_media_tag = 0;
  int high_score_changes = 0;

  score_dict_->GetInteger(kVisitsKey, &stored_visits);
  score_dict_->GetInteger(kMediaPlaybacksKey, &stored_media_playbacks);
  score_dict_->GetDouble(kLastMediaPlaybackTimeKey,
                         &stored_last_media_playback_internal);
  score_dict_->GetBoolean(kHasHighScoreKey, &is_high);
  score_dict_->GetInteger(kAudiblePlaybacksKey, &stored_audible_playbacks);
  score_dict_->GetInteger(kSignificantPlaybacksKey,
                          &stored_significant_playbacks);
  score_dict_->GetInteger(kVisitsWithMediaTagKey,
                          &stored_visits_with_media_tag);
  score_dict_->GetInteger(kHighScoreChanges, &high_score_changes);

  bool changed = stored_visits != visits() ||
                 stored_media_playbacks != media_playbacks() ||
                 is_high_ != is_high ||
                 stored_audible_playbacks != audible_playbacks() ||
                 stored_significant_playbacks != significant_playbacks() ||
                 stored_visits_with_media_tag != visits_with_media_tag() ||
                 stored_last_media_playback_internal !=
                     last_media_playback_time_.ToInternalValue();

  // Do not check for high_score_changes_ change as it MUST happen only if
  // `changed` is true (ie. it can't happen alone).
  DCHECK(high_score_changes == high_score_changes_ || changed);

  if (!changed)
    return false;

  if (is_high_ != is_high)
    ++high_score_changes_;

  score_dict_->SetInteger(kVisitsKey, visits_);
  score_dict_->SetInteger(kMediaPlaybacksKey, media_playbacks_);
  score_dict_->SetDouble(kLastMediaPlaybackTimeKey,
                         last_media_playback_time_.ToInternalValue());
  score_dict_->SetBoolean(kHasHighScoreKey, is_high_);
  score_dict_->SetInteger(kAudiblePlaybacksKey, audible_playbacks_);
  score_dict_->SetInteger(kSignificantPlaybacksKey, significant_playbacks_);
  score_dict_->SetInteger(kVisitsWithMediaTagKey, visits_with_media_tag_);
  score_dict_->SetInteger(kHighScoreChanges, high_score_changes_);

  return true;
}

void MediaEngagementScore::Recalculate() {
  // Use the minimum visits to compute the score to allow websites that would
  // surely have a high MEI to pass the bar early.
  double effective_visits = std::max(visits(), GetScoreMinVisits());
  actual_score_ = static_cast<double>(media_playbacks()) / effective_visits;

  // Recalculate whether the engagement score is considered high.
  if (is_high_) {
    is_high_ = actual_score_ >= GetHighScoreLowerThreshold();
  } else {
    is_high_ = actual_score_ >= GetHighScoreUpperThreshold();
  }
}

void MediaEngagementScore::SetVisits(int visits) {
  visits_ = visits;
  Recalculate();
}

void MediaEngagementScore::SetMediaPlaybacks(int media_playbacks) {
  media_playbacks_ = media_playbacks;
  Recalculate();
}
