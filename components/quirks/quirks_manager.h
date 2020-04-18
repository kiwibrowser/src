// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_QUIRKS_QUIRKS_MANAGER_H_
#define COMPONENTS_QUIRKS_QUIRKS_MANAGER_H_

#include <memory>
#include <set>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/quirks/quirks_export.h"

class GURL;
class PrefRegistrySimple;
class PrefService;

namespace base {
class TaskRunner;
}

namespace net {
class URLFetcher;
class URLFetcherDelegate;
class URLRequestContextGetter;
}

namespace quirks {

class QuirksClient;

// Callback when Quirks path request is complete.
// First parameter - path found, or empty if no file.
// Second parameter - true if file was just downloaded.
using RequestFinishedCallback =
    base::Callback<void(const base::FilePath&, bool)>;

// Format int as hex string for filename.
QUIRKS_EXPORT std::string IdToHexString(int64_t product_id);

// Append ".icc" to hex string in filename.
QUIRKS_EXPORT std::string IdToFileName(int64_t product_id);

// Manages downloads of and requests for hardware calibration and configuration
// files ("Quirks").  The manager presents an external Quirks API, handles
// needed components from browser (local preferences, url context getter,
// blocking pool, etc), and owns clients and manages their life cycles.
class QUIRKS_EXPORT QuirksManager {
 public:
  // Passed function to create a URLFetcher for tests.
  // Same parameters as URLFetcher::Create().
  using FakeQuirksFetcherCreator = base::Callback<
      std::unique_ptr<net::URLFetcher>(const GURL&, net::URLFetcherDelegate*)>;

  // Delegate class, so implementation can access browser functionality.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Provides Chrome API key for quirks server.
    virtual std::string GetApiKey() const = 0;

    // Returns the path to the writable display profile directory.
    // This directory must already exist.
    virtual base::FilePath GetDisplayProfileDirectory() const = 0;

    // Whether downloads are allowed by enterprise device policy.
    virtual bool DevicePolicyEnabled() const = 0;

   private:
    DISALLOW_ASSIGN(Delegate);
  };

  static void Initialize(
      std::unique_ptr<Delegate> delegate,
      PrefService* local_state,
      scoped_refptr<net::URLRequestContextGetter> url_context_getter);
  static void Shutdown();
  static QuirksManager* Get();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Signal to start queued downloads after login.
  void OnLoginCompleted();

  // Entry point into manager.  Finds or downloads icc file.
  void RequestIccProfilePath(
      int64_t product_id,
      const std::string& display_name,
      const RequestFinishedCallback& on_request_finished);

  void ClientFinished(QuirksClient* client);

  // Creates a real URLFetcher for OS, and a fake one for tests.
  std::unique_ptr<net::URLFetcher> CreateURLFetcher(
      const GURL& url,
      net::URLFetcherDelegate* delegate);

  Delegate* delegate() { return delegate_.get(); }
  base::TaskRunner* task_runner() { return task_runner_.get(); }
  net::URLRequestContextGetter* url_context_getter() {
    return url_context_getter_.get();
  }

 protected:
  friend class QuirksBrowserTest;

  void SetFakeQuirksFetcherCreatorForTests(
      const FakeQuirksFetcherCreator& creator) {
    fake_quirks_fetcher_creator_ = creator;
  }

 private:
  QuirksManager(std::unique_ptr<Delegate> delegate,
                PrefService* local_state,
                scoped_refptr<net::URLRequestContextGetter> url_context_getter);
  ~QuirksManager();

  // Callback after checking for existing icc file; proceed if not found.
  void OnIccFilePathRequestCompleted(
      int64_t product_id,
      const std::string& display_name,
      const RequestFinishedCallback& on_request_finished,
      base::FilePath path);

  // Whether downloads allowed by cmd line flag and device policy.
  bool QuirksEnabled();

  // Records time of most recent server check.
  void SetLastServerCheck(int64_t product_id, const base::Time& last_check);

  // Set of active clients, each created to download a different Quirks file.
  std::set<std::unique_ptr<QuirksClient>> clients_;

  // Don't start downloads before first session login.
  bool waiting_for_login_;

  // Ensure this class runs on a single thread.
  base::ThreadChecker thread_checker_;

  // These objects provide resources from the browser.
  std::unique_ptr<Delegate> delegate_;  // Impl runs from chrome/browser.
  scoped_refptr<base::TaskRunner> task_runner_;
  PrefService* local_state_;  // For local prefs.
  scoped_refptr<net::URLRequestContextGetter> url_context_getter_;

  FakeQuirksFetcherCreator fake_quirks_fetcher_creator_;  // For tests.

  // Factory for callbacks.
  base::WeakPtrFactory<QuirksManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(QuirksManager);
};

}  // namespace quirks

#endif  // COMPONENTS_QUIRKS_QUIRKS_MANAGER_H_
