// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SERVICE_H_
#define CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SERVICE_H_

#include <map>
#include <set>

#include "base/macros.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/values.h"
#include "chrome/browser/media/media_engagement_score.h"
#include "chrome/browser/media/media_engagement_score_details.mojom.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"

class GURL;
class MediaEngagementContentsObserver;
class MediaEngagementScore;
class Profile;

namespace base {
class Clock;
}

namespace content {
class WebContents;
}  // namespace content

namespace history {
class HistoryService;
}

class MediaEngagementService : public KeyedService,
                               public history::HistoryServiceObserver {
 public:
  // Returns the instance attached to the given |profile|.
  static MediaEngagementService* Get(Profile* profile);

  // Returns whether the feature is enabled.
  static bool IsEnabled();

  // Observe the given |web_contents| by creating an internal
  // WebContentsObserver.
  static void CreateWebContentsObserver(content::WebContents* web_contents);

  // Register profile prefs in the pref registry.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  explicit MediaEngagementService(Profile* profile);
  ~MediaEngagementService() override;

  // Returns the engagement score of |url|.
  double GetEngagementScore(const GURL& url) const;

  // Returns true if |url| has an engagement score considered high.
  bool HasHighEngagement(const GURL& url) const;

  // Returns a map of all stored origins and their engagement levels.
  std::map<GURL, double> GetScoreMapForTesting() const;

  // Record a visit of a |url|.
  void RecordVisit(const GURL& url);

  // Record a media playback on a |url|.
  void RecordPlayback(const GURL& url);

  // Returns an array of engagement score details for all origins which
  // have a score.
  std::vector<media::mojom::MediaEngagementScoreDetailsPtr> GetAllScoreDetails()
      const;

  // Overridden from history::HistoryServiceObserver:
  void OnURLsDeleted(history::HistoryService* history_service,
                     const history::DeletionInfo& deletion_info) override;

  // KeyedService support:
  void Shutdown() override;

  // Clear data if the last playback time is between these two time points.
  void ClearDataBetweenTime(const base::Time& delete_begin,
                            const base::Time& delete_end);

  // Retrieves the MediaEngagementScore for |url|.
  MediaEngagementScore CreateEngagementScore(const GURL& url) const;

  MediaEngagementContentsObserver* GetContentsObserverFor(
      content::WebContents* web_contents) const;

  Profile* profile() const;

  // The name of the histogram that scores are logged to on startup.
  static const char kHistogramScoreAtStartupName[];

  // The name of the histogram that records the reduction in score when history
  // is cleared.
  static const char kHistogramURLsDeletedScoreReductionName[];

  // The name of the histogram that records the reason why the engagement was
  // cleared, either partially or fully.
  static const char kHistogramClearName[];

 private:
  friend class MediaEngagementBrowserTest;
  friend class MediaEngagementContentsObserverTest;
  friend class MediaEngagementServiceTest;
  friend class MediaEngagementSessionTest;
  friend class MediaEngagementContentsObserver;

  MediaEngagementService(Profile* profile, base::Clock* clock);

  // Returns true if we should record engagement for this url. Currently,
  // engagement is only earned for HTTP and HTTPS.
  bool ShouldRecordEngagement(const GURL& url) const;

  base::flat_map<content::WebContents*, MediaEngagementContentsObserver*>
      contents_observers_;

  Profile* profile_;

  // Clear any data for a specific origin.
  void Clear(const GURL& url);

  // An internal clock for testing.
  base::Clock* clock_;

  // Records all the stored scores to a histogram.
  void RecordStoredScoresToHistogram();

  std::vector<MediaEngagementScore> GetAllStoredScores() const;

  int GetSchemaVersion() const;
  void SetSchemaVersion(int);

  // Remove origins from `deleted_origins` that have no more visits in the
  // history service, represented as `origin_data`. This is meant to be used
  // when the service receives a notification of history expiration.
  void RemoveOriginsWithNoVisits(
      const std::set<GURL>& deleted_origins,
      const history::OriginCountAndLastVisitMap& origin_data);

  // Allows us to cancel the RecordScoresToHistogram task if we are destroyed.
  base::CancelableTaskTracker task_tracker_;

  DISALLOW_COPY_AND_ASSIGN(MediaEngagementService);
};

#endif  // CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SERVICE_H_
