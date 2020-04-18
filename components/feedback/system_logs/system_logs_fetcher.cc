// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feedback/system_logs/system_logs_fetcher.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace system_logs {

namespace {

// List of keys in the SystemLogsResponse map whose corresponding values will
// not be anonymized.
constexpr const char* const kWhitelistedKeysOfUUIDs[] = {
    "CHROMEOS_BOARD_APPID", "CHROMEOS_CANARY_APPID", "CHROMEOS_RELEASE_APPID",
    "CLIENT_ID",
};

// Returns true if the given |key| is anonymizer-whitelisted and whose
// corresponding value should not be anonymized.
bool IsKeyWhitelisted(const std::string& key) {
  for (auto* const whitelisted_key : kWhitelistedKeysOfUUIDs) {
    if (key == whitelisted_key)
      return true;
  }
  return false;
}

// Runs the Anonymizer tool over the entris of |response|.
void Anonymize(feedback::AnonymizerTool* anonymizer,
               SystemLogsResponse* response) {
  for (auto& element : *response) {
    if (!IsKeyWhitelisted(element.first))
      element.second = anonymizer->Anonymize(element.second);
  }
}

}  // namespace

SystemLogsFetcher::SystemLogsFetcher(bool scrub_data)
    : response_(std::make_unique<SystemLogsResponse>()),
      num_pending_requests_(0),
      task_runner_for_anonymizer_(base::CreateSequencedTaskRunnerWithTraits(
          {// User visible because this is called when the user is looking at
           // the send feedback dialog, watching a spinner.
           base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN})),
      weak_ptr_factory_(this) {
  if (scrub_data)
    anonymizer_ = std::make_unique<feedback::AnonymizerTool>();
}

SystemLogsFetcher::~SystemLogsFetcher() {
  // Ensure that destruction happens on same sequence where the object is being
  // accessed.
  task_runner_for_anonymizer_->DeleteSoon(FROM_HERE, std::move(anonymizer_));
}

void SystemLogsFetcher::AddSource(std::unique_ptr<SystemLogsSource> source) {
  data_sources_.emplace_back(std::move(source));
  num_pending_requests_++;
}

void SystemLogsFetcher::Fetch(SysLogsFetcherCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(callback_.is_null());
  DCHECK(!callback.is_null());

  callback_ = std::move(callback);
  for (size_t i = 0; i < data_sources_.size(); ++i) {
    VLOG(1) << "Fetching SystemLogSource: " << data_sources_[i]->source_name();
    data_sources_[i]->Fetch(base::Bind(&SystemLogsFetcher::OnFetched,
                                       weak_ptr_factory_.GetWeakPtr(),
                                       data_sources_[i]->source_name()));
  }
}

void SystemLogsFetcher::OnFetched(
    const std::string& source_name,
    std::unique_ptr<SystemLogsResponse> response) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  VLOG(1) << "Received SystemLogSource: " << source_name;

  if (anonymizer_) {
    // It is safe to pass the unretained anonymizer_ instance here because
    // the anonymizer_ is owned by |this| and |this| only deletes itself
    // once all responses have been collected and added (see AddResponse()).
    SystemLogsResponse* response_ptr = response.get();
    task_runner_for_anonymizer_->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(Anonymize, base::Unretained(anonymizer_.get()),
                       base::Unretained(response_ptr)),
        base::BindOnce(&SystemLogsFetcher::AddResponse,
                       weak_ptr_factory_.GetWeakPtr(), source_name,
                       std::move(response)));
  } else {
    AddResponse(source_name, std::move(response));
  }
}

void SystemLogsFetcher::AddResponse(
    const std::string& source_name,
    std::unique_ptr<SystemLogsResponse> response) {
  for (const auto& it : *response) {
    // An element with a duplicate key would not be successfully inserted.
    bool ok = response_->emplace(it).second;
    DCHECK(ok) << "Duplicate key found: " << it.first;
  }

  --num_pending_requests_;
  if (num_pending_requests_ > 0)
    return;

  DCHECK(!callback_.is_null());
  std::move(callback_).Run(std::move(response_));

  BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
}

}  // namespace system_logs
