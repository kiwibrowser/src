// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A Predictor object is instantiated once in the browser process, and manages
// both preresolution of hostnames, as well as TCP/IP preconnection to expected
// subresources.
// Most hostname lists are provided by the renderer processes, and include URLs
// that *might* be used in the near future by the browsing user.  One goal of
// this class is to cause the underlying DNS structure to lookup a hostname
// before it is really needed, and hence reduce latency in the standard lookup
// paths.
// Subresource relationships are usually acquired from the referrer field in a
// navigation.  A subresource URL may be associated with a referrer URL.  Later
// navigations may, if the likelihood of needing the subresource is high enough,
// cause this module to speculatively create a TCP/IP connection. If there is
// only a low likelihood, then a DNS pre-resolution operation may be performed.

#ifndef CHROME_BROWSER_NET_PREDICTOR_H_
#define CHROME_BROWSER_NET_PREDICTOR_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/mru_cache.h"
#include "base/containers/queue.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/net/prediction_options.h"
#include "chrome/browser/net/referrer.h"
#include "chrome/browser/net/timed_cache.h"
#include "chrome/browser/net/url_info.h"
#include "components/network_hints/common/network_hints_common.h"
#include "net/base/host_port_pair.h"
#include "url/gurl.h"

class IOThread;
class PrefService;
class Profile;
class ProfileIOData;

namespace base {
class ListValue;
}

namespace net {
class HostResolver;
class SSLConfigService;
class ProxyResolutionService;
class TransportSecurityState;
class URLRequestContextGetter;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace chrome_browser_net {

class InitialObserver;
typedef std::map<GURL, UrlInfo> Results;

// An observer for testing.
class PredictorObserver {
 public:
  virtual ~PredictorObserver() {}

  virtual void OnPreconnectUrl(const GURL& original_url,
                               const GURL& site_for_cookies,
                               UrlInfo::ResolutionMotivation motivation,
                               int count) {}
  virtual void OnLearnFromNavigation(const GURL& referring_url,
                                     const GURL& target_url) {}

  virtual void OnDnsLookupFinished(const GURL& url, bool found) {}
};

// Predictor is constructed during Profile construction (on the UI thread),
// but it is destroyed on the IO thread when ProfileIOData goes away. All of
// its core state and functionality happens on the IO thread. The only UI
// methods are initialization / shutdown related (including preconnect
// initialization), or convenience methods that internally forward calls to
// the IO thread.
class Predictor {
 public:
  // A version number for prefs that are saved. This should be incremented when
  // we change the format so that we discard old data.
  static const int kPredictorReferrerVersion;

  // Given that the underlying Chromium resolver defaults to a total maximum of
  // 8 paralell resolutions, we will avoid any chance of starving navigational
  // resolutions by limiting the number of paralell speculative resolutions.
  // This is used in the field trials and testing.
  // TODO(jar): Move this limitation into the resolver.
  static const size_t kMaxSpeculativeParallelResolves;

  // To control the congestion avoidance system, we need an estimate of how
  // many speculative requests may arrive at once.  Since we currently only
  // keep 8 subresource names for each frame, we'll use that as our basis.
  // Note that when scanning search results lists, we might actually get 10 at
  // a time, and wikipedia can often supply (during a page scan) upwards of 50.
  // In those odd cases, we may discard some of the later speculative requests
  // mistakenly assuming that the resolutions took too long.
  static const int kTypicalSpeculativeGroupSize;

  // The next constant specifies an amount of queueing delay that is
  // "too large," and indicative of problems with resolutions (perhaps due to
  // an overloaded router, or such).  When we exceed this delay, congestion
  // avoidance will kick in and all speculations in the queue will be discarded.
  static const int kMaxSpeculativeResolveQueueDelayMs;

  // We don't bother learning to preconnect via a GET if the original URL
  // navigation was so long ago, that a preconnection would have been dropped
  // anyway.  We believe most servers will drop the connection in 10 seconds, so
  // we currently estimate this time-till-drop at 10 seconds.
  // TODO(jar): We should do a persistent field trial to validate/optimize this.
  static const int kMaxUnusedSocketLifetimeSecondsWithoutAGet;

  // The maximum size of the MRU cache of referrers.
  static const int kMaxReferrers;

  explicit Predictor(bool predictor_enabled);

  virtual ~Predictor();

  // This function is used to create a predictor. For testing, we can create
  // a version which does a simpler shutdown.
  // TODO(636128): This method should return a unique_ptr.
  static Predictor* CreatePredictor(bool simple_shutdown);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // ------------- Start UI thread methods.

  virtual void InitNetworkPredictor(PrefService* user_prefs,
                                    IOThread* io_thread,
                                    net::URLRequestContextGetter* getter,
                                    ProfileIOData* profile_io_data);

  // The Omnibox has proposed a given url to the user, and if it is a search
  // URL, then it also indicates that this is preconnectable (i.e., we could
  // preconnect to the search server).
  void AnticipateOmniboxUrl(const GURL& url, bool preconnectable);

  // Preconnect a URL and all of its subresource domains.
  void PreconnectUrlAndSubresources(const GURL& url,
                                    const GURL& site_for_cookies);

  static std::vector<GURL> GetPredictedUrlListAtStartup(
      PrefService* user_prefs);

  // Calls ClearPrefsOnUIThread and posts a task to the IO thread to
  // DiscardAllResults.
  void DiscardAllResultsAndClearPrefsOnUIThread();

  // Clears the preferences used by the predictor. Must be called on the UI
  // thread.
  void ClearPrefsOnUIThread();

  static void set_max_queueing_delay(int max_queueing_delay_ms);

  static void set_max_parallel_resolves(size_t max_parallel_resolves);

  virtual void ShutdownOnUIThread();

  void UpdatePrefsOnUIThread(std::unique_ptr<base::ListValue> startup_list,
                             std::unique_ptr<base::ListValue> referral_list);

  // ------------- End UI thread methods.

  // ------------- Start IO thread methods.

  void WriteDnsPrefetchState(base::ListValue* startup_list,
                             base::ListValue* referral_list);

  // Cancel pending requests and prevent new ones from being made.
  void Shutdown();

  // In some circumstances, for privacy reasons, all results should be
  // discarded.  This method gracefully handles that activity.
  // Destroy all our internal state, which shows what names we've looked up, and
  // how long each has taken, etc. etc.  We also destroy records of suggesses
  // (cache hits etc.).
  void DiscardAllResults();

  // Add hostname(s) to the queue for processing.
  void ResolveList(const std::vector<GURL>& urls,
                   UrlInfo::ResolutionMotivation motivation);

  void Resolve(const GURL& url, UrlInfo::ResolutionMotivation motivation);

  // Record details of a navigation so that we can preresolve the host name
  // ahead of time the next time the users navigates to the indicated host.
  // Should only be called when urls are distinct, and they should already be
  // canonicalized to not have a path.
  void LearnFromNavigation(const GURL& referring_url, const GURL& target_url);

  // When displaying info in about:dns, the following API is called.
  static void PredictorGetHtmlInfo(Predictor* predictor, std::string* output);

  // Dump HTML table containing list of referrers for about:dns.
  void GetHtmlReferrerLists(std::string* output);

  // Dump the list of currently known referrer domains and related prefetchable
  // domains for about:dns.
  void GetHtmlInfo(std::string* output);

  // Construct a ListValue object that contains all the data in the referrers_
  // so that it can be persisted in a pref.
  void SerializeReferrers(base::ListValue* referral_list);

  // Process a ListValue that contains all the data from a previous reference
  // list, as constructed by SerializeReferrers(), and add all the identified
  // values into the current referrer list.
  void DeserializeReferrers(const base::ListValue& referral_list);

  void DiscardInitialNavigationHistory();

  void FinalizeInitializationOnIOThread(
      const std::vector<GURL>& urls_to_prefetch,
      std::unique_ptr<base::ListValue> referral_list,
      IOThread* io_thread,
      ProfileIOData* profile_io_data);

  // During startup, we learn what the first N urls visited are, and then
  // resolve the associated hosts ASAP during our next startup.
  void LearnAboutInitialNavigation(const GURL& url);

  // Renderer bundles up list and sends to this browser API via IPC.
  // TODO(csharrison): Use a GURL vector instead to include port and scheme.
  void DnsPrefetchList(const std::vector<std::string>& hostnames);

  // May be called from either the IO or UI thread and will PostTask
  // to the IO thread if necessary.
  void DnsPrefetchMotivatedList(const std::vector<GURL>& urls,
                                UrlInfo::ResolutionMotivation motivation);

  // Called from the UI thread in response to the load event.
  void SaveStateForNextStartup();

  // May be called from either the IO or UI thread and will PostTask
  // to the IO thread if necessary.
  void PreconnectUrl(const GURL& url,
                     const GURL& site_for_cookies,
                     UrlInfo::ResolutionMotivation motivation,
                     bool allow_credentials,
                     int count);

  void PreconnectUrlOnIOThread(const GURL& url,
                               const GURL& site_for_cookies,
                               UrlInfo::ResolutionMotivation motivation,
                               bool allow_credentials,
                               int count);

  // ------------- End IO thread methods.

  // The following methods may be called on either the IO or UI threads.

  // Instigate pre-connection to any URLs, or pre-resolution of related host,
  // that we predict will be needed after this navigation (typically
  // more-embedded resources on a page).  This method will actually post a task
  // to do the actual work, so as not to jump ahead of the frame navigation that
  // instigated this activity.
  void PredictFrameSubresources(const GURL& url, const GURL& site_for_cookies);

  // Put URL in canonical form, including a scheme, host, and port.
  // Returns GURL::EmptyGURL() if the scheme is not http/https or if the url
  // cannot be otherwise canonicalized.
  static GURL CanonicalizeUrl(const GURL& url);

  // Used for testing.
  void SetTransportSecurityState(
      net::TransportSecurityState* transport_security_state) {
    transport_security_state_ = transport_security_state;
  }
  // Used for testing.
  size_t max_concurrent_dns_lookups() const {
    return max_concurrent_dns_lookups_;
  }
  // Used for testing.
  void SetShutdown(bool shutdown) {
    shutdown_ = shutdown;
  }
  // Used for testing.
  void SetObserver(PredictorObserver* observer) {
    observer_ = observer;
  }

  ProfileIOData* profile_io_data() const {
    return profile_io_data_;
  }

  bool PredictorEnabled() const;

  // Used only for testing. Overrides command line flag to disable the
  // predictor, which is added in the browser test fixture.
  void SetPredictorEnabledForTest(bool predictor_enabled);

  net::URLRequestContextGetter* url_request_context_getter_for_test() {
    return url_request_context_getter_.get();
  }

  TimedCache* timed_cache() { return timed_cache_.get(); }

 private:
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, PriorityQueuePushPopTest);
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, PriorityQueueReorderTest);
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, ReferrerSerializationTrimTest);
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, SingleLookupTestWithDisabledAdvisor);
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, SingleLookupTestWithEnabledAdvisor);
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, TestSimplePreconnectAdvisor);
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, NoProxyService);
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, ProxyDefinitelyEnabled);
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, ProxyDefinitelyNotEnabled);
  FRIEND_TEST_ALL_PREFIXES(PredictorTest, ProxyMaybeEnabled);
  friend class WaitForResolutionHelper;  // For testing.
  friend class PredictorBrowserTest;

  // A simple priority queue for handling host names.
  // Some names that are queued up have |motivation| that requires very rapid
  // handling.  For example, a sub-resource name lookup MUST be done before the
  // actual sub-resource is fetched.  In contrast, a name that was speculatively
  // noted in a page has to be resolved before the user "gets around to"
  // clicking on a link.  By tagging (with a motivation) each push we make into
  // this FIFO queue, the queue can re-order the more important names to service
  // them sooner (relative to some low priority background resolutions).
  class HostNameQueue {
   public:
    HostNameQueue();
    ~HostNameQueue();
    void Push(const GURL& url,
              UrlInfo::ResolutionMotivation motivation);
    bool IsEmpty() const;
    GURL Pop();

   private:
    // The names in the queue that should be serviced (popped) ASAP.
    base::queue<GURL> rush_queue_;
    // The names in the queue that should only be serviced when rush_queue is
    // empty.
    base::queue<GURL> background_queue_;

    DISALLOW_COPY_AND_ASSIGN(HostNameQueue);
  };

  // A map that is keyed with the host/port that we've learned were the cause
  // of loading additional URLs.  The list of additional targets is held
  // in a Referrer instance, which is a value in this map.
  typedef base::MRUCache<GURL, Referrer> Referrers;

  // Depending on the expected_subresource_use_, we may either make a TCP/IP
  // preconnection, or merely pre-resolve the hostname via DNS (or even do
  // nothing).  The following are the threasholds for taking those actions.
  static const double kPreconnectWorthyExpectedValue;
  static const double kDNSPreresolutionWorthyExpectedValue;
  static const double kDiscardableExpectedValue;

  // These two members call the appropriate global functions in
  // prediction_options.cc depending on which thread they are called on.
  virtual bool CanPrefetchAndPrerender() const;
  virtual bool CanPreresolveAndPreconnect() const;

  // ------------- Start IO thread methods.

  // Perform actual resolution or preconnection to subresources now.  This is
  // an internal worker method that is reached via a post task from
  // PredictFrameSubresources().
  void PrepareFrameSubresources(const GURL& url, const GURL& site_for_cookies);

  // Access method for use by async lookup request to pass resolution result.
  void OnLookupFinished(const GURL& url, int result);

  // Underlying method for both async and synchronous lookup to update state.
  void LookupFinished(const GURL& url, bool found);

  // Queues hostname for resolution. If the proxy advisor is enabled, and
  // |url| is likely to be proxied, the hostname will not be queued as the
  // browser is not expected to fetch it directly.
  void AppendToResolutionQueue(const GURL& url,
                               UrlInfo::ResolutionMotivation motivation);

  // Check to see if too much queuing delay has been noted for the given info,
  // which indicates that there is "congestion" or growing delay in handling the
  // resolution of names.  Rather than letting this congestion potentially grow
  // without bounds, we abandon our queued efforts at pre-resolutions in such a
  // case.
  // To do this, we will recycle |info_it|, as well as all queued items.
  // We can't do anything about the resolutions we've already sent off for
  // processing on another thread, so we just let them complete.  On a slow
  // system, subject to congestion, this will greatly reduce the number of
  // resolutions done, but it will assure that any resolutions that are done,
  // are in a timely and hence potentially helpful manner.
  bool CongestionControlPerformed(Results::iterator info_it);

  // Take lookup requests from work_queue_ and tell HostResolver to look them up
  // asynchronously, provided we don't exceed concurrent resolution limit.
  void StartSomeQueuedResolutions();

  // If we can determine immediately (i.e. synchronously) that requests to this
  // URL would likely go through a proxy, then return true.  Otherwise, return
  // false. This is used to avoid issuing DNS requests when a fixed proxy
  // configuration is in place, which improves efficiency, and is also important
  // if the unproxied DNS may contain incorrect entries.
  bool WouldLikelyProxyURL(const GURL& url);

  // Applies the HSTS redirect for |url|, if any.
  GURL GetHSTSRedirectOnIOThread(const GURL& url);

  // ------------- End IO thread methods.

  std::unique_ptr<InitialObserver> initial_observer_;

  // Reference to URLRequestContextGetter from the Profile which owns the
  // predictor. Used by Preconnect.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // Status of speculative DNS resolution and speculative TCP/IP connection
  // feature. This is true if features::kNetworkPredicton is enabled and
  // LoadingPredictor's preconnect is disabled.
  // Protected by |preconnect_enabled_lock_|, which is used by tests to bypass
  // the command line flags.
  bool predictor_enabled_;

  // This is set by InitNetworkPredictor and used for calling
  // CanPrefetchAndPrerenderUI and CanPreresolveAndPreconnectUI.
  PrefService* user_prefs_;

  // This is set by InitNetworkPredictor and used for calling
  // CanPrefetchAndPrerenderIO and CanPreresolveAndPreconnectIO.
  ProfileIOData* profile_io_data_;

  // work_queue_ holds a list of names we need to look up.
  HostNameQueue work_queue_;

  // results_ contains information for existing/prior prefetches.
  Results results_;

  size_t num_pending_lookups_;

  // For testing, to verify that we don't exceed the limit.
  size_t peak_pending_lookups_;

  // When true, we don't make new lookup requests.
  bool shutdown_;

  // The number of concurrent speculative lookups currently allowed to be sent
  // to the resolver.  Any additional lookups will be queued to avoid exceeding
  // this value.  The queue is a priority queue that will accelerate
  // sub-resource speculation, and retard resolutions suggested by page scans.
  const size_t max_concurrent_dns_lookups_;

  // The maximum queueing delay that is acceptable before we enter congestion
  // reduction mode, and discard all queued (but not yet assigned) resolutions.
  const base::TimeDelta max_dns_queue_delay_;

  // The TransportSecurityState instance we query HSTS redirects from.
  net::TransportSecurityState* transport_security_state_;

  // The SSLConfigService we query SNI support from (used in querying HSTS
  // redirects).
  net::SSLConfigService* ssl_config_service_;

  // The ProxyResolutionService, used to determine whether preresolve is useful.
  net::ProxyResolutionService* proxy_resolution_service_;

  // Most recent suggestion from Omnibox provided via AnticipateOmniboxUrl().
  std::string last_omnibox_host_;

  // The time when the last preresolve was done for last_omnibox_host_.
  base::TimeTicks last_omnibox_preresolve_;

  // The number of consecutive requests to AnticipateOmniboxUrl() that suggested
  // preconnecting (because it was to a search service).
  int consecutive_omnibox_preconnect_count_;

  // The time when the last preconnection was requested to a search service.
  base::TimeTicks last_omnibox_preconnect_;

  // For each URL that we might navigate to (that we've "learned about")
  // we have a Referrer list. Each Referrer list has all hostnames we might
  // need to pre-resolve or pre-connect to when there is a navigation to the
  // orginial hostname.
  Referrers referrers_;

  // An observer for testing.
  PredictorObserver* observer_;

  std::unique_ptr<TimedCache> timed_cache_;

  // TODO(csharrison): It is not great that two weak pointer factories are
  // needed in this class. Let's split it into two classes that live on each
  // thread.
  //
  // Weak factory for weak pointers that should be dereferenced on the IO
  // thread.
  std::unique_ptr<base::WeakPtrFactory<Predictor>> io_weak_factory_;

  // Weak factory for weak pointers that should be dereferenced on the UI
  // thread.
  std::unique_ptr<base::WeakPtrFactory<Predictor>> ui_weak_factory_;

  // Protects |predictor_enabled_|.
  mutable base::Lock predictor_enabled_lock_;

  DISALLOW_COPY_AND_ASSIGN(Predictor);
};

// This version of the predictor is used for testing.
class SimplePredictor : public Predictor {
 public:
  explicit SimplePredictor(bool predictor_enabled)
      : Predictor(predictor_enabled) {}
  ~SimplePredictor() override {}
  void InitNetworkPredictor(PrefService* user_prefs,
                            IOThread* io_thread,
                            net::URLRequestContextGetter* getter,
                            ProfileIOData* profile_io_data) override;
  void ShutdownOnUIThread() override;

 private:
  // These member functions return True for unittests.
  bool CanPrefetchAndPrerender() const override;
  bool CanPreresolveAndPreconnect() const override;
};

}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_PREDICTOR_H_
