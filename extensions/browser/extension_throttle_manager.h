// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_THROTTLE_MANAGER_H_
#define EXTENSIONS_BROWSER_EXTENSION_THROTTLE_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/threading/platform_thread.h"
#include "extensions/browser/extension_throttle_entry.h"
#include "net/base/backoff_entry.h"
#include "net/base/net_export.h"
#include "net/base/network_change_notifier.h"
#include "url/gurl.h"

namespace content {
class ResourceThrottle;
}

namespace net {
class NetLog;
class NetLogWithSource;
}

namespace extensions {

// Class that registers URL request throttler entries for URLs being accessed
// in order to supervise traffic. URL requests for HTTP contents should
// register their URLs in this manager on each request.
//
// ExtensionThrottleManager maintains a map of URL IDs to URL request
// throttler entries. It creates URL request throttler entries when new URLs
// are registered, and does garbage collection from time to time in order to
// clean out outdated entries. URL ID consists of lowercased scheme, host, port
// and path. All URLs converted to the same ID will share the same entry.
class ExtensionThrottleManager
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  ExtensionThrottleManager();
  ~ExtensionThrottleManager() override;

  // Creates a content::ResourceThrottle for |request| to prevent extensions
  // from requesting a URL too often, if such a throttle is needed.
  std::unique_ptr<content::ResourceThrottle> MaybeCreateThrottle(
      const net::URLRequest* request);

  // TODO(xunjieli): Remove this method and replace with
  // ShouldRejectRequest(request) and UpdateWithResponse(request, status_code),
  // which will also allow ExtensionThrottleEntry to no longer be reference
  // counted, and ExtensionThrottleEntryInterface to be removed.

  // Must be called for every request, returns the URL request throttler entry
  // associated with the URL. The caller must inform this entry of some events.
  // Please refer to extension_throttle_entry_interface.h for further
  // informations.
  scoped_refptr<ExtensionThrottleEntryInterface> RegisterRequestUrl(
      const GURL& url);

  void SetBackoffPolicyForTests(
      std::unique_ptr<net::BackoffEntry::Policy> policy);

  // Registers a new entry in this service and overrides the existing entry (if
  // any) for the URL. The service will hold a reference to the entry.
  // It is only used by unit tests.
  void OverrideEntryForTests(const GURL& url, ExtensionThrottleEntry* entry);

  // Explicitly erases an entry.
  // This is useful to remove those entries which have got infinite lifetime and
  // thus won't be garbage collected.
  // It is only used by unit tests.
  void EraseEntryForTests(const GURL& url);

  // Sets whether to ignore net::LOAD_MAYBE_USER_GESTURE of the request for
  // testing. Otherwise, requests will not be throttled when they may have been
  // throttled in response to a recent user gesture, though they're still
  // counted for the purpose of throttling other requests.
  void SetIgnoreUserGestureLoadFlagForTests(
      bool ignore_user_gesture_load_flag_for_tests);

  // Turns threading model verification on or off.  Any code that correctly
  // uses the network stack should preferably call this function to enable
  // verification of correct adherence to the network stack threading model.
  void set_enable_thread_checks(bool enable);
  bool enable_thread_checks() const;

  // Whether throttling is enabled or not.
  void set_enforce_throttling(bool enforce);
  bool enforce_throttling();

  // Sets the net::NetLog instance to use.
  void set_net_log(net::NetLog* net_log);
  net::NetLog* net_log() const;

  // NetworkChangeObserver interface.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // Method that allows us to transform a URL into an ID that can be used in our
  // map. Resulting IDs will be lowercase and consist of the scheme, host, port
  // and path (without query string, fragment, etc.).
  // If the URL is invalid, the invalid spec will be returned, without any
  // transformation.
  std::string GetIdFromUrl(const GURL& url) const;

  // Method that ensures the map gets cleaned from time to time. The period at
  // which garbage collecting happens is adjustable with the
  // kRequestBetweenCollecting constant.
  void GarbageCollectEntriesIfNecessary();

  // Method that does the actual work of garbage collecting.
  void GarbageCollectEntries();

  // Used by tests.
  int GetNumberOfEntriesForTests() const { return url_entries_.size(); }

 private:
  // From each URL we generate an ID composed of the scheme, host, port and path
  // that allows us to uniquely map an entry to it.
  typedef std::map<std::string, scoped_refptr<ExtensionThrottleEntry>>
      UrlEntryMap;

  // Maximum number of entries that we are willing to collect in our map.
  static const unsigned int kMaximumNumberOfEntries;
  // Number of requests that will be made between garbage collection.
  static const unsigned int kRequestsBetweenCollecting;

  // Map that contains a list of URL ID and their matching
  // ExtensionThrottleEntry.
  UrlEntryMap url_entries_;

  // This keeps track of how many requests have been made. Used with
  // GarbageCollectEntries.
  unsigned int requests_since_last_gc_;

  // Valid after construction.
  GURL::Replacements url_id_replacements_;

  // Certain tests do not obey the net component's threading policy, so we
  // keep track of whether we're being used by tests, and turn off certain
  // checks.
  //
  // TODO(joi): See if we can fix the offending unit tests and remove this
  // workaround.
  bool enable_thread_checks_;

  // Initially false, switches to true once we have logged because of back-off
  // being disabled for localhost.
  bool logged_for_localhost_disabled_;

  // net::NetLog to use, if configured.
  net::NetLogWithSource net_log_;

  // Valid once we've registered for network notifications.
  base::PlatformThreadId registered_from_thread_;

  bool ignore_user_gesture_load_flag_for_tests_;

  // This is NULL when it is not set for tests.
  std::unique_ptr<net::BackoffEntry::Policy> backoff_policy_for_tests_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ExtensionThrottleManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_THROTTLE_MANAGER_H_
