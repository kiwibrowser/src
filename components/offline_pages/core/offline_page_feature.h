// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_FEATURE_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_FEATURE_H_

#include "base/feature_list.h"
#include "build/build_config.h"

namespace offline_pages {

extern const base::Feature kOfflineBookmarksFeature;
extern const base::Feature kOffliningRecentPagesFeature;
extern const base::Feature kOfflinePagesSvelteConcurrentLoadingFeature;
extern const base::Feature kOfflinePagesCTFeature;
extern const base::Feature kOfflinePagesSharingFeature;
extern const base::Feature kBackgroundLoaderForDownloadsFeature;
extern const base::Feature kPrefetchingOfflinePagesFeature;
extern const base::Feature kOfflinePagesLoadSignalCollectingFeature;
extern const base::Feature kOfflinePagesCTV2Feature;
extern const base::Feature kOfflinePagesRenovationsFeature;
extern const base::Feature kOfflinePagesResourceBasedSnapshotFeature;
extern const base::Feature kOfflinePagesPrefetchingUIFeature;
extern const base::Feature kOfflinePagesLimitlessPrefetchingFeature;
extern const base::Feature kOfflinePagesDescriptivePendingStatusFeature;
extern const base::Feature kOfflinePagesInDownloadHomeOpenInCctFeature;
extern const base::Feature kOfflinePagesDescriptiveFailStatusFeature;
extern const base::Feature kOfflinePagesCTSuppressNotificationsFeature;
extern const base::Feature kOfflinePagesShowAlternateDinoPageFeature;

// The parameter name used to find the experiment tag for prefetching offline
// pages.
extern const char kPrefetchingOfflinePagesExperimentsOption[];

// Returns true if saving bookmarked pages for offline viewing is enabled.
bool IsOfflineBookmarksEnabled();

// Returns true if offlining of recent pages (aka 'Last N pages') is enabled.
bool IsOffliningRecentPagesEnabled();

// Returns true if offline CT features are enabled.  See crbug.com/620421.
bool IsOfflinePagesCTEnabled();

// Returns true if offline page sharing is enabled.
bool IsOfflinePagesSharingEnabled();

// Returns true if saving a foreground tab that is taking too long using the
// background scheduler is enabled.
bool IsBackgroundLoaderForDownloadsEnabled();

// Returns true if concurrent background loading is enabled for svelte.
bool IsOfflinePagesSvelteConcurrentLoadingEnabled();

// Returns true if prefetching offline pages is enabled.
bool IsPrefetchingOfflinePagesEnabled();

// Returns true if we should show UI for prefetched pages.
bool IsOfflinePagesPrefetchingUIEnabled();

// Returns true if prefetching offline pages should ignore its normal resource
// usage limits.
bool IsLimitlessPrefetchingEnabled();

// Returns true if we enable load timing signals to be collected.
bool IsOfflinePagesLoadSignalCollectingEnabled();

// Returns true if we should use the "page renovation" framework in
// the BackgroundLoaderOffliner.
bool IsOfflinePagesRenovationsEnabled();

// Returns true if we should use the "Resource percentage signal" for taking
// snapshots instead of a time delay after the document is loaded in the main
// frame.
bool IsOfflinePagesResourceBasedSnapshotEnabled();

// Returns true if a command line for test has been set that shortens the
// snapshot delay.
bool ShouldUseTestingSnapshotDelay();

// Returns true if we should record request origin as part of custom tabs V2.
bool IsOfflinePagesCTV2Enabled();

// Returns true if descriptive failed download status texts should be used in
// notifications and Downloads Home.
bool IsOfflinePagesDescriptiveFailStatusEnabled();

// Returns true if descriptive pending download status texts should be used in
// notifications and Downloads Home.
bool IsOfflinePagesDescriptivePendingStatusEnabled();

// Controls whether offline pages opened from the Downloads Home should be
// opened in CCTs instead of new tabs.
bool ShouldOfflinePagesInDownloadHomeOpenInCct();

// Returns true if we should suppress completed notifications for certain custom
// tabs downloads.
bool IsOfflinePagesSuppressNotificationsEnabled();

// Controls whether we should show a dinosaur page with alternate UI.
bool ShouldShowAlternateDinoPage();

// Returns an experiment tag provided by the field trial. This experiment tag
// will be included in a custom header in all requests sent to Offline Prefetch
// Server. The server will use this this optional tag to decide how to process
// the request.
std::string GetPrefetchingOfflinePagesExperimentTag();

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_OFFLINE_PAGE_FEATURE_H_
