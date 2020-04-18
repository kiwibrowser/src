// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_URL_FETCHER_DOWNLOADER_H_
#define COMPONENTS_UPDATE_CLIENT_URL_FETCHER_DOWNLOADER_H_

#include <stdint.h>

#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/update_client/crx_downloader.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

namespace update_client {

// Implements a CRX downloader in top of the URLFetcher.
class UrlFetcherDownloader : public CrxDownloader {
 public:
  UrlFetcherDownloader(
      std::unique_ptr<CrxDownloader> successor,
      scoped_refptr<net::URLRequestContextGetter> context_getter);
  ~UrlFetcherDownloader() override;

 private:
  class URLFetcherDelegate : public net::URLFetcherDelegate {
   public:
    explicit URLFetcherDelegate(UrlFetcherDownloader* downloader);
    ~URLFetcherDelegate() override;

   private:
    // Overrides for URLFetcherDelegate.
    void OnURLFetchComplete(const net::URLFetcher* source) override;
    void OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                    int64_t current,
                                    int64_t total,
                                    int64_t current_network_bytes) override;
    // Not owned by this class.
    UrlFetcherDownloader* downloader_ = nullptr;
    DISALLOW_COPY_AND_ASSIGN(URLFetcherDelegate);
  };

  // Overrides for CrxDownloader.
  void DoStartDownload(const GURL& url) override;

  void CreateDownloadDir();
  void StartURLFetch(const GURL& url);
  void OnURLFetchComplete(const net::URLFetcher* source);
  void OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                  int64_t current,
                                  int64_t total,
                                  int64_t current_network_bytes);

  THREAD_CHECKER(thread_checker_);

  std::unique_ptr<URLFetcherDelegate> delegate_;

  std::unique_ptr<net::URLFetcher> url_fetcher_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;

  // Contains a temporary download directory for the downloaded file.
  base::FilePath download_dir_;

  base::TimeTicks download_start_time_;

  int64_t downloaded_bytes_ = -1;
  int64_t total_bytes_ = -1;

  DISALLOW_COPY_AND_ASSIGN(UrlFetcherDownloader);
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_URL_FETCHER_DOWNLOADER_H_
