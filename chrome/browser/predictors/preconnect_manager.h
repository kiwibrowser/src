// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREDICTORS_PRECONNECT_MANAGER_H_
#define CHROME_BROWSER_PREDICTORS_PRECONNECT_MANAGER_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/completion_callback.h"
#include "net/http/http_request_info.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace predictors {

struct PreconnectRequest;

struct PreconnectedRequestStats {
  PreconnectedRequestStats(const GURL& origin,
                           bool was_preresolve_cached,
                           bool was_preconnected);
  PreconnectedRequestStats(const PreconnectedRequestStats& other);
  ~PreconnectedRequestStats();

  GURL origin;
  bool was_preresolve_cached;
  bool was_preconnected;
};

struct PreconnectStats {
  explicit PreconnectStats(const GURL& url);
  ~PreconnectStats();

  GURL url;
  base::TimeTicks start_time;
  std::vector<PreconnectedRequestStats> requests_stats;

  // Stats must be moved only.
  DISALLOW_COPY_AND_ASSIGN(PreconnectStats);
};

// Stores the status of all preconnects associated with a given |url|.
struct PreresolveInfo {
  PreresolveInfo(const GURL& url, size_t count);
  ~PreresolveInfo();

  bool is_done() const { return queued_count == 0 && inflight_count == 0; }

  GURL url;
  size_t queued_count;
  size_t inflight_count = 0;
  bool was_canceled = false;
  std::unique_ptr<PreconnectStats> stats;

  DISALLOW_COPY_AND_ASSIGN(PreresolveInfo);
};

// Stores all data need for running a preresolve and a subsequent optional
// preconnect for a |url|.
struct PreresolveJob {
  PreresolveJob(const GURL& url,
                int num_sockets,
                bool allow_credentials,
                PreresolveInfo* info);
  PreresolveJob(const PreresolveJob& other);
  ~PreresolveJob();
  bool need_preconnect() const { return num_sockets > 0; }

  GURL url;
  int num_sockets;
  bool allow_credentials;
  // Raw pointer usage is fine here because even though PreresolveJob can
  // outlive PreresolveInfo it's only accessed on PreconnectManager class
  // context and PreresolveInfo lifetime is tied to PreconnectManager.
  // May be equal to nullptr in case of detached job.
  PreresolveInfo* info;
};

// PreconnectManager is responsible for preresolving and preconnecting to
// origins based on the input list of URLs.
//  - The input list of URLs is associated with a main frame url that can be
//  used for cancelling.
//  - Limits the total number of preresolves in flight.
//  - Preresolves an URL before preconnecting to it to have a better control on
//  number of speculative dns requests in flight.
//  - When stopped, waits for the pending preresolve requests to finish without
//  issuing preconnects for them.
//  - All methods of the class except the constructor must be called on the IO
//  thread. The constructor must be called on the UI thread.
class PreconnectManager {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called when all preresolve jobs for the |stats->url| are finished. Note
    // that some preconnect jobs can be still in progress, because they are
    // fire-and-forget.
    // Is called on the UI thread.
    virtual void PreconnectFinished(std::unique_ptr<PreconnectStats> stats) = 0;
  };

  static const size_t kMaxInflightPreresolves = 3;

  PreconnectManager(base::WeakPtr<Delegate> delegate,
                    scoped_refptr<net::URLRequestContextGetter> context_getter);
  virtual ~PreconnectManager();

  // Starts preconnect and preresolve jobs keyed by |url|.
  virtual void Start(const GURL& url,
                     std::vector<PreconnectRequest>&& requests);

  // Starts special preconnect and preresolve jobs that are not cancellable and
  // don't report about their completion. They are considered more important
  // than trackable requests thus they are put in the front of the jobs queue.
  virtual void StartPreresolveHost(const GURL& url);
  virtual void StartPreresolveHosts(const std::vector<std::string>& hostnames);
  virtual void StartPreconnectUrl(const GURL& url, bool allow_credentials);

  // No additional jobs keyed by the |url| will be queued after this.
  virtual void Stop(const GURL& url);

  // Public for mocking in unit tests. Don't use, internal only.
  virtual void PreconnectUrl(const GURL& url,
                             const GURL& site_for_cookies,
                             int num_sockets,
                             bool allow_credentials) const;
  virtual int PreresolveUrl(const GURL& url,
                            const net::CompletionCallback& callback) const;

 private:
  void TryToLaunchPreresolveJobs();
  void OnPreresolveFinished(const PreresolveJob& job, int result);
  void FinishPreresolve(const PreresolveJob& job, bool found, bool cached);
  void AllPreresolvesForUrlFinished(PreresolveInfo* info);
  GURL GetHSTSRedirect(const GURL& url) const;
  bool WouldLikelyProxyURL(const GURL& url) const;

  base::WeakPtr<Delegate> delegate_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;
  std::list<PreresolveJob> queued_jobs_;
  std::map<std::string, std::unique_ptr<PreresolveInfo>> preresolve_info_;
  size_t inflight_preresolves_count_ = 0;

  base::WeakPtrFactory<PreconnectManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PreconnectManager);
};

}  // namespace predictors

#endif  // CHROME_BROWSER_PREDICTORS_PRECONNECT_MANAGER_H_
