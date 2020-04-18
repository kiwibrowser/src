// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_CRX_DOWNLOADER_H_
#define COMPONENTS_UPDATE_CLIENT_CRX_DOWNLOADER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "url/gurl.h"


namespace net {
class URLRequestContextGetter;
}

namespace update_client {

// Defines a download interface for downloading components, with retrying on
// fallback urls in case of errors. This class implements a chain of
// responsibility design pattern. It can give successors in the chain a chance
// to handle a download request, until one of them succeeds, or there are no
// more urls or successors to try. A callback is always called at the end of
// the download, one time only.
// When multiple urls and downloaders exists, first all the urls are tried, in
// the order they are provided in the StartDownload function argument. After
// that, the download request is routed to the next downloader in the chain.
// The members of this class expect to be called from the main thread only.
class CrxDownloader {
 public:
  struct DownloadMetrics {
    enum Downloader { kNone = 0, kUrlFetcher, kBits };

    DownloadMetrics();

    GURL url;

    Downloader downloader;

    int error;

    int64_t downloaded_bytes;  // -1 means that the byte count is unknown.
    int64_t total_bytes;

    uint64_t download_time_ms;
  };

  // Contains the progress or the outcome of the download.
  struct Result {
    Result();

    // Download error: 0 indicates success.
    int error;

    // Path of the downloaded file if the download was successful.
    base::FilePath response;

    // Number of bytes actually downloaded, not including the bytes downloaded
    // as a result of falling back on urls.
    int64_t downloaded_bytes;

    // Number of bytes expected to be downloaded.
    int64_t total_bytes;
  };

  // The callback fires only once, regardless of how many urls are tried, and
  // how many successors in the chain of downloaders have handled the
  // download. The callback interface can be extended if needed to provide
  // more visibility into how the download has been handled, including
  // specific error codes and download metrics.
  using DownloadCallback = base::OnceCallback<void(const Result& result)>;

  // The callback may fire 0 or many times during a download. Since this
  // class implements a chain of responsibility, the callback can fire for
  // different urls and different downloaders. The number of actual downloaded
  // bytes is not guaranteed to monotonically increment over time.
  using ProgressCallback = base::Callback<void(const Result& result)>;

  using Factory = std::unique_ptr<CrxDownloader> (*)(
      bool,
      scoped_refptr<net::URLRequestContextGetter>);

  // Factory method to create an instance of this class and build the
  // chain of responsibility. |is_background_download| specifies that a
  // background downloader be used, if the platform supports it.
  // |task_runner| should be a task runner able to run blocking
  // code such as file IO operations.
  static std::unique_ptr<CrxDownloader> Create(
      bool is_background_download,
      scoped_refptr<net::URLRequestContextGetter> context_getter);
  virtual ~CrxDownloader();

  void set_progress_callback(const ProgressCallback& progress_callback);

  // Starts the download. One instance of the class handles one download only.
  // One instance of CrxDownloader can only be started once, otherwise the
  // behavior is undefined. The callback gets invoked if the download can't
  // be started. |expected_hash| represents the SHA256 cryptographic hash of
  // the download payload, represented as a hexadecimal string.
  void StartDownloadFromUrl(const GURL& url,
                            const std::string& expected_hash,
                            DownloadCallback download_callback);
  void StartDownload(const std::vector<GURL>& urls,
                     const std::string& expected_hash,
                     DownloadCallback download_callback);

  const std::vector<DownloadMetrics> download_metrics() const;

 protected:
  explicit CrxDownloader(std::unique_ptr<CrxDownloader> successor);

  // Handles the fallback in the case of multiple urls and routing of the
  // download to the following successor in the chain. Derived classes must call
  // this function after each attempt at downloading the urls provided
  // in the StartDownload function.
  // In case of errors, |is_handled| indicates that a server side error has
  // occured for the current url and the url should not be retried down
  // the chain to avoid DDOS of the server. This url will be removed from the
  // list of url and never tried again.
  void OnDownloadComplete(bool is_handled,
                          const Result& result,
                          const DownloadMetrics& download_metrics);

  // Calls the callback when progress is made.
  void OnDownloadProgress(const Result& result);

  // Returns the url which is currently being downloaded from.
  GURL url() const;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner() const {
    return main_task_runner_;
  }

 private:
  virtual void DoStartDownload(const GURL& url) = 0;

  void VerifyResponse(bool is_handled,
                      Result result,
                      DownloadMetrics download_metrics);

  void HandleDownloadError(bool is_handled,
                           const Result& result,
                           const DownloadMetrics& download_metrics);

  base::ThreadChecker thread_checker_;

  // Used to post callbacks to the main thread.
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  std::vector<GURL> urls_;

  // The SHA256 hash of the download payload in hexadecimal format.
  std::string expected_hash_;
  std::unique_ptr<CrxDownloader> successor_;
  DownloadCallback download_callback_;
  ProgressCallback progress_callback_;

  std::vector<GURL>::iterator current_url_;

  std::vector<DownloadMetrics> download_metrics_;

  DISALLOW_COPY_AND_ASSIGN(CrxDownloader);
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_CRX_DOWNLOADER_H_
