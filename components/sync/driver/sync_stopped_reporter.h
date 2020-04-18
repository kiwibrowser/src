// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SYNC_STOPPED_REPORTER_H_
#define COMPONENTS_SYNC_DRIVER_SYNC_STOPPED_REPORTER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace syncer {

// Manages informing the sync server that sync has been disabled.
// An implementation of URLFetcherDelegate was needed in order to
// clean up the fetcher_ pointer when the request completes.
class SyncStoppedReporter : public net::URLFetcherDelegate {
 public:
  enum Result { RESULT_SUCCESS, RESULT_ERROR, RESULT_TIMEOUT };

  using ResultCallback = base::Callback<void(const Result&)>;

  SyncStoppedReporter(
      const GURL& sync_service_url,
      const std::string& user_agent,
      const scoped_refptr<net::URLRequestContextGetter>& request_context,
      const ResultCallback& callback);
  ~SyncStoppedReporter() override;

  // Inform the sync server that sync was stopped on this device.
  // |access_token|, |cache_guid|, and |birthday| must not be empty.
  void ReportSyncStopped(const std::string& access_token,
                         const std::string& cache_guid,
                         const std::string& birthday);

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  // Convert the base sync URL into the sync event URL.
  static GURL GetSyncEventURL(const GURL& sync_service_url);

  // Callback for a request timing out.
  void OnTimeout();

  // Handles timing out requests.
  base::OneShotTimer timer_;

  // The URL for the sync server's event RPC.
  const GURL sync_event_url_;

  // The user agent for the browser.
  const std::string user_agent_;

  // Stored to simplify the API; needed for URLFetcher::Create().
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  // The current URLFetcher. Null unless a request is in progress.
  std::unique_ptr<net::URLFetcher> fetcher_;

  // A callback for request completion or timeout.
  ResultCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(SyncStoppedReporter);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SYNC_STOPPED_REPORTER_H_
