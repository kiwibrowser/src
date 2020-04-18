// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SCORE_H_
#define CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SCORE_H_

#include <memory>

#include "base/macros.h"
#include "base/time/clock.h"
#include "base/values.h"
#include "chrome/browser/media/media_engagement_score_details.mojom.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "url/gurl.h"

class HostContentSettingsMap;

// Calculates and stores the Media Engagement Index score for a certain origin.
class MediaEngagementScore final {
 public:
  // The dictionary keys to store individual metrics. kVisitsKey will store the
  // number of visits to an origin and kMediaPlaybacksKey will store the number
  // of media playbacks on an origin. kLastMediaPlaybackTimeKey will store the
  // timestamp of the last media playback on an origin. kHasHighScoreKey will
  // store whether the score is considered high. kAudiblePlaybacksKey will
  // store the number of audible playbacks on an origin.
  // kSignificantPlaybacksKey will store the number of significant playbacks
  // on an origin. kVisitsWithMediaTagKey will store the number of visits to
  // an origin where at least one page had a media tag.
  static const char kVisitsKey[];
  static const char kMediaPlaybacksKey[];
  static const char kLastMediaPlaybackTimeKey[];
  static const char kHasHighScoreKey[];
  static const char kAudiblePlaybacksKey[];
  static const char kSignificantPlaybacksKey[];
  static const char kVisitsWithMediaTagKey[];
  static const char kHighScoreChanges[];

  // Origins with a number of visits less than this number will recieve
  // a score of zero.
  static int GetScoreMinVisits();

  // The upper and lower threshold of whether the total score is considered
  // to be high.
  static double GetHighScoreLowerThreshold();
  static double GetHighScoreUpperThreshold();

  MediaEngagementScore(base::Clock* clock,
                       const GURL& origin,
                       HostContentSettingsMap* settings);
  ~MediaEngagementScore();

  MediaEngagementScore(MediaEngagementScore&&);
  MediaEngagementScore& operator=(MediaEngagementScore&&);

  // Returns the total score, as per the formula.
  double actual_score() const { return actual_score_; }

  // Returns whether the total score is considered high.
  bool high_score() const { return is_high_; }

  // Returns the number of times the high engagement bit was changed.
  int high_score_changes() const { return high_score_changes_; }

  // Returns the origin associated with this score.
  const GURL& origin() const { return origin_; }

  // Writes the values in this score into |settings_map_|. If there are multiple
  // instances of a score object for an origin, this could result in stale data
  // being stored.
  void Commit();

  // Get/increment the number of visits this origin has had.
  int visits() const { return visits_; }
  void IncrementVisits() { SetVisits(visits() + 1); }

  // Get/increment the number of media playbacks this origin has had.
  int media_playbacks() const { return media_playbacks_; }
  void IncrementMediaPlaybacks();

  // Get the last time media was played on this origin.
  base::Time last_media_playback_time() const {
    return last_media_playback_time_;
  }

  // Get/increment the number of audible media playbacks this origin had.
  int audible_playbacks() const { return audible_playbacks_; }
  void IncrementAudiblePlaybacks(int amount) {
    set_audible_playbacks(audible_playbacks_ + amount);
  }

  // Get/increment the number of significant media playbacks this origin had.
  int significant_playbacks() const { return significant_playbacks_; }
  void IncrementSignificantPlaybacks(int amount) {
    set_significant_playbacks(significant_playbacks_ + amount);
  }

  // Get/increment the number of visits where at least one page had a media tag.
  int visits_with_media_tag() const { return visits_with_media_tag_; }
  void IncrementVisitsWithMediaTag() {
    set_visits_with_media_tag(visits_with_media_tag_ + 1);
  }

  // Get a breakdown of the score that can be serialized by Mojo.
  media::mojom::MediaEngagementScoreDetailsPtr GetScoreDetails() const;

 protected:
  friend class MediaEngagementAutoplayBrowserTest;
  friend class MediaEngagementContentsObserverTest;
  friend class MediaEngagementSessionTest;
  friend class MediaEngagementService;

  // Only used by the Media Engagement service when bulk loading data.
  MediaEngagementScore(base::Clock* clock,
                       const GURL& origin,
                       std::unique_ptr<base::DictionaryValue> score_dict,
                       HostContentSettingsMap* settings);

  static const char kScoreMinVisitsParamName[];
  static const char kHighScoreLowerThresholdParamName[];
  static const char kHighScoreUpperThresholdParamName[];

  void SetVisits(int visits);
  void SetMediaPlaybacks(int media_playbacks);

  void set_audible_playbacks(int playbacks) { audible_playbacks_ = playbacks; }
  void set_significant_playbacks(int playbacks) {
    significant_playbacks_ = playbacks;
  }
  void set_visits_with_media_tag(int visits) {
    visits_with_media_tag_ = visits;
  }
  void set_last_media_playback_time(base::Time new_time) {
    last_media_playback_time_ = new_time;
  }

 private:
  friend class MediaEngagementServiceTest;
  friend class MediaEngagementScoreTest;

  // Used for tests.
  MediaEngagementScore(base::Clock* clock,
                       const GURL& origin,
                       std::unique_ptr<base::DictionaryValue> score_dict);

  // Update the dictionary continaing the latest score values and return whether
  // they have changed or not (since what was last retrieved from content
  // settings).
  bool UpdateScoreDict();

  // If the number of playbacks or visits is updated then this will recalculate
  // the total score and whether the score is considered high.
  void Recalculate();

  // The number of media playbacks this origin has had.
  int media_playbacks_ = 0;

  // The number of visits this origin has had.
  int visits_ = 0;

  // If the current score is considered high.
  bool is_high_ = false;

  // Number of times the high engagement bit was changed for this origin.
  int high_score_changes_ = 0;

  // The current engagement score.
  double actual_score_ = 0.0;

  // The number of audible media playbacks this origin had.
  int audible_playbacks_ = 0;

  // The number of significant media playbacks this origin had.
  int significant_playbacks_ = 0;

  // The number of visits with a media tag this origin had.
  int visits_with_media_tag_ = 0;

  // The last time media was played back on this origin.
  base::Time last_media_playback_time_;

  // The origin this score represents.
  GURL origin_;

  // A clock that can be used for testing, owned by the service.
  base::Clock* clock_;

  // The dictionary that represents this engagement score.
  std::unique_ptr<base::DictionaryValue> score_dict_;

  // The content settings map that will persist the score,
  // has a lifetime of the Profile like the service which owns |this|.
  HostContentSettingsMap* settings_map_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(MediaEngagementScore);
};

#endif  // CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SCORE_H_
