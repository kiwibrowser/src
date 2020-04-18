// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_H_
#define COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_H_

#include <queue>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace net {
class URLFetcher;
}  // namespace net

namespace feedback {

class FeedbackReport;

// FeedbackUploader is used to add a feedback report to the queue of reports
// being uploaded. In case uploading a report fails, it is written to disk and
// tried again when it's turn comes up next in the queue.
class FeedbackUploader : public KeyedService,
                         public base::SupportsWeakPtr<FeedbackUploader> {
 public:
  FeedbackUploader(content::BrowserContext* context,
                   scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~FeedbackUploader() override;

  static void SetMinimumRetryDelayForTesting(base::TimeDelta delay);

  // Queues a report for uploading.
  void QueueReport(const std::string& data);

  bool QueueEmpty() const { return reports_queue_.empty(); }

  content::BrowserContext* context() { return context_; }

  const base::FilePath& feedback_reports_path() const {
    return feedback_reports_path_;
  }

  scoped_refptr<base::SingleThreadTaskRunner> task_runner() const {
    return task_runner_;
  }

  base::TimeDelta retry_delay() const { return retry_delay_; }

 protected:
  // Virtual to give implementers a chance to do work before the report is
  // disptached. Implementers can then call
  // FeedbackUploader::StartSendingReport() when ready so that the report is
  // dispatched.
  virtual void StartDispatchingReport();

  // Invoked when a feedback report upload succeeds. It will reset the
  // |retry_delay_| to its minimum value and schedules the next report upload if
  // any.
  void OnReportUploadSuccess();

  // Invoked when |report_being_dispatched_| fails to upload. If |should_retry|
  // is true, it will double the |retry_delay_| and reenqueue
  // |report_being_dispatched_| with the new delay. All subsequent retries will
  // keep increasing the delay until a successful upload is encountered.
  void OnReportUploadFailure(bool should_retry);

  const scoped_refptr<FeedbackReport>& report_being_dispatched() const {
    return report_being_dispatched_;
  }

 private:
  friend class FeedbackUploaderTest;

  struct ReportsUploadTimeComparator {
    bool operator()(const scoped_refptr<FeedbackReport>& a,
                    const scoped_refptr<FeedbackReport>& b) const;
  };

  // Called from DispatchReport() to give implementers a chance to add extra
  // headers to the upload request before it's sent.
  virtual void AppendExtraHeadersToUploadRequest(net::URLFetcher* fetcher);

  // Uploads the |report_being_dispatched_| to be uploaded. It must
  // call either OnReportUploadSuccess() or OnReportUploadFailure() so that
  // dispatching reports can progress.
  void DispatchReport();

  // Update our timer for uploading the next report.
  void UpdateUploadTimer();

  void QueueReportWithDelay(const std::string& data, base::TimeDelta delay);

  // Browser context this uploader was created for.
  content::BrowserContext* context_;

  const base::FilePath feedback_reports_path_;

  // Timer to upload the next report at.
  base::OneShotTimer upload_timer_;

  // See comment of |FeedbackUploaderFactory::task_runner_|.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  scoped_refptr<FeedbackReport> report_being_dispatched_;

  const GURL feedback_post_url_;

  // Priority queue of reports prioritized by the time the report is supposed
  // to be uploaded at.
  std::priority_queue<scoped_refptr<FeedbackReport>,
                      std::vector<scoped_refptr<FeedbackReport>>,
                      ReportsUploadTimeComparator>
      reports_queue_;

  base::TimeDelta retry_delay_;

  // True when a report is currently being dispatched. Only a single report
  // at-a-time should be dispatched.
  bool is_dispatching_;

  DISALLOW_COPY_AND_ASSIGN(FeedbackUploader);
};

}  // namespace feedback

#endif  // COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_H_
