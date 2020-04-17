// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GOOGLE_CORE_BROWSER_SEARCH_URL_TRACKER_H_
#define COMPONENTS_GOOGLE_CORE_BROWSER_SEARCH_URL_TRACKER_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/callback_list.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/google/core/browser/search_url_tracker_client.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/base/network_change_notifier.h"
#include "url/gurl.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace network {
class SimpleURLLoader;
}  // namespace network

// This object is responsible for checking the Google URL once per network
// change.  The current value is saved to prefs.
//
// Most consumers should only call search_url().  Consumers who need to be
// notified when things change should use RegisterCallback().
//
// To protect users' privacy and reduce server load, no updates will be
// performed (ever) unless at least one consumer registers interest by calling
// RequestServerCheck().
class SearchURLTracker
    : public net::NetworkChangeNotifier::NetworkChangeObserver,
      public KeyedService {
 public:
  // Callback that is called when the Google URL is updated.
  typedef base::Callback<void()> OnSearchURLUpdatedCallback;
  typedef base::CallbackList<void()> CallbackList;
  typedef CallbackList::Subscription Subscription;

  // The mode of the tracker that controls how the tracker behaves and that must
  // be passed to its constructor.
  enum Mode {
    // Use current local Google TLD.
    // Defer network requests to update TLD until 5 seconds after
    // creation, to avoid an expensive load during Chrome startup.
    NORMAL_MODE,

    // Always use www.google.com.
    ALWAYS_DOT_COM_MODE,
  };

  static const char kDefaultGoogleHomepage[];

  // Only the SearchURLTrackerFactory and tests should call this.
  // Note: you *must* manually call Shutdown() before this instance gets
  // destroyed if you want to create another instance in the same binary
  // (e.g. in unit tests).
  SearchURLTracker(std::unique_ptr<SearchURLTrackerClient> client, Mode mode);

  ~SearchURLTracker() override;

  // Register user preferences for SearchURLTracker.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns the current version of the Search Engines list.
  int search_version() const { return search_version_; }

  // Requests that the tracker perform a server check to update the Google URL
  // as necessary.  This will happen at most once per network change, not sooner
  // than five seconds after startup (checks requested before that time will
  // occur then; checks requested afterwards will occur immediately, if no other
  // checks have been made during this run).
  void RequestServerCheck();

  std::unique_ptr<Subscription> RegisterCallback(
      const OnSearchURLUpdatedCallback& cb);

 private:
  friend class SearchURLTrackerTest;
  friend class SyncTest;

  static const char kSearchDomainCheckURL[];

  void OnURLLoaderComplete(std::unique_ptr<std::string> response_body);

  // NetworkChangeNotifier::NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // KeyedService:
  void Shutdown() override;

  // Sets |need_to_load_| and attempts to start a load.
  void SetNeedToLoad();

  // Called when the five second startup sleep has finished.  Runs any pending
  // load.
  void FinishSleep();

  // Starts the load of the up-to-date Google URL if we actually want to load
  // it and can currently do so.
  void StartLoadIfDesirable();

  CallbackList callback_list_;

  std::unique_ptr<SearchURLTrackerClient> client_;

  int search_version_;
  std::unique_ptr<network::SimpleURLLoader> simple_loader_;
  bool in_startup_sleep_;  // True if we're in the five-second "no loading"
                           // period that begins at browser start.
  bool already_loaded_;    // True if we've already loaded a URL once this run;
                           // we won't load again until after a restart.
  bool need_to_load_;      // True if a consumer actually wants us to load an
                           // updated URL.  If this is never set, we won't
                           // bother to load anything.
                           // Consumers should register a callback via
                           // RegisterCallback().
  base::WeakPtrFactory<SearchURLTracker> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SearchURLTracker);
};

#endif  // COMPONENTS_GOOGLE_CORE_BROWSER_SEARCH_URL_TRACKER_H_
