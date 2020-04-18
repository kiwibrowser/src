// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREDICTORS_LOADING_DATA_COLLECTOR_H_
#define CHROME_BROWSER_PREDICTORS_LOADING_DATA_COLLECTOR_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/predictors/loading_predictor_config.h"
#include "chrome/browser/predictors/resource_prefetch_common.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

namespace net {
class URLRequest;
}

namespace predictors {

class LoadingStatsCollector;
class ResourcePrefetchPredictor;

// Data collected for origin-based prediction, for a single origin during a
// page load (see PageRequestSummary).
struct OriginRequestSummary {
  OriginRequestSummary();
  OriginRequestSummary(const OriginRequestSummary& other);
  ~OriginRequestSummary();

  GURL origin;
  bool always_access_network = false;
  bool accessed_network = false;
  int first_occurrence = 0;
};

// Stores the data that we need to get from the URLRequest.
struct URLRequestSummary {
  URLRequestSummary();
  URLRequestSummary(const URLRequestSummary& other);
  ~URLRequestSummary();

  NavigationID navigation_id;
  GURL request_url;  // URL after all redirects.
  GURL redirect_url;  // Empty unless request was redirected to a valid url.
  content::ResourceType resource_type = content::RESOURCE_TYPE_LAST_TYPE;

  bool always_revalidate = false;
  bool is_no_store = false;
  bool network_accessed = false;

  // Initializes a |URLRequestSummary| from a |URLRequest| response.
  // Returns true for success. Note: NavigationID is NOT initialized
  // by this function.
  static bool SummarizeResponse(const net::URLRequest& request,
                                URLRequestSummary* summary);
};

// Stores the data learned from a single navigation.
struct PageRequestSummary {
  explicit PageRequestSummary(const GURL& main_frame_url);
  PageRequestSummary(const PageRequestSummary& other);
  void UpdateOrAddToOrigins(const URLRequestSummary& request_summary);
  ~PageRequestSummary();

  GURL main_frame_url;
  GURL initial_url;
  base::TimeTicks first_contentful_paint;

  // Map of origin -> OriginRequestSummary. Only one instance of each origin
  // is kept per navigation, but the summary is updated several times.
  std::map<GURL, OriginRequestSummary> origins;
};

// Records navigation events as reported by various observers to the database
// and stats collection classes. All the non-static methods of this class need
// to be called on the UI thread.
class LoadingDataCollector {
 public:
  explicit LoadingDataCollector(
      predictors::ResourcePrefetchPredictor* predictor,
      predictors::LoadingStatsCollector* stats_collector,
      const LoadingPredictorConfig& config);
  ~LoadingDataCollector();

  // Determines the resource type from the declared one, falling back to MIME
  // type detection when it is not explicit.
  static content::ResourceType GetResourceType(
      content::ResourceType resource_type,
      const std::string& mime_type);

  // Thread safe.
  static bool ShouldRecordRequest(net::URLRequest* request,
                                  content::ResourceType resource_type);
  static bool ShouldRecordResponse(net::URLRequest* response);
  static bool ShouldRecordRedirect(net::URLRequest* response);
  static bool ShouldRecordResourceFromMemoryCache(
      const GURL& url,
      content::ResourceType resource_type,
      const std::string& mime_type);

  // 'LoadingPredictorObserver' and 'ResourcePrefetchPredictorTabHelper' call
  // the below functions to inform the collector of main frame and resource
  // requests. Should only be called if the corresponding Should* functions
  // return true.
  void RecordURLRequest(const URLRequestSummary& request);
  void RecordURLResponse(const URLRequestSummary& response);
  void RecordURLRedirect(const URLRequestSummary& response);

  // Called when the main frame of a page completes loading. We treat this point
  // as the "completion" of the navigation. The resources requested by the page
  // up to this point are the only ones considered.
  void RecordMainFrameLoadComplete(const NavigationID& navigation_id);

  // Called after the main frame's first contentful paint.
  void RecordFirstContentfulPaint(
      const NavigationID& navigation_id,
      const base::TimeTicks& first_contentful_paint);

 private:
  using NavigationMap =
      std::map<NavigationID, std::unique_ptr<PageRequestSummary>>;

  friend class LoadingDataCollectorTest;
  friend class ResourcePrefetchPredictorBrowserTest;

  FRIEND_TEST_ALL_PREFIXES(LoadingDataCollectorTest, HandledResourceTypes);
  FRIEND_TEST_ALL_PREFIXES(LoadingDataCollectorTest, SimpleNavigation);
  FRIEND_TEST_ALL_PREFIXES(LoadingDataCollectorTest, SimpleRedirect);
  FRIEND_TEST_ALL_PREFIXES(LoadingDataCollectorTest, OnMainFrameRequest);
  FRIEND_TEST_ALL_PREFIXES(LoadingDataCollectorTest, OnMainFrameRedirect);
  FRIEND_TEST_ALL_PREFIXES(LoadingDataCollectorTest, OnSubresourceResponse);
  FRIEND_TEST_ALL_PREFIXES(LoadingDataCollectorTest,
                           TestRecordFirstContentfulPaint);

  // Determines the ResourceType from the mime type, defaulting to the
  // |fallback| if the ResourceType could not be determined.
  static content::ResourceType GetResourceTypeFromMimeType(
      const std::string& mime_type,
      content::ResourceType fallback);

  // Returns true if the main page request is supported for prediction.
  static bool IsHandledMainPage(net::URLRequest* request);

  // Returns true if the subresource request is supported for prediction.
  static bool IsHandledSubresource(net::URLRequest* request,
                                   content::ResourceType resource_type);

  // Returns true if the subresource has a supported type.
  static bool IsHandledResourceType(content::ResourceType resource_type,
                                    const std::string& mime_type);

  // Returns true if the url could be written into the database.
  static bool IsHandledUrl(const GURL& url);

  static void SetAllowPortInUrlsForTesting(bool state);

  // Functions called on different network events pertaining to the loading of
  // main frame resource or sub resources.
  void OnMainFrameRedirect(const URLRequestSummary& response);
  void OnSubresourceRedirect(const URLRequestSummary& response);

  // Cleanup inflight_navigations_ and call a cleanup for stats_collector_.
  void CleanupAbandonedNavigations(const NavigationID& navigation_id);

  ResourcePrefetchPredictor* const predictor_;
  LoadingStatsCollector* const stats_collector_;
  const LoadingPredictorConfig config_;

  NavigationMap inflight_navigations_;
};

}  // namespace predictors

#endif  // CHROME_BROWSER_PREDICTORS_LOADING_DATA_COLLECTOR_H_
