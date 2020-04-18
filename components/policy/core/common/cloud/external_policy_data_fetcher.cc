// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/external_policy_data_fetcher.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/stl_util.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

namespace policy {

namespace {

// Helper that forwards the result of a fetch job from the thread that the
// ExternalPolicyDataFetcherBackend runs on to the thread that the
// ExternalPolicyDataFetcher which started the job runs on.
void ForwardJobFinished(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const ExternalPolicyDataFetcherBackend::FetchCallback& callback,
    ExternalPolicyDataFetcher::Job* job,
    ExternalPolicyDataFetcher::Result result,
    std::unique_ptr<std::string> data) {
  task_runner->PostTask(FROM_HERE,
                        base::BindOnce(callback, job, result, std::move(data)));
}

// Helper that forwards a job cancelation confirmation from the thread that the
// ExternalPolicyDataFetcherBackend runs on to the thread that the
// ExternalPolicyDataFetcher which canceled the job runs on.
void ForwardJobCanceled(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const base::Closure& callback) {
  task_runner->PostTask(FROM_HERE, callback);
}

}  // namespace

struct ExternalPolicyDataFetcher::Job {
  Job(const GURL& url,
      int64_t max_size,
      const ExternalPolicyDataFetcherBackend::FetchCallback& callback);

  const GURL url;
  const int64_t max_size;
  const ExternalPolicyDataFetcherBackend::FetchCallback callback;

 private:
  DISALLOW_COPY_AND_ASSIGN(Job);
};

ExternalPolicyDataFetcher::Job::Job(
    const GURL& url,
    int64_t max_size,
    const ExternalPolicyDataFetcherBackend::FetchCallback& callback)
    : url(url), max_size(max_size), callback(callback) {}

ExternalPolicyDataFetcher::ExternalPolicyDataFetcher(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    const base::WeakPtr<ExternalPolicyDataFetcherBackend>& backend)
    : task_runner_(task_runner),
      io_task_runner_(io_task_runner),
      backend_(backend),
      weak_factory_(this) {
}

ExternalPolicyDataFetcher::~ExternalPolicyDataFetcher() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  for (JobSet::iterator it = jobs_.begin(); it != jobs_.end(); ++it)
    CancelJob(*it);
}

ExternalPolicyDataFetcher::Job* ExternalPolicyDataFetcher::StartJob(
    const GURL& url,
    int64_t max_size,
    const FetchCallback& callback) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  Job* job = new Job(
      url, max_size,
      base::Bind(&ForwardJobFinished,
                 task_runner_,
                 base::Bind(&ExternalPolicyDataFetcher::OnJobFinished,
                            weak_factory_.GetWeakPtr(),
                            callback)));
  jobs_.insert(job);
  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&ExternalPolicyDataFetcherBackend::StartJob, backend_, job));
  return job;
}

void ExternalPolicyDataFetcher::CancelJob(Job* job) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK(jobs_.find(job) != jobs_.end());
  jobs_.erase(job);
  // Post a task that will cancel the |job| in the |backend_|. The |job| is
  // removed from |jobs_| immediately to indicate that it has been canceled but
  // is not actually deleted until the cancelation has reached the |backend_|
  // and a confirmation has been posted back. This ensures that no new job can
  // be allocated at the same address while an OnJobFinished() callback may
  // still be pending for the canceled |job|.
  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&ExternalPolicyDataFetcherBackend::CancelJob, backend_, job,
                 base::Bind(&ForwardJobCanceled, task_runner_,
                            base::Bind(base::DoNothing::Repeatedly<Job*>(),
                                       base::Owned(job)))));
}

void ExternalPolicyDataFetcher::OnJobFinished(
    const FetchCallback& callback,
    Job* job,
    Result result,
    std::unique_ptr<std::string> data) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  JobSet::iterator it = jobs_.find(job);
  if (it == jobs_.end()) {
    // The |job| has been canceled and removed from |jobs_| already. This can
    // happen because the |backend_| runs on a different thread and a |job| may
    // finish before the cancellation has reached that thread.
    return;
  }
  callback.Run(result, std::move(data));
  jobs_.erase(it);
  delete job;
}

struct ExternalPolicyDataFetcherBackend::FetcherAndJob {
  std::unique_ptr<net::URLFetcher> fetcher;
  ExternalPolicyDataFetcher::Job* job;
};

ExternalPolicyDataFetcherBackend::ExternalPolicyDataFetcherBackend(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    scoped_refptr<net::URLRequestContextGetter> request_context)
    : io_task_runner_(io_task_runner),
      request_context_(request_context),
      last_fetch_id_(-1),
      weak_factory_(this) {
}

ExternalPolicyDataFetcherBackend::~ExternalPolicyDataFetcherBackend() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
}

std::unique_ptr<ExternalPolicyDataFetcher>
ExternalPolicyDataFetcherBackend::CreateFrontend(
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  return std::make_unique<ExternalPolicyDataFetcher>(
      task_runner, io_task_runner_, weak_factory_.GetWeakPtr());
}

void ExternalPolicyDataFetcherBackend::StartJob(
    ExternalPolicyDataFetcher::Job* job) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("external_policy_fetcher", R"(
        semantics {
          sender: "Cloud Policy"
          description:
            "Used to fetch policy for extensions, policy-controlled wallpaper, "
            "and custom terms of service."
          trigger:
            "Periodically loaded when a managed user is signed in to Chrome."
          data:
            "This request does not send any data. It loads external resources "
            "by a unique URL provided by the admin."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be controlled by Chrome settings, but users "
            "can sign out of Chrome to disable it."
          policy_exception_justification:
            "Not implemented, considered not useful. This request is part of "
            "the policy fetcher itself."
        })");
  std::unique_ptr<net::URLFetcher> owned_fetcher =
      net::URLFetcher::Create(++last_fetch_id_, job->url, net::URLFetcher::GET,
                              this, traffic_annotation);
  net::URLFetcher* fetcher = owned_fetcher.get();
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher, data_use_measurement::DataUseUserData::POLICY);
  fetcher->SetRequestContext(request_context_.get());
  fetcher->SetLoadFlags(net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                        net::LOAD_DO_NOT_SAVE_COOKIES |
                        net::LOAD_DO_NOT_SEND_COOKIES |
                        net::LOAD_DO_NOT_SEND_AUTH_DATA);
  fetcher->SetAutomaticallyRetryOnNetworkChanges(3);
  fetcher->Start();
  job_map_[fetcher] = {std::move(owned_fetcher), job};
}

void ExternalPolicyDataFetcherBackend::CancelJob(
    ExternalPolicyDataFetcher::Job* job,
    const base::Closure& callback) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  for (auto it = job_map_.begin(); it != job_map_.end();) {
    if (it->second.job == job) {
      job_map_.erase(it++);
    } else {
      ++it;
    }
  }
  callback.Run();
}

void ExternalPolicyDataFetcherBackend::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  auto it = job_map_.find(const_cast<net::URLFetcher*>(source));
  if (it == job_map_.end()) {
    NOTREACHED();
    return;
  }

  ExternalPolicyDataFetcher::Result result = ExternalPolicyDataFetcher::SUCCESS;
  std::unique_ptr<std::string> data;

  const net::URLRequestStatus status = it->first->GetStatus();
  if (status.error() == net::ERR_CONNECTION_RESET ||
      status.error() == net::ERR_TEMPORARILY_THROTTLED ||
      status.error() == net::ERR_CONNECTION_CLOSED) {
    // The connection was interrupted.
    result = ExternalPolicyDataFetcher::CONNECTION_INTERRUPTED;
  } else if (status.status() != net::URLRequestStatus::SUCCESS) {
    // Another network error occurred.
    result = ExternalPolicyDataFetcher::NETWORK_ERROR;
  } else if (source->GetResponseCode() >= 500) {
    // Problem at the server.
    result = ExternalPolicyDataFetcher::SERVER_ERROR;
  } else if (source->GetResponseCode() >= 400) {
    // Client error.
    result = ExternalPolicyDataFetcher::CLIENT_ERROR;
  } else if (source->GetResponseCode() != 200) {
    // Any other type of HTTP failure.
    result = ExternalPolicyDataFetcher::HTTP_ERROR;
  } else {
    data.reset(new std::string);
    source->GetResponseAsString(data.get());
    if (static_cast<int64_t>(data->size()) > it->second.job->max_size) {
      // Received |data| exceeds maximum allowed size.
      data.reset();
      result = ExternalPolicyDataFetcher::MAX_SIZE_EXCEEDED;
    }
  }

  ExternalPolicyDataFetcher::Job* job = it->second.job;
  job_map_.erase(it);
  job->callback.Run(job, result, std::move(data));
}

void ExternalPolicyDataFetcherBackend::OnURLFetchDownloadProgress(
    const net::URLFetcher* source,
    int64_t current,
    int64_t total,
    int64_t current_network_bytes) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  auto it = job_map_.find(source);
  DCHECK(it != job_map_.end());
  if (it == job_map_.end())
    return;

  // Reject the data if it exceeds the size limit. The content length is in
  // |total|, and it may be -1 when not known.
  ExternalPolicyDataFetcher::Job* job = it->second.job;
  int64_t max_size = job->max_size;
  if (current > max_size || total > max_size) {
    job_map_.erase(it);
    job->callback.Run(job, ExternalPolicyDataFetcher::MAX_SIZE_EXCEEDED,
                      std::unique_ptr<std::string>());
  }
}

}  // namespace policy
