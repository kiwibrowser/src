// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_fetcher_win.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/guid.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/srt_field_trial_win.h"
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/install_modes.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/version_info/version_info.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace safe_browsing {

namespace {

constexpr char kDownloadStatusErrorCodeHistogramName[] =
    "SoftwareReporter.Cleaner.DownloadStatusErrorCode";

// Indicates the suffix to use for some histograms that depend on the final
// download status. This is used because UMA histogram macros define static
// constant strings to represent the name, so they can't be used when a name
// is dynamically generated. The alternative would be to replicate the logic
// of those macros, which is not ideal.
enum class FetchCompletedReasonHistogramSuffix {
  kDownloadSuccess,
  kDownloadFailure,
  kNetworkError,
};

base::FilePath::StringType CleanerTempDirectoryPrefix() {
  // Create a temporary directory name prefix like "ChromeCleaner_4_", where
  // "Chrome" is the product name and the 4 refers to the install mode of the
  // browser.
  int install_mode = install_static::InstallDetails::Get().install_mode_index();
  return base::StringPrintf(
      FILE_PATH_LITERAL("%" PRFilePath "%" PRFilePath "_%d_"),
      install_static::kProductPathName, FILE_PATH_LITERAL("Cleaner"),
      install_mode);
}

// These values are used to send UMA information and are replicated in the
// histograms.xml file, so the order MUST NOT CHANGE.
enum CleanerDownloadStatusHistogramValue {
  CLEANER_DOWNLOAD_STATUS_SUCCEEDED = 0,
  CLEANER_DOWNLOAD_STATUS_OTHER_FAILURE = 1,
  CLEANER_DOWNLOAD_STATUS_NOT_FOUND_ON_SERVER = 2,
  CLEANER_DOWNLOAD_STATUS_FAILED_TO_CREATE_TEMP_DIR = 3,
  CLEANER_DOWNLOAD_STATUS_FAILED_TO_SAVE_TO_FILE = 4,

  CLEANER_DOWNLOAD_STATUS_MAX,
};

net::NetworkTrafficAnnotationTag kChromeCleanerTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("chrome_cleaner", R"(
      semantics {
        sender: "Chrome Cleaner"
        description:
          "Chrome Cleaner removes unwanted software that violates Google's "
          "unwanted software policy."
        trigger:
          "Chrome reporter detected unwanted software that the Cleaner can "
          "remove."
        data: "No data is sent up, this is just a download."
        destination: GOOGLE_OWNED_SERVICE
      }
      policy {
        cookies_allowed: NO
        setting: "This feature cannot be disabled in settings."
        policy_exception_justification: "Not implemented."
  })");

void RecordCleanerDownloadStatusHistogram(
    CleanerDownloadStatusHistogramValue value) {
  UMA_HISTOGRAM_ENUMERATION("SoftwareReporter.Cleaner.DownloadStatus", value,
                            CLEANER_DOWNLOAD_STATUS_MAX);
}

// Class that will attempt to download the Chrome Cleaner executable and call a
// given callback when done. Instances of ChromeCleanerFetcher own themselves
// and will self-delete if they encounter an error or when the network request
// has completed.
class ChromeCleanerFetcher : public net::URLFetcherDelegate {
 public:
  explicit ChromeCleanerFetcher(ChromeCleanerFetchedCallback fetched_callback);

 protected:
  ~ChromeCleanerFetcher() override;

 private:
  // Must be called on a sequence where IO is allowed.
  bool CreateTemporaryDirectory();
  // Will be called back on the same sequence as this object was created on.
  void OnTemporaryDirectoryCreated(bool success);
  void PostCallbackAndDeleteSelf(base::FilePath path,
                                 ChromeCleanerFetchStatus fetch_status);

  // Sends a histogram indicating an error and invokes the fetch callback if
  // the cleaner binary can't be downloaded or saved to the disk.
  void RecordDownloadStatusAndPostCallback(
      CleanerDownloadStatusHistogramValue histogram_value,
      ChromeCleanerFetchStatus fetch_status);

  void RecordTimeToCompleteDownload(FetchCompletedReasonHistogramSuffix suffix);

  // net::URLFetcherDelegate overrides.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  ChromeCleanerFetchedCallback fetched_callback_;

  // The underlying URL fetcher. The instance is alive from construction through
  // OnURLFetchComplete.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // Used for file operations such as creating a new temporary directory.
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  // We will take ownership of the scoped temp directory once we know that the
  // fetch has succeeded. Must be deleted on a sequence where IO is allowed.
  std::unique_ptr<base::ScopedTempDir, base::OnTaskRunnerDeleter>
      scoped_temp_dir_;
  base::FilePath temp_file_;

  // For metrics reporting.
  base::Time time_fetching_started_;

  DISALLOW_COPY_AND_ASSIGN(ChromeCleanerFetcher);
};

ChromeCleanerFetcher::ChromeCleanerFetcher(
    ChromeCleanerFetchedCallback fetched_callback)
    : fetched_callback_(std::move(fetched_callback)),
      url_fetcher_(net::URLFetcher::Create(0,
                                           GetSRTDownloadURL(),
                                           net::URLFetcher::GET,
                                           this,
                                           kChromeCleanerTrafficAnnotation)),
      blocking_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN})),
      scoped_temp_dir_(new base::ScopedTempDir(),
                       base::OnTaskRunnerDeleter(blocking_task_runner_)) {
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(), FROM_HERE,
      base::Bind(&ChromeCleanerFetcher::CreateTemporaryDirectory,
                 base::Unretained(this)),
      base::Bind(&ChromeCleanerFetcher::OnTemporaryDirectoryCreated,
                 base::Unretained(this)));
}

ChromeCleanerFetcher::~ChromeCleanerFetcher() = default;

bool ChromeCleanerFetcher::CreateTemporaryDirectory() {
  base::FilePath temp_dir;
  return base::CreateNewTempDirectory(CleanerTempDirectoryPrefix(),
                                      &temp_dir) &&
         scoped_temp_dir_->Set(temp_dir);
}

void ChromeCleanerFetcher::OnTemporaryDirectoryCreated(bool success) {
  if (!success) {
    RecordCleanerDownloadStatusHistogram(
        CLEANER_DOWNLOAD_STATUS_FAILED_TO_CREATE_TEMP_DIR);
    PostCallbackAndDeleteSelf(
        base::FilePath(),
        ChromeCleanerFetchStatus::kFailedToCreateTemporaryDirectory);
    return;
  }

  DCHECK(!scoped_temp_dir_->GetPath().empty());

  temp_file_ = scoped_temp_dir_->GetPath().Append(
      base::ASCIIToUTF16(base::GenerateGUID()) + L".tmp");

  data_use_measurement::DataUseUserData::AttachToFetcher(
      url_fetcher_.get(), data_use_measurement::DataUseUserData::SAFE_BROWSING);
  url_fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE |
                             net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SAVE_COOKIES);
  url_fetcher_->SetMaxRetriesOn5xx(3);
  url_fetcher_->SaveResponseToFileAtPath(temp_file_, blocking_task_runner_);
  url_fetcher_->SetRequestContext(g_browser_process->system_request_context());

  time_fetching_started_ = base::Time::Now();
  url_fetcher_->Start();
}

void ChromeCleanerFetcher::PostCallbackAndDeleteSelf(
    base::FilePath path,
    ChromeCleanerFetchStatus fetch_status) {
  DCHECK(fetched_callback_);

  std::move(fetched_callback_).Run(std::move(path), fetch_status);

  // Since url_fetcher_ is passed a pointer to this object during construction,
  // explicitly destroy the url_fetcher_ to avoid potential destruction races.
  url_fetcher_.reset();

  // At this point, the url_fetcher_ is gone and this ChromeCleanerFetcher
  // instance is no longer needed.
  delete this;
}

void ChromeCleanerFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  // Take ownership of the fetcher in this scope (source == url_fetcher_).
  DCHECK_EQ(url_fetcher_.get(), source);
  DCHECK(!source->GetStatus().is_io_pending());
  DCHECK(fetched_callback_);

  if (!source->GetStatus().is_success()) {
    base::UmaHistogramSparse(kDownloadStatusErrorCodeHistogramName,
                             source->GetStatus().error());
    RecordTimeToCompleteDownload(
        FetchCompletedReasonHistogramSuffix::kNetworkError);
    RecordDownloadStatusAndPostCallback(
        CLEANER_DOWNLOAD_STATUS_OTHER_FAILURE,
        ChromeCleanerFetchStatus::kOtherFailure);
    return;
  }

  const int response_code = source->GetResponseCode();
  base::UmaHistogramSparse(kDownloadStatusErrorCodeHistogramName,
                           response_code);
  const FetchCompletedReasonHistogramSuffix suffix =
      response_code == net::HTTP_OK
          ? FetchCompletedReasonHistogramSuffix::kDownloadSuccess
          : FetchCompletedReasonHistogramSuffix::kDownloadFailure;
  RecordTimeToCompleteDownload(suffix);

  if (response_code == net::HTTP_NOT_FOUND) {
    RecordDownloadStatusAndPostCallback(
        CLEANER_DOWNLOAD_STATUS_NOT_FOUND_ON_SERVER,
        ChromeCleanerFetchStatus::kNotFoundOnServer);
    return;
  }

  if (response_code != net::HTTP_OK) {
    RecordDownloadStatusAndPostCallback(
        CLEANER_DOWNLOAD_STATUS_OTHER_FAILURE,
        ChromeCleanerFetchStatus::kOtherFailure);
    return;
  }

  base::FilePath download_path;
  if (!source->GetResponseAsFilePath(/*take_ownership=*/true, &download_path)) {
    RecordDownloadStatusAndPostCallback(
        CLEANER_DOWNLOAD_STATUS_FAILED_TO_SAVE_TO_FILE,
        ChromeCleanerFetchStatus::kOtherFailure);
    return;
  }

  DCHECK(!download_path.empty());
  DCHECK_EQ(temp_file_.value(), download_path.value());

  // Take ownership of the scoped temp directory so it is not deleted.
  scoped_temp_dir_->Take();

  RecordCleanerDownloadStatusHistogram(CLEANER_DOWNLOAD_STATUS_SUCCEEDED);
  PostCallbackAndDeleteSelf(std::move(download_path),
                            ChromeCleanerFetchStatus::kSuccess);
}

void ChromeCleanerFetcher::RecordDownloadStatusAndPostCallback(
    CleanerDownloadStatusHistogramValue histogram_value,
    ChromeCleanerFetchStatus fetch_status) {
  RecordCleanerDownloadStatusHistogram(histogram_value);
  PostCallbackAndDeleteSelf(base::FilePath(), fetch_status);
}

void ChromeCleanerFetcher::RecordTimeToCompleteDownload(
    FetchCompletedReasonHistogramSuffix suffix) {
  const base::TimeDelta time_difference =
      base::Time::Now() - time_fetching_started_;
  switch (suffix) {
    case FetchCompletedReasonHistogramSuffix::kDownloadFailure:
      UMA_HISTOGRAM_LONG_TIMES_100(
          "SoftwareReporter.Cleaner.TimeToCompleteDownload_DownloadFailure",
          time_difference);
      break;

    case FetchCompletedReasonHistogramSuffix::kDownloadSuccess:
      UMA_HISTOGRAM_LONG_TIMES_100(
          "SoftwareReporter.Cleaner.TimeToCompleteDownload_DownloadSuccess",
          time_difference);
      break;

    case FetchCompletedReasonHistogramSuffix::kNetworkError:
      UMA_HISTOGRAM_LONG_TIMES_100(
          "SoftwareReporter.Cleaner.TimeToCompleteDownload_NetworkError",
          time_difference);
      break;
  }
}

}  // namespace

void FetchChromeCleaner(ChromeCleanerFetchedCallback fetched_callback) {
  new ChromeCleanerFetcher(std::move(fetched_callback));
}

}  // namespace safe_browsing
