// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feedback/feedback_uploader.h"

#include "base/callback.h"
#include "base/command_line.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/feedback/feedback_report.h"
#include "components/feedback/feedback_switches.h"
#include "components/feedback/feedback_uploader_delegate.h"
#include "components/variations/net/variations_http_headers.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"

namespace feedback {

namespace {

constexpr base::FilePath::CharType kFeedbackReportPath[] =
    FILE_PATH_LITERAL("Feedback Reports");

constexpr char kFeedbackPostUrl[] =
    "https://www.google.com/tools/feedback/chrome/__submit";

constexpr char kProtoBufMimeType[] = "application/x-protobuf";

// The minimum time to wait before uploading reports are retried. Exponential
// backoff delay is applied on successive failures.
// This value can be overriden by tests by calling
// FeedbackUploader::SetMinimumRetryDelayForTesting().
base::TimeDelta g_minimum_retry_delay = base::TimeDelta::FromMinutes(60);

// If a new report is queued to be dispatched immediately while another is being
// dispatched, this is the time to wait for the on-going dispatching to finish.
base::TimeDelta g_dispatching_wait_delay = base::TimeDelta::FromSeconds(4);

base::FilePath GetPathFromContext(content::BrowserContext* context) {
  return context->GetPath().Append(kFeedbackReportPath);
}

GURL GetFeedbackPostGURL() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  return GURL(command_line.HasSwitch(switches::kFeedbackServer)
                  ? command_line.GetSwitchValueASCII(switches::kFeedbackServer)
                  : kFeedbackPostUrl);
}

}  // namespace

FeedbackUploader::FeedbackUploader(
    content::BrowserContext* context,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : context_(context),
      feedback_reports_path_(GetPathFromContext(context)),
      task_runner_(task_runner),
      feedback_post_url_(GetFeedbackPostGURL()),
      retry_delay_(g_minimum_retry_delay),
      is_dispatching_(false) {
  DCHECK(task_runner_);
  DCHECK(context_);
}

FeedbackUploader::~FeedbackUploader() {}

// static
void FeedbackUploader::SetMinimumRetryDelayForTesting(base::TimeDelta delay) {
  g_minimum_retry_delay = delay;
}

void FeedbackUploader::QueueReport(const std::string& data) {
  QueueReportWithDelay(data, base::TimeDelta());
}

void FeedbackUploader::StartDispatchingReport() {
  DispatchReport();
}

void FeedbackUploader::OnReportUploadSuccess() {
  retry_delay_ = g_minimum_retry_delay;
  is_dispatching_ = false;
  // Explicitly release the successfully dispatched report.
  report_being_dispatched_->DeleteReportOnDisk();
  report_being_dispatched_ = nullptr;
  UpdateUploadTimer();
}

void FeedbackUploader::OnReportUploadFailure(bool should_retry) {
  if (should_retry) {
    // Implement a backoff delay by doubling the retry delay on each failure.
    retry_delay_ *= 2;
    report_being_dispatched_->set_upload_at(retry_delay_ + base::Time::Now());
    reports_queue_.emplace(report_being_dispatched_);
  } else {
    // The report won't be retried, hence explicitly delete its file on disk.
    report_being_dispatched_->DeleteReportOnDisk();
  }

  // The report dispatching failed, and should either be retried or not. In all
  // cases, we need to release |report_being_dispatched_|. If it was up for
  // retry, then it has already been re-enqueued and will be kept alive.
  // Otherwise we're done with it and it should destruct.
  report_being_dispatched_ = nullptr;
  is_dispatching_ = false;
  UpdateUploadTimer();
}

bool FeedbackUploader::ReportsUploadTimeComparator::operator()(
    const scoped_refptr<FeedbackReport>& a,
    const scoped_refptr<FeedbackReport>& b) const {
  return a->upload_at() > b->upload_at();
}

void FeedbackUploader::AppendExtraHeadersToUploadRequest(
    net::URLFetcher* fetcher) {}

void FeedbackUploader::DispatchReport() {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("chrome_feedback_report_app", R"(
        semantics {
          sender: "Chrome Feedback Report App"
          description:
            "Users can press Alt+Shift+i to report a bug or a feedback in "
            "general. Along with the free-form text they entered, system logs "
            "that helps in diagnosis of the issue are sent to Google. This "
            "service uploads the report to Google Feedback server."
          trigger:
            "When user chooses to send a feedback to Google."
          data:
            "The free-form text that user has entered and useful debugging "
            "logs (UI logs, Chrome logs, kernel logs, auto update engine logs, "
            "ARC++ logs, etc.). The logs are anonymized to remove any "
            "user-private data. The user can view the system information "
            "before sending, and choose to send the feedback report without "
            "system information and the logs (unchecking 'Send system "
            "information' prevents sending logs as well), the screenshot, or "
            "even his/her email address."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings and is only activated "
            "by direct user request."
          policy_exception_justification: "Not implemented."
        })");
  // Note: FeedbackUploaderDelegate deletes itself and the fetcher.
  net::URLFetcher* fetcher =
      net::URLFetcher::Create(
          feedback_post_url_, net::URLFetcher::POST,
          new FeedbackUploaderDelegate(
              base::Bind(&FeedbackUploader::OnReportUploadSuccess, AsWeakPtr()),
              base::Bind(&FeedbackUploader::OnReportUploadFailure,
                         AsWeakPtr())),
          traffic_annotation)
          .release();
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher, data_use_measurement::DataUseUserData::FEEDBACK_UPLOADER);
  // Tell feedback server about the variation state of this install.
  net::HttpRequestHeaders headers;
  // Note: It's OK to pass SignedIn::kNo if it's unknown, as it does not affect
  // transmission of experiments coming from the variations server.
  variations::AppendVariationHeaders(fetcher->GetOriginalURL(),
                                     context_->IsOffTheRecord()
                                         ? variations::InIncognito::kYes
                                         : variations::InIncognito::kNo,
                                     variations::SignedIn::kNo, &headers);
  fetcher->SetExtraRequestHeaders(headers.ToString());

  fetcher->SetUploadData(kProtoBufMimeType, report_being_dispatched_->data());
  fetcher->SetRequestContext(
      content::BrowserContext::GetDefaultStoragePartition(context_)
          ->GetURLRequestContext());

  AppendExtraHeadersToUploadRequest(fetcher);

  fetcher->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                        net::LOAD_DO_NOT_SEND_COOKIES);
  fetcher->Start();
}

void FeedbackUploader::UpdateUploadTimer() {
  if (reports_queue_.empty())
    return;

  scoped_refptr<FeedbackReport> report = reports_queue_.top();
  const base::Time now = base::Time::Now();
  if (report->upload_at() <= now && !is_dispatching_) {
    reports_queue_.pop();
    is_dispatching_ = true;
    report_being_dispatched_ = report;
    StartDispatchingReport();
  } else {
    // Stop the old timer and start an updated one.
    const base::TimeDelta delay = (is_dispatching_ || now > report->upload_at())
                                      ? g_dispatching_wait_delay
                                      : report->upload_at() - now;
    upload_timer_.Stop();
    upload_timer_.Start(FROM_HERE, delay, this,
                        &FeedbackUploader::UpdateUploadTimer);
  }
}

void FeedbackUploader::QueueReportWithDelay(const std::string& data,
                                            base::TimeDelta delay) {
  reports_queue_.emplace(base::MakeRefCounted<FeedbackReport>(
      feedback_reports_path_, base::Time::Now() + delay, data, task_runner_));
  UpdateUploadTimer();
}

}  // namespace feedback
