// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_CLOUD_EXTERNAL_POLICY_DATA_FETCHER_H_
#define COMPONENTS_POLICY_CORE_COMMON_CLOUD_EXTERNAL_POLICY_DATA_FETCHER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/policy/policy_export.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace base {
class SequencedTaskRunner;
}

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

namespace policy {

class ExternalPolicyDataFetcherBackend;

// This class handles network fetch jobs for the ExternalPolicyDataUpdater by
// forwarding them to an ExternalPolicyDataFetcherBackend running on a different
// thread. This is necessary because the ExternalPolicyDataUpdater runs on a
// background thread where network I/O is not allowed.
// The class can be instantiated on any thread but from then on, it must be
// accessed and destroyed on the background thread that the
// ExternalPolicyDataUpdater runs on only.
class POLICY_EXPORT ExternalPolicyDataFetcher {
 public:
  // The result of a fetch job.
  enum Result {
    // Successful fetch.
    SUCCESS,
    // The connection was interrupted.
    CONNECTION_INTERRUPTED,
    // Another network error occurred.
    NETWORK_ERROR,
    // Problem at the server.
    SERVER_ERROR,
    // Client error.
    CLIENT_ERROR,
    // Any other type of HTTP failure.
    HTTP_ERROR,
    // Received data exceeds maximum allowed size.
    MAX_SIZE_EXCEEDED,
  };

  // Encapsulates the metadata for a fetch job.
  struct Job;

  // Callback invoked when a fetch job finishes. If the fetch was successful,
  // the Result is SUCCESS and the scoped_ptr contains the retrieved data.
  // Otherwise, Result indicates the type of error that occurred and the
  // scoped_ptr is NULL.
  typedef base::Callback<void(Result, std::unique_ptr<std::string>)>
      FetchCallback;

  // |task_runner| represents the background thread that |this| runs on.
  // |backend| is used to perform network I/O. It will be dereferenced and
  // accessed via |io_task_runner| only.
  ExternalPolicyDataFetcher(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      scoped_refptr<base::SequencedTaskRunner> io_task_runner,
      const base::WeakPtr<ExternalPolicyDataFetcherBackend>& backend);
  ~ExternalPolicyDataFetcher();

  // Fetch data from |url| and invoke |callback| with the result. See the
  // documentation of FetchCallback and Result for more details. If a fetch
  // should be retried after an error, it is the caller's responsibility to call
  // StartJob() again. Returns an opaque job identifier. Ownership of the job
  // identifier is retained by |this|.
  Job* StartJob(const GURL& url,
                int64_t max_size,
                const FetchCallback& callback);

  // Cancel the fetch job identified by |job|. The job is canceled silently,
  // without invoking the |callback| that was passed to StartJob().
  void CancelJob(Job* job);

 private:
  // Callback invoked when a fetch job finishes in the |backend_|.
  void OnJobFinished(const FetchCallback& callback,
                     Job* job,
                     Result result,
                     std::unique_ptr<std::string> data);

  // Task runner representing the thread that |this| runs on.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // Task runner representing the thread on which the |backend_| runs and
  // performs network I/O.
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  // The |backend_| is used to perform network I/O. It may be dereferenced and
  // accessed via |io_task_runner_| only.
  base::WeakPtr<ExternalPolicyDataFetcherBackend> backend_;

  // Set that owns all currently running Jobs.
  typedef std::set<Job*> JobSet;
  JobSet jobs_;

  base::WeakPtrFactory<ExternalPolicyDataFetcher> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExternalPolicyDataFetcher);
};

// This class handles network I/O for one or more ExternalPolicyDataFetchers. It
// can be instantiated on any thread that is allowed to reference
// URLRequestContextGetters (in Chrome, these are the UI and IO threads) and
// CreateFrontend() may be called from the same thread after instantiation. From
// then on, it must be accessed and destroyed on the thread that handles network
// I/O only (in Chrome, this is the IO thread).
class POLICY_EXPORT ExternalPolicyDataFetcherBackend
    : public net::URLFetcherDelegate {
 public:
  // Callback invoked when a fetch job finishes. If the fetch was successful,
  // the Result is SUCCESS and the scoped_ptr contains the retrieved data.
  // Otherwise, Result indicates the type of error that occurred and the
  // scoped_ptr is NULL.
  typedef base::Callback<void(ExternalPolicyDataFetcher::Job*,
                              ExternalPolicyDataFetcher::Result,
                              std::unique_ptr<std::string>)>
      FetchCallback;

  // |io_task_runner_| represents the thread that handles network I/O and that
  // |this| runs on. |request_context| is used to construct URLFetchers.
  ExternalPolicyDataFetcherBackend(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner,
      scoped_refptr<net::URLRequestContextGetter> request_context);
  ~ExternalPolicyDataFetcherBackend() override;

  // Create an ExternalPolicyDataFetcher that allows fetch jobs to be started
  // from the thread represented by |task_runner|.
  std::unique_ptr<ExternalPolicyDataFetcher> CreateFrontend(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  // Start a fetch job defined by |job|. The caller retains ownership of |job|
  // and must ensure that it remains valid until the job ends, CancelJob() is
  // called or |this| is destroyed.
  void StartJob(ExternalPolicyDataFetcher::Job* job);

  // Cancel the fetch job defined by |job| and invoke |callback| to confirm.
  void CancelJob(ExternalPolicyDataFetcher::Job* job,
                 const base::Closure& callback);

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;
  void OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                  int64_t current,
                                  int64_t total,
                                  int64_t current_network_bytes) override;

 private:
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  // A monotonically increasing fetch ID. Used to identify fetches in tests.
  int last_fetch_id_;

  // Map that owns the net::URLFetchers for all currently running jobs and maps
  // from these to the corresponding Job.
  struct FetcherAndJob;
  std::map<const net::URLFetcher*, FetcherAndJob> job_map_;

  base::WeakPtrFactory<ExternalPolicyDataFetcherBackend> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExternalPolicyDataFetcherBackend);
};


}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_CLOUD_EXTERNAL_POLICY_DATA_FETCHER_H_
