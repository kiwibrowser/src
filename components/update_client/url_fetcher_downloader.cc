// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/url_fetcher_downloader.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/update_client/utils.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace {

constexpr base::TaskTraits kTaskTraits = {
    base::MayBlock(), base::TaskPriority::BACKGROUND,
    base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN};

}  // namespace

namespace update_client {

UrlFetcherDownloader::URLFetcherDelegate::URLFetcherDelegate(
    UrlFetcherDownloader* downloader)
    : downloader_(downloader) {}

UrlFetcherDownloader::URLFetcherDelegate::~URLFetcherDelegate() = default;

void UrlFetcherDownloader::URLFetcherDelegate::OnURLFetchComplete(
    const net::URLFetcher* source) {
  downloader_->OnURLFetchComplete(source);
}

void UrlFetcherDownloader::URLFetcherDelegate::OnURLFetchDownloadProgress(
    const net::URLFetcher* source,
    int64_t current,
    int64_t total,
    int64_t current_network_bytes) {
  downloader_->OnURLFetchDownloadProgress(source, current, total,
                                          current_network_bytes);
}

UrlFetcherDownloader::UrlFetcherDownloader(
    std::unique_ptr<CrxDownloader> successor,
    scoped_refptr<net::URLRequestContextGetter> context_getter)
    : CrxDownloader(std::move(successor)),
      delegate_(std::make_unique<URLFetcherDelegate>(this)),
      context_getter_(context_getter) {}

UrlFetcherDownloader::~UrlFetcherDownloader() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void UrlFetcherDownloader::DoStartDownload(const GURL& url) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  base::PostTaskWithTraitsAndReply(
      FROM_HERE, kTaskTraits,
      base::BindOnce(&UrlFetcherDownloader::CreateDownloadDir,
                     base::Unretained(this)),
      base::BindOnce(&UrlFetcherDownloader::StartURLFetch,
                     base::Unretained(this), url));
}

void UrlFetcherDownloader::CreateDownloadDir() {
  base::CreateNewTempDirectory(FILE_PATH_LITERAL("chrome_url_fetcher_"),
                               &download_dir_);
}

void UrlFetcherDownloader::StartURLFetch(const GURL& url) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("url_fetcher_downloader", R"(
        semantics {
          sender: "Component Updater"
          description:
            "The component updater in Chrome is responsible for updating code "
            "and data modules such as Flash, CrlSet, Origin Trials, etc. These "
            "modules are updated on cycles independent of the Chrome release "
            "tracks. It runs in the browser process and communicates with a "
            "set of servers using the Omaha protocol to find the latest "
            "versions of components, download them, and register them with the "
            "rest of Chrome."
          trigger: "Manual or automatic software updates."
          data:
            "The URL that refers to a component. It is obfuscated for most "
            "components."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled."
          chrome_policy {
            ComponentUpdatesEnabled {
              policy_options {mode: MANDATORY}
              ComponentUpdatesEnabled: false
            }
          }
        })");

  if (download_dir_.empty()) {
    Result result;
    result.error = -1;
    result.downloaded_bytes = downloaded_bytes_;
    result.total_bytes = total_bytes_;

    DownloadMetrics download_metrics;
    download_metrics.url = url;
    download_metrics.downloader = DownloadMetrics::kUrlFetcher;
    download_metrics.error = -1;
    download_metrics.downloaded_bytes = downloaded_bytes_;
    download_metrics.total_bytes = total_bytes_;
    download_metrics.download_time_ms = 0;

    main_task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&UrlFetcherDownloader::OnDownloadComplete,
                                  base::Unretained(this), false, result,
                                  download_metrics));
    return;
  }

  const base::FilePath response =
      download_dir_.AppendASCII(url.ExtractFileName());

  url_fetcher_ = net::URLFetcher::Create(0, url, net::URLFetcher::GET,
                                         delegate_.get(), traffic_annotation);
  url_fetcher_->SetRequestContext(context_getter_.get());
  url_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DISABLE_CACHE);
  url_fetcher_->SetAutomaticallyRetryOn5xx(false);
  url_fetcher_->SetAutomaticallyRetryOnNetworkChanges(3);
  url_fetcher_->SaveResponseToFileAtPath(
      response, base::CreateSequencedTaskRunnerWithTraits(kTaskTraits));
  data_use_measurement::DataUseUserData::AttachToFetcher(
      url_fetcher_.get(), data_use_measurement::DataUseUserData::UPDATE_CLIENT);

  VLOG(1) << "Starting background download: " << url.spec();
  url_fetcher_->Start();

  download_start_time_ = base::TimeTicks::Now();
}

void UrlFetcherDownloader::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  const base::TimeTicks download_end_time(base::TimeTicks::Now());
  const base::TimeDelta download_time =
      download_end_time >= download_start_time_
          ? download_end_time - download_start_time_
          : base::TimeDelta();

  // Consider a 5xx response from the server as an indication to terminate
  // the request and avoid overloading the server in this case.
  // is not accepting requests for the moment.
  const int fetch_error(GetFetchError(*url_fetcher_));
  const bool is_handled = fetch_error == 0 || IsHttpServerError(fetch_error);

  Result result;
  result.error = fetch_error;
  if (!fetch_error) {
    source->GetResponseAsFilePath(true, &result.response);
  }
  result.downloaded_bytes = downloaded_bytes_;
  result.total_bytes = total_bytes_;

  DownloadMetrics download_metrics;
  download_metrics.url = url();
  download_metrics.downloader = DownloadMetrics::kUrlFetcher;
  download_metrics.error = fetch_error;
  download_metrics.downloaded_bytes = downloaded_bytes_;
  download_metrics.total_bytes = total_bytes_;
  download_metrics.download_time_ms = download_time.InMilliseconds();

  VLOG(1) << "Downloaded " << downloaded_bytes_ << " bytes in "
          << download_time.InMilliseconds() << "ms from "
          << source->GetURL().spec() << " to " << result.response.value();

  // Delete the download directory in the error cases.
  if (fetch_error && !download_dir_.empty())
    base::PostTaskWithTraits(
        FROM_HERE, kTaskTraits,
        base::BindOnce(IgnoreResult(&base::DeleteFile), download_dir_, true));

  main_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&UrlFetcherDownloader::OnDownloadComplete,
                                base::Unretained(this), is_handled, result,
                                download_metrics));
}

void UrlFetcherDownloader::OnURLFetchDownloadProgress(
    const net::URLFetcher* source,
    int64_t current,
    int64_t total,
    int64_t current_network_bytes) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  downloaded_bytes_ = current;
  total_bytes_ = total;

  Result result;
  result.downloaded_bytes = downloaded_bytes_;
  result.total_bytes = total_bytes_;

  OnDownloadProgress(result);
}

}  // namespace update_client
