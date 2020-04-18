// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/webrtc_event_log_uploader.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/base/mime_util.h"
#include "net/http/http_status_code.h"

// Explanation about the life cycle of a WebRtcEventLogUploaderImpl object, and
// about why its use of base::Unretained is safe:
// * WebRtcEventLogUploaderImpl objects are owned (indirectly) by
//   WebRtcEventLogManager, which is a singleton object that is only destroyed
//   during Chrome shutdown, from ~BrowserProcessImpl().
//   When ~BrowserProcessImpl() executes, tasks previously posted to
//   WebRtcEventLogManager's internal task will not execute, and anything posted
//   later will be discarded. Deleting a WebRtcEventLogUploaderImpl will
//   therefore have no adverse effects.
// * Except for during Chrome shutdown, WebRtcEventLogUploaderImpl objects will
//   only be destroyed when their owner explicitly decides to destroy them.
// * The direct owner, WebRtcRemoteEventLogManager, only deletes a
//   WebRtcEventLogUploaderImpl after it receives a notification
//   of type OnWebRtcEventLogUploadComplete.
// * OnWebRtcEventLogUploadComplete() is only ever called as the last step in
//   URLFetcher's lifecycle. When it is called, there are no tasks pending which
//   have a reference to this WebRtcEventLogUploaderImpl object.
// * The previous point follows from OnURLFetchComplete being guaranteed to
//   be the last callback called on a URLFetcherDelegate.

namespace {
// TODO(crbug.com/817495): Eliminate the duplication with other uploaders.
const char kUploadContentType[] = "multipart/form-data";
const char kBoundary[] = "----**--yradnuoBgoLtrapitluMklaTelgooG--**----";

const char kLogFilename[] = "webrtc_event_log";
const char kLogExtension[] = "log";

constexpr size_t kExpectedMimeOverheadBytes = 1000;  // Intentional overshot.

// TODO(crbug.com/817495): Eliminate the duplication with other uploaders.
#if defined(OS_WIN)
const char kProduct[] = "Chrome";
#elif defined(OS_MACOSX)
const char kProduct[] = "Chrome_Mac";
#elif defined(OS_LINUX)
const char kProduct[] = "Chrome_Linux";
#elif defined(OS_ANDROID)
const char kProduct[] = "Chrome_Android";
#elif defined(OS_CHROMEOS)
const char kProduct[] = "Chrome_ChromeOS";
#else
#error Platform not supported.
#endif

// TODO(crbug.com/775415): Update comment to reflect new policy when discarding
// the command line flag.
constexpr net::NetworkTrafficAnnotationTag
    kWebrtcEventLogUploaderTrafficAnnotation =
        net::DefineNetworkTrafficAnnotation("webrtc_event_log_uploader", R"(
      semantics {
        sender: "WebRTC Event Log uploader module"
        description:
          "Uploads a WebRTC event log to a server called Crash. These logs "
          "will not contain private information. They will be used to "
          "improve WebRTC (fix bugs, tune performance, etc.)."
        trigger:
          "A privileged JS application (Hangouts/Meet) has requested a peer "
          "connection to be logged, and the resulting event log to be "
          "uploaded at a time deemed to cause the least interference to the "
          "user (i.e., when the user is not busy making other VoIP calls)."
        data:
          "WebRTC events such as the timing of audio playout (but not the "
          "content), timing and size of RTP packets sent/received, etc."
        destination: GOOGLE_OWNED_SERVICE
      }
      policy {
        cookies_allowed: NO
        setting: "This feature is only enabled if the user launches Chrome "
                 "with a specific command line flag: "
                 "--enable-features=WebRtcRemoteEventLog"
        policy_exception_justification:
          "Not applicable."
      })");

void AddFileContents(const std::string& file_contents,
                     const std::string& content_type,
                     std::string* post_data) {
  // net::AddMultipartValueForUpload does almost what we want to do here, except
  // that it does not add the "filename" attribute. We hack it to force it to.
  std::string mime_value_name = base::StringPrintf(
      "%s\"; filename=\"%s.%s\"", kLogFilename, kLogFilename, kLogExtension);
  net::AddMultipartValueForUpload(mime_value_name, file_contents, kBoundary,
                                  content_type, post_data);
}

std::string MimeContentType() {
  const char kBoundaryKeywordAndMisc[] = "; boundary=";

  std::string content_type;
  content_type.reserve(sizeof(content_type) + sizeof(kBoundaryKeywordAndMisc) +
                       sizeof(kBoundary));

  content_type.append(kUploadContentType);
  content_type.append(kBoundaryKeywordAndMisc);
  content_type.append(kBoundary);

  return content_type;
}
}  // namespace

const char WebRtcEventLogUploaderImpl::kUploadURL[] =
    "https://clients2.google.com/cr/report";

std::unique_ptr<WebRtcEventLogUploader>
WebRtcEventLogUploaderImpl::Factory::Create(
    const base::FilePath& log_file,
    WebRtcEventLogUploaderObserver* observer) {
  DCHECK(observer);
  return std::make_unique<WebRtcEventLogUploaderImpl>(
      log_file, observer, kMaxRemoteLogFileSizeBytes);
}

std::unique_ptr<WebRtcEventLogUploader>
WebRtcEventLogUploaderImpl::Factory::CreateWithCustomMaxSizeForTesting(
    const base::FilePath& log_file,
    WebRtcEventLogUploaderObserver* observer,
    size_t max_log_file_size_bytes) {
  DCHECK(observer);
  return std::make_unique<WebRtcEventLogUploaderImpl>(log_file, observer,
                                                      max_log_file_size_bytes);
}

WebRtcEventLogUploaderImpl::Delegate::Delegate(
    WebRtcEventLogUploaderImpl* owner)
    : owner_(owner) {}

#if DCHECK_IS_ON()
void WebRtcEventLogUploaderImpl::Delegate::OnURLFetchUploadProgress(
    const net::URLFetcher* source,
    int64_t current,
    int64_t total) {
  std::string unit;
  if (total <= 1000) {
    unit = "bytes";
  } else if (total <= 1000 * 1000) {
    unit = "KBs";
    current /= 1000;
    total /= 1000;
  } else {
    unit = "MBs";
    current /= 1000 * 1000;
    total /= 1000 * 1000;
  }
  VLOG(1) << "WebRTC event log upload progress: " << current << " / " << total
          << " " << unit << ".";
}
#endif

void WebRtcEventLogUploaderImpl::Delegate::OnURLFetchComplete(
    const net::URLFetcher* source) {
  owner_->OnURLFetchComplete(source);
}

WebRtcEventLogUploaderImpl::WebRtcEventLogUploaderImpl(
    const base::FilePath& log_file,
    WebRtcEventLogUploaderObserver* observer,
    size_t max_log_file_size_bytes)
    : delegate_(this),
      log_file_(log_file),
      observer_(observer),
      max_log_file_size_bytes_(max_log_file_size_bytes),
      request_context_getter_(nullptr),
      io_task_runner_(base::SequencedTaskRunnerHandle::Get()) {
  DCHECK(observer);

  if (!PrepareUploadData()) {
    ReportResult(false);
    return;
  }

  // See the comment at the beginning of this file for an explanation about why
  // base::Unretained is safe to use here.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&WebRtcEventLogUploaderImpl::PrepareRequestContext,
                     base::Unretained(this)));
}

WebRtcEventLogUploaderImpl::~WebRtcEventLogUploaderImpl() {
  // WebRtcEventLogUploaderImpl objects only deleted if either:
  // 1. Chrome shutdown - see the explanation at top of this file.
  // 2. The upload was never started, meaning |url_fetcher_| was never set.
  // 3. Upload started and finished - |url_fetcher_| should have been reset
  //    so that we would be able to DCHECK and demonstrate that the determinant
  //    is maintained.
  // Therefore, we can be sure that when we destroy this object, there are
  // either no tasks holding a reference to it, or they would not be allowed
  // to run.
  if (io_task_runner_->RunsTasksInCurrentSequence()) {
    DCHECK(!url_fetcher_);
  } else {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    // This is only expected to happen during Chrome shutdown.
    bool will_delete =
        io_task_runner_->DeleteSoon(FROM_HERE, url_fetcher_.release());
    DCHECK(!will_delete)
        << "Task runners must have been stopped by this stage of shutdown.";
  }
}

bool WebRtcEventLogUploaderImpl::PrepareUploadData() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  // TODO(crbug.com/775415): Avoid reading the entire file into memory.
  std::string log_file_contents;
  if (!base::ReadFileToStringWithMaxSize(log_file_, &log_file_contents,
                                         max_log_file_size_bytes_)) {
    LOG(WARNING) << "Couldn't read event log file, or max file size exceeded.";
    return false;
  }

  DCHECK(post_data_.empty());
  post_data_.reserve(log_file_contents.size() + kExpectedMimeOverheadBytes);
  net::AddMultipartValueForUpload("prod", kProduct, kBoundary, "", &post_data_);
  net::AddMultipartValueForUpload("ver",
                                  version_info::GetVersionNumber() + "-webrtc",
                                  kBoundary, "", &post_data_);
  net::AddMultipartValueForUpload("guid", "0", kBoundary, "", &post_data_);
  net::AddMultipartValueForUpload("type", kLogFilename, kBoundary, "",
                                  &post_data_);
  AddFileContents(log_file_contents, "application/log", &post_data_);
  net::AddMultipartFinalDelimiterForUpload(kBoundary, &post_data_);

  return true;
}

void WebRtcEventLogUploaderImpl::PrepareRequestContext() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // system_request_context() can only be gotten on the UI thread, but can
  // then be used by any thread.
  DCHECK(!request_context_getter_);
  request_context_getter_ = g_browser_process->system_request_context();
  // In unit tests, request_context_getter_ will remain null.

  // See the comment at the beginning of this file for an explanation about why
  // base::Unretained is safe to use here.
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WebRtcEventLogUploaderImpl::StartUpload,
                                base::Unretained(this)));
}

void WebRtcEventLogUploaderImpl::StartUpload() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  url_fetcher_ = net::URLFetcher::Create(
      GURL(kUploadURL), net::URLFetcher::POST, &delegate_,
      kWebrtcEventLogUploaderTrafficAnnotation);
  url_fetcher_->SetRequestContext(request_context_getter_);
  url_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES);
  url_fetcher_->SetUploadData(MimeContentType(), post_data_);
  url_fetcher_->Start();  // Delegat::OnURLFetchComplete called when finished.
}

void WebRtcEventLogUploaderImpl::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  DCHECK_EQ(source, url_fetcher_.get());

  const bool upload_successful =
      (source->GetStatus().status() == net::URLRequestStatus::SUCCESS &&
       source->GetResponseCode() == net::HTTP_OK);

  if (upload_successful) {
    // TODO(crbug.com/775415): Update chrome://webrtc-logs.
    std::string report_id;
    if (!url_fetcher_->GetResponseAsString(&report_id)) {
      LOG(WARNING) << "WebRTC event log completed, but report ID unknown.";
    } else {
      // TODO(crbug.com/775415): Remove this when chrome://webrtc-logs updated.
      VLOG(1) << "WebRTC event log successfully uploaded: " << report_id;
    }
  } else {
    LOG(WARNING) << "WebRTC event log upload failed.";
  }

  ReportResult(upload_successful);

  url_fetcher_.reset();  // Explicitly maintain determinant.
}

void WebRtcEventLogUploaderImpl::ReportResult(bool result) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  // If the upload was successful, the file is no longer needed.
  // If the upload failed, we don't want to retry, because we run the risk of
  // uploading significant amounts of data once again, only for the upload to
  // fail again after (as an example) wasting 50MBs of upload bandwidth.
  // TODO(crbug.com/775415): Provide refined retrial behavior.
  DeleteLogFile();

  observer_->OnWebRtcEventLogUploadComplete(log_file_, result);
}

void WebRtcEventLogUploaderImpl::DeleteLogFile() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  const bool deletion_successful =
      base::DeleteFile(log_file_, /*recursive=*/false);
  if (!deletion_successful) {
    // This is a somewhat serious (though unlikely) error, because now we'll
    // try to upload this file again next time Chrome launches.
    LOG(ERROR) << "Could not delete pending WebRTC event log file.";
  }
}
