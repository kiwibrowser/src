// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_sync/signin_confirmation_helper.h"

#include <memory>

#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/history/core/browser/history_backend.h"
#include "components/history/core/browser/history_db_task.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"

namespace browser_sync {

namespace {

// Determines whether there are any typed URLs in a history backend.
class HasTypedURLsTask : public history::HistoryDBTask {
 public:
  explicit HasTypedURLsTask(const base::Callback<void(bool)>& cb)
      : has_typed_urls_(false), cb_(cb) {}

  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override {
    history::URLRows rows;
    backend->GetAllTypedURLs(&rows);
    if (!rows.empty()) {
      DVLOG(1) << "SigninConfirmationHelper: history contains " << rows.size()
               << " typed URLs";
      has_typed_urls_ = true;
    }
    return true;
  }

  void DoneRunOnMainThread() override { cb_.Run(has_typed_urls_); }

 private:
  ~HasTypedURLsTask() override {}

  bool has_typed_urls_;
  base::Callback<void(bool)> cb_;
};

}  // namespace

SigninConfirmationHelper::SigninConfirmationHelper(
    history::HistoryService* history_service,
    const base::Callback<void(bool)>& return_result)
    : origin_thread_(base::ThreadTaskRunnerHandle::Get()),
      history_service_(history_service),
      pending_requests_(0),
      return_result_(return_result) {}

SigninConfirmationHelper::~SigninConfirmationHelper() {
  DCHECK(origin_thread_->BelongsToCurrentThread());
}

void SigninConfirmationHelper::OnHistoryQueryResults(
    size_t max_entries,
    history::QueryResults* results) {
  history::QueryResults owned_results;
  results->Swap(&owned_results);
  bool too_much_history = owned_results.size() >= max_entries;
  if (too_much_history) {
    DVLOG(1) << "SigninConfirmationHelper: profile contains "
             << owned_results.size() << " history entries";
  }
  ReturnResult(too_much_history);
}

void SigninConfirmationHelper::CheckHasHistory(int max_entries) {
  pending_requests_++;
  if (!history_service_) {
    PostResult(false);
    return;
  }
  history::QueryOptions opts;
  opts.max_count = max_entries;
  history_service_->QueryHistory(
      base::string16(), opts,
      base::Bind(&SigninConfirmationHelper::OnHistoryQueryResults,
                 base::Unretained(this), max_entries),
      &task_tracker_);
}

void SigninConfirmationHelper::CheckHasTypedURLs() {
  pending_requests_++;
  if (!history_service_) {
    PostResult(false);
    return;
  }
  history_service_->ScheduleDBTask(
      FROM_HERE,
      std::unique_ptr<history::HistoryDBTask>(new HasTypedURLsTask(base::Bind(
          &SigninConfirmationHelper::ReturnResult, base::Unretained(this)))),
      &task_tracker_);
}

void SigninConfirmationHelper::PostResult(bool result) {
  origin_thread_->PostTask(FROM_HERE,
                           base::Bind(&SigninConfirmationHelper::ReturnResult,
                                      base::Unretained(this), result));
}

void SigninConfirmationHelper::ReturnResult(bool result) {
  DCHECK(origin_thread_->BelongsToCurrentThread());
  // Pass |true| into the callback as soon as one of the tasks passes a
  // result of |true|, otherwise pass the last returned result.
  if (--pending_requests_ == 0 || result) {
    return_result_.Run(result);

    // This leaks at shutdown if the HistoryService is destroyed, but
    // the process is going to die anyway.
    delete this;
  }
}

}  // namespace browser_sync
