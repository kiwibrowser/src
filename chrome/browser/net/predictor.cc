// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/predictor.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>
#include <sstream>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/mru_cache.h"
#include "base/feature_list.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/predictors/loading_predictor_config.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_hints.h"
#include "net/base/address_list.h"
#include "net/base/completion_callback.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/http/transport_security_state.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/ssl/ssl_config_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

using base::TimeDelta;
using content::BrowserThread;

namespace chrome_browser_net {

namespace {

#if defined(OS_ANDROID)
// Disabled on Android, as there are no "pinned tabs", meaning that a startup
// is unlikely to request the same URL, and hence to resolve the same domains
// as the previous one.
constexpr bool kInitialDnsPrefetchListEnabled = false;
#else
constexpr bool kInitialDnsPrefetchListEnabled = true;
#endif  // defined(OS_ANDROID)

}  // namespace

// The InitialObserver monitors navigations made by the network stack. This
// is only used to identify startup time resolutions (for re-resolution
// during our next process startup).
// TODO(jar): Consider preconnecting at startup, which may be faster than
// waiting for render process to start and request a connection.
class InitialObserver {
 public:
  InitialObserver();
  ~InitialObserver();
  // Recording of when we observed each navigation.
  typedef std::map<GURL, base::TimeTicks> FirstNavigations;

  // Potentially add a new URL to our startup list.
  void Append(const GURL& url, Predictor* predictor);

  // Get an HTML version of our current planned first_navigations_.
  void GetFirstResolutionsHtml(std::string* output);

  // Persist the current first_navigations_ for storage in a list.
  void GetInitialDnsResolutionList(base::ListValue* startup_list);

  // Discards all initial loading history.
  void DiscardInitialNavigationHistory() { first_navigations_.clear(); }

 private:
  // List of the first N URL resolutions observed in this run.
  FirstNavigations first_navigations_;

  // The number of URLs we'll save for pre-resolving at next startup.
  static const size_t kStartupResolutionCount = 10;
};

// static
const int Predictor::kPredictorReferrerVersion = 2;
const double Predictor::kPreconnectWorthyExpectedValue = 0.8;
const double Predictor::kDNSPreresolutionWorthyExpectedValue = 0.1;
const double Predictor::kDiscardableExpectedValue = 0.05;
const size_t Predictor::kMaxSpeculativeParallelResolves = 3;
const int Predictor::kMaxUnusedSocketLifetimeSecondsWithoutAGet = 10;

// This number was obtained by the Net.Predictor.MRUIndex histogram on Canary
// and Dev channel (M53). The database size was initialized to 1000, and the
// histogram logged the index into the MRU in PrepareFrameSubresources. The
// results showed that 99% of all accesses used the first 100 elements, and
// 99.5% of all accesses used the first 170 elements.
const int Predictor::kMaxReferrers = 100;

// To control our congestion avoidance system, which discards a queue when
// resolutions are "taking too long," we need an expected resolution time.
// Common average is in the range of 300-500ms.
const int kExpectedResolutionTimeMs = 500;
const int Predictor::kTypicalSpeculativeGroupSize = 8;
const int Predictor::kMaxSpeculativeResolveQueueDelayMs =
    (kExpectedResolutionTimeMs * Predictor::kTypicalSpeculativeGroupSize) /
    Predictor::kMaxSpeculativeParallelResolves;

// The default value of the credentials flag when preconnecting.
static constexpr bool kAllowCredentialsOnPreconnectByDefault = true;

static int g_max_queueing_delay_ms =
    Predictor::kMaxSpeculativeResolveQueueDelayMs;
static size_t g_max_parallel_resolves =
    Predictor::kMaxSpeculativeParallelResolves;

// A version number for prefs that are saved. This should be incremented when
// we change the format so that we discard old data.
static const int kPredictorStartupFormatVersion = 1;

Predictor::Predictor(bool predictor_enabled)
    : url_request_context_getter_(nullptr),
      predictor_enabled_(predictor_enabled),
      user_prefs_(nullptr),
      profile_io_data_(nullptr),
      num_pending_lookups_(0),
      peak_pending_lookups_(0),
      shutdown_(false),
      max_concurrent_dns_lookups_(g_max_parallel_resolves),
      max_dns_queue_delay_(
          TimeDelta::FromMilliseconds(g_max_queueing_delay_ms)),
      transport_security_state_(nullptr),
      ssl_config_service_(nullptr),
      proxy_resolution_service_(nullptr),
      consecutive_omnibox_preconnect_count_(0),
      referrers_(kMaxReferrers),
      observer_(nullptr),
      timed_cache_(new TimedCache(base::TimeDelta::FromSeconds(
          kMaxUnusedSocketLifetimeSecondsWithoutAGet))),
      ui_weak_factory_(new base::WeakPtrFactory<Predictor>(this)) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

Predictor::~Predictor() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(shutdown_);
}

// static
Predictor* Predictor::CreatePredictor(bool simple_shutdown) {
  bool predictor_enabled =
      base::FeatureList::IsEnabled(features::kNetworkPrediction) &&
      !predictors::ShouldDisableOtherPreconnects();
  if (simple_shutdown)
    return new SimplePredictor(predictor_enabled);
  return new Predictor(predictor_enabled);
}

void Predictor::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(prefs::kDnsPrefetchingStartupList,
                             PrefRegistry::LOSSY_PREF);
  registry->RegisterListPref(prefs::kDnsPrefetchingHostReferralList,
                             PrefRegistry::LOSSY_PREF);
}

// --------------------- Start UI methods. ------------------------------------

void Predictor::InitNetworkPredictor(PrefService* user_prefs,
                                     IOThread* io_thread,
                                     net::URLRequestContextGetter* getter,
                                     ProfileIOData* profile_io_data) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  user_prefs_ = user_prefs;
  url_request_context_getter_ = getter;

  // Gather the list of hostnames to prefetch on startup.
  std::vector<GURL> urls = GetPredictedUrlListAtStartup(user_prefs);

  std::unique_ptr<base::ListValue> referral_list = base::WrapUnique(
      user_prefs->GetList(prefs::kDnsPrefetchingHostReferralList)->DeepCopy());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&Predictor::FinalizeInitializationOnIOThread,
                     base::Unretained(this), urls, std::move(referral_list),
                     io_thread, profile_io_data));
}

void Predictor::AnticipateOmniboxUrl(const GURL& url, bool preconnectable) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!PredictorEnabled())
    return;
  if (!url.is_valid() || !url.has_host())
    return;
  if (!CanPreresolveAndPreconnect())
    return;

  std::string host = url.HostNoBrackets();
  bool is_new_host_request = (host != last_omnibox_host_);
  last_omnibox_host_ = host;

  UrlInfo::ResolutionMotivation motivation(UrlInfo::OMNIBOX_MOTIVATED);
  base::TimeTicks now = base::TimeTicks::Now();

  if (preconnectable && !is_new_host_request) {
    ++consecutive_omnibox_preconnect_count_;
    // The omnibox suggests a search URL (for which we can preconnect) after
    // one or two characters are typed, even though such typing often (1 in
    // 3?) becomes a real URL.  This code waits till is has more evidence of a
    // preconnectable URL (search URL) before forming a preconnection, so as
    // to reduce the useless preconnect rate.
    // Perchance this logic should be pushed back into the omnibox, where the
    // actual characters typed, such as a space, can better forcast whether
    // we need to search/preconnect or not.  By waiting for at least 4
    // characters in a row that have lead to a search proposal, we avoid
    // preconnections for a prefix like "www." and we also wait until we have
    // at least a 4 letter word to search for.
    // Each character typed appears to induce 2 calls to AnticipateOmniboxUrl(),
    // so we double 4 characters and limit at 8 requests.
    // TODO(jar): Use an A/B test to optimize this.
    const int kMinConsecutiveRequests = 8;
    if (consecutive_omnibox_preconnect_count_ >= kMinConsecutiveRequests) {
      // TODO(jar): Perhaps we should do a GET to leave the socket open in the
      // pool.  Currently, we just do a connect, which MAY be reset if we
      // don't use it in 10 secondes!!!  As a result, we may do more
      // connections, and actually cost the server more than if we did a real
      // get with a fake request (/gen_204 might be the good path on Google).
      const int kMaxSearchKeepaliveSeconds(10);
      if ((now - last_omnibox_preconnect_).InSeconds() <
          kMaxSearchKeepaliveSeconds)
        return;  // We've done a preconnect recently.
      last_omnibox_preconnect_ = now;
      const int kConnectionsNeeded = 1;
      PreconnectUrl(CanonicalizeUrl(url), GURL(), motivation,
                    kAllowCredentialsOnPreconnectByDefault, kConnectionsNeeded);
      return;  // Skip pre-resolution, since we'll open a connection.
    }
  } else {
    consecutive_omnibox_preconnect_count_ = 0;
  }

  // Fall through and consider pre-resolution.

  // Omnibox tends to call in pairs (just a few milliseconds apart), and we
  // really don't need to keep resolving a name that often.
  // TODO(jar): A/B tests could check for perf impact of the early returns.
  if (!is_new_host_request) {
    const int kMinPreresolveSeconds(10);
    if (kMinPreresolveSeconds > (now - last_omnibox_preresolve_).InSeconds())
      return;
  }
  last_omnibox_preresolve_ = now;

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&Predictor::Resolve, base::Unretained(this),
                     CanonicalizeUrl(url), motivation));
}

void Predictor::PreconnectUrlAndSubresources(const GURL& url,
                                             const GURL& site_for_cookies) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
         BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!PredictorEnabled())
    return;
  if (!CanPreresolveAndPreconnect())
    return;
  const GURL canonicalized_url = CanonicalizeUrl(url);
  if (!canonicalized_url.is_valid() || !canonicalized_url.has_host())
    return;
  UrlInfo::ResolutionMotivation motivation(UrlInfo::EARLY_LOAD_MOTIVATED);
  const int kConnectionsNeeded = 1;
  PreconnectUrl(canonicalized_url, site_for_cookies, motivation,
                kAllowCredentialsOnPreconnectByDefault, kConnectionsNeeded);
  PredictFrameSubresources(canonicalized_url.GetWithEmptyPath(),
                           site_for_cookies);
}

std::vector<GURL> Predictor::GetPredictedUrlListAtStartup(
    PrefService* user_prefs) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::vector<GURL> urls;
  // Recall list of URLs we learned about during last session.
  // This may catch secondary hostnames, pulled in by the homepages.  It will
  // also catch more of the "primary" home pages, since that was (presumably)
  // rendered first (and will be rendered first this time too).
  const base::ListValue* startup_list =
      user_prefs->GetList(prefs::kDnsPrefetchingStartupList);

  if (startup_list) {
    base::ListValue::const_iterator it = startup_list->begin();
    int format_version = -1;
    if (it != startup_list->end() && it->GetAsInteger(&format_version) &&
        format_version == kPredictorStartupFormatVersion) {
      ++it;
      for (; it != startup_list->end(); ++it) {
        std::string url_spec;
        if (!it->GetAsString(&url_spec)) {
          LOG(DFATAL);
          break;  // Format incompatibility.
        }
        GURL url(url_spec);
        if (!url.has_host() || !url.has_scheme()) {
          LOG(DFATAL);
          break;  // Format incompatibility.
        }

        urls.push_back(url);
      }
    }
  }

  // Prepare for any static home page(s) the user has in prefs.  The user may
  // have a LOT of tab's specified, so we may as well try to warm them all.
  SessionStartupPref tab_start_pref =
      SessionStartupPref::GetStartupPref(user_prefs);
  if (SessionStartupPref::URLS == tab_start_pref.type) {
    for (size_t i = 0; i < tab_start_pref.urls.size(); i++) {
      GURL gurl = tab_start_pref.urls[i];
      if (!gurl.is_valid() || gurl.SchemeIsFile() || gurl.host_piece().empty())
        continue;
      if (gurl.SchemeIsHTTPOrHTTPS())
        urls.push_back(gurl.GetWithEmptyPath());
    }
  }

  if (urls.empty())
    urls.push_back(GURL("http://www.google.com:80"));

  return urls;
}

void Predictor::DiscardAllResultsAndClearPrefsOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // The post task here is guaranteed to execute before the post task in
  // ShutdownOnUIThread, because the caller has a valid profile here. Note that
  // the ChromeNetBenchmarkingMessageFilter calls unsafely (an existing bug)
  // into the profile, but doing so would crash before this point anyways.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&Predictor::DiscardAllResults, base::Unretained(this)));
  ClearPrefsOnUIThread();
}

void Predictor::ClearPrefsOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  user_prefs_->ClearPref(prefs::kDnsPrefetchingStartupList);
  user_prefs_->ClearPref(prefs::kDnsPrefetchingHostReferralList);
}

void Predictor::set_max_queueing_delay(int max_queueing_delay_ms) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  g_max_queueing_delay_ms = max_queueing_delay_ms;
}

void Predictor::set_max_parallel_resolves(size_t max_parallel_resolves) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  g_max_parallel_resolves = max_parallel_resolves;
}

void Predictor::ShutdownOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ui_weak_factory_->InvalidateWeakPtrs();
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&Predictor::Shutdown, base::Unretained(this)));
}

// ---------------------- End UI methods. -------------------------------------

// --------------------- Start IO methods. ------------------------------------

void Predictor::Shutdown() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!shutdown_);
  shutdown_ = true;
}

void Predictor::DiscardAllResults() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Delete anything listed so far in this session that shows in about:dns.
  referrers_.Clear();

  // Try to delete anything in our work queue.
  while (!work_queue_.IsEmpty()) {
    // Emulate processing cycle as though host was not found.
    GURL url = work_queue_.Pop();
    UrlInfo* info = &results_[url];
    DCHECK(info->HasUrl(url));
    info->SetAssignedState();
    info->SetNoSuchNameState();
  }
  // Now every result_ is either resolved, or is being resolved.

  // Step through result_, recording names of all hosts that can't be erased.
  // We can't erase anything being worked on.
  Results assignees;
  for (Results::iterator it = results_.begin(); results_.end() != it; ++it) {
    GURL url(it->first);
    UrlInfo* info = &it->second;
    DCHECK(info->HasUrl(url));
    if (info->is_assigned()) {
      info->SetPendingDeleteState();
      assignees[url] = *info;
    }
  }
  DCHECK_LE(assignees.size(), max_concurrent_dns_lookups_);
  results_.clear();
  // Put back in the names being worked on.
  for (Results::iterator it = assignees.begin(); assignees.end() != it; ++it) {
    DCHECK(it->second.is_marked_to_delete());
    results_[it->first] = it->second;
  }
}

// Overloaded Resolve() to take a vector of names.
void Predictor::ResolveList(const std::vector<GURL>& urls,
                            UrlInfo::ResolutionMotivation motivation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (std::vector<GURL>::const_iterator it = urls.begin(); it < urls.end();
       ++it) {
    AppendToResolutionQueue(*it, motivation);
  }
}

// Basic Resolve() takes an invidual name, and adds it
// to the queue.
void Predictor::Resolve(const GURL& url,
                        UrlInfo::ResolutionMotivation motivation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!url.has_host())
    return;
  AppendToResolutionQueue(url, motivation);
}

void Predictor::LearnFromNavigation(const GURL& referring_url,
                                    const GURL& target_url) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!PredictorEnabled() || !CanPreresolveAndPreconnect())
    return;
  DCHECK_EQ(referring_url, Predictor::CanonicalizeUrl(referring_url));
  DCHECK_NE(referring_url, GURL::EmptyGURL());
  DCHECK_EQ(target_url, Predictor::CanonicalizeUrl(target_url));
  DCHECK_NE(target_url, GURL::EmptyGURL());

  // Skip HSTS redirects to learn the true referrer.
  GURL referring_url_with_hsts = GetHSTSRedirectOnIOThread(referring_url);

  if (observer_)
    observer_->OnLearnFromNavigation(referring_url_with_hsts, target_url);
  // Peek here, as Get() occurs on the actual navigation. Note, Put is used here
  // due to the fact that on a new navigation, it is unclear whether the URL
  // will be a referrer to any subresource. That could result in bloating the
  // database with empty entries.
  Referrers::iterator it = referrers_.Peek(referring_url_with_hsts);
  if (it == referrers_.end())
    it = referrers_.Put(referring_url_with_hsts, Referrer());

  it->second.SuggestHost(target_url);
}

//-----------------------------------------------------------------------------
// This section supports the about:dns page.

void Predictor::PredictorGetHtmlInfo(Predictor* predictor,
                                     std::string* output) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  output->append("<html><head><title>About DNS</title>"
                 // We'd like the following no-cache... but it doesn't work.
                 // "<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">"
                 "</head><body>");
  if (predictor && predictor->PredictorEnabled() &&
      predictor->CanPreresolveAndPreconnect()) {
    predictor->GetHtmlInfo(output);
  } else {
    output->append("DNS pre-resolution and TCP pre-connection is disabled.");
  }
  output->append("</body></html>");
}

void Predictor::GetHtmlReferrerLists(std::string* output) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (referrers_.empty())
    return;

  // TODO(jar): Remove any plausible JavaScript from names before displaying.
  output->append("<br><table border>");
  output->append(
      "<tr><th>Host for Page</th>"
      "<th>Page Load<br>Count</th>"
      "<th>Subresource<br>Navigations</th>"
      "<th>Subresource<br>PreConnects</th>"
      "<th>Subresource<br>PreResolves</th>"
      "<th>Expected<br>Connects</th>"
      "<th>Subresource Spec</th></tr>");

  for (Referrers::iterator it = referrers_.begin(); referrers_.end() != it;
       ++it) {
    const Referrer& referrer = it->second;
    bool first_set_of_futures = true;
    for (Referrer::const_iterator future_url = referrer.begin();
         future_url != referrer.end(); ++future_url) {
      output->append("<tr align=right>");
      if (first_set_of_futures) {
        base::StringAppendF(
            output, "<td rowspan=%d>%s</td><td rowspan=%d>%d</td>",
            static_cast<int>(referrer.size()), it->first.spec().c_str(),
            static_cast<int>(referrer.size()),
            static_cast<int>(referrer.use_count()));
      }
      first_set_of_futures = false;
      base::StringAppendF(output,
          "<td>%d</td><td>%d</td><td>%d</td><td>%2.3f</td><td>%s</td></tr>",
          static_cast<int>(future_url->second.navigation_count()),
          static_cast<int>(future_url->second.preconnection_count()),
          static_cast<int>(future_url->second.preresolution_count()),
          static_cast<double>(future_url->second.subresource_use_rate()),
          future_url->first.spec().c_str());
    }
  }
  output->append("</table>");
}

void Predictor::GetHtmlInfo(std::string* output) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (kInitialDnsPrefetchListEnabled && initial_observer_)
    initial_observer_->GetFirstResolutionsHtml(output);
  // Show list of subresource predictions and stats.
  GetHtmlReferrerLists(output);

  // Local lists for calling UrlInfo
  UrlInfo::UrlInfoTable name_not_found;
  UrlInfo::UrlInfoTable name_preresolved;

  // UrlInfo supports value semantics, so we can do a shallow copy.
  for (Results::iterator it(results_.begin()); it != results_.end(); it++) {
    if (it->second.was_nonexistent()) {
      name_not_found.push_back(it->second);
      continue;
    }
    if (!it->second.was_found())
      continue;  // Still being processed.
    name_preresolved.push_back(it->second);
  }

  bool brief = false;
#ifdef NDEBUG
  brief = true;
#endif  // NDEBUG

  // Call for display of each table, along with title.
  UrlInfo::GetHtmlTable(name_preresolved,
      "Preresolution DNS records performed for ", brief, output);
  UrlInfo::GetHtmlTable(name_not_found,
      "Preresolving DNS records revealed non-existence for ", brief, output);
}

// Iterating through a MRUCache goes through most recent first. Iterate
// backwards here so that adding items in order "Just Works" when deserializing.
void Predictor::SerializeReferrers(base::ListValue* referral_list) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(referral_list->empty());
  referral_list->AppendInteger(kPredictorReferrerVersion);
  for (Referrers::const_reverse_iterator it = referrers_.rbegin();
       it != referrers_.rend(); ++it) {
    // Create a list for each referer.
    std::unique_ptr<base::ListValue> motivator(new base::ListValue);
    motivator->AppendString(it->first.spec());
    motivator->Append(it->second.Serialize());

    referral_list->Append(std::move(motivator));
  }
}

void Predictor::DeserializeReferrers(const base::ListValue& referral_list) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int format_version = -1;
  if (referral_list.GetSize() > 0 &&
      referral_list.GetInteger(0, &format_version) &&
      format_version == kPredictorReferrerVersion) {
    for (size_t i = 1; i < referral_list.GetSize(); ++i) {
      const base::ListValue* motivator;
      if (!referral_list.GetList(i, &motivator)) {
        NOTREACHED();
        return;
      }
      std::string motivating_url_spec;
      if (!motivator->GetString(0, &motivating_url_spec)) {
        NOTREACHED();
        return;
      }

      const base::Value* subresource_list;
      if (!motivator->Get(1, &subresource_list)) {
        NOTREACHED();
        return;
      }

      referrers_.Put(GURL(motivating_url_spec), Referrer())
          ->second.Deserialize(*subresource_list);
    }
  }
}

void Predictor::DiscardInitialNavigationHistory() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (kInitialDnsPrefetchListEnabled && initial_observer_)
    initial_observer_->DiscardInitialNavigationHistory();
}

void Predictor::FinalizeInitializationOnIOThread(
    const std::vector<GURL>& startup_urls,
    std::unique_ptr<base::ListValue> referral_list,
    IOThread* io_thread,
    ProfileIOData* profile_io_data) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  TRACE_EVENT0("net", "Predictor::FinalizeInitializationOnIOThread");

  profile_io_data_ = profile_io_data;
  if (kInitialDnsPrefetchListEnabled)
    initial_observer_ = std::make_unique<InitialObserver>();

  net::URLRequestContext* context =
      url_request_context_getter_->GetURLRequestContext();
  transport_security_state_ = context->transport_security_state();
  ssl_config_service_ = context->ssl_config_service();
  proxy_resolution_service_ = context->proxy_resolution_service();

  // base::WeakPtrFactory instances need to be created and destroyed
  // on the same thread. Initialize the IO thread weak factory now.
  io_weak_factory_.reset(new base::WeakPtrFactory<Predictor>(this));

  // Prefetch these hostnames on startup.
  if (kInitialDnsPrefetchListEnabled)
    DnsPrefetchMotivatedList(startup_urls, UrlInfo::STARTUP_LIST_MOTIVATED);

  DeserializeReferrers(*referral_list);
}

//-----------------------------------------------------------------------------
// This section intermingles prefetch results with actual browser HTTP
// network activity.  It supports calculating of the benefit of a prefetch, as
// well as recording what prefetched hostname resolutions might be potentially
// helpful during the next chrome-startup.
//-----------------------------------------------------------------------------

void Predictor::LearnAboutInitialNavigation(const GURL& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!PredictorEnabled() || !kInitialDnsPrefetchListEnabled ||
      !initial_observer_ || !CanPreresolveAndPreconnect()) {
    return;
  }
  initial_observer_->Append(url, this);
}

// This API is only used in the browser process.
// It is called from an IPC message originating in the renderer.  It currently
// includes both Page-Scan, and Link-Hover prefetching.
// TODO(jar): Separate out link-hover prefetching, and page-scan results.
void Predictor::DnsPrefetchList(const std::vector<std::string>& hostnames) {
  // TODO(jar): Push GURL transport further back into renderer, but this will
  // require a Webkit change in the observer :-/.
  std::vector<GURL> urls;
  for (std::vector<std::string>::const_iterator it = hostnames.begin();
       it < hostnames.end(); ++it) {
    urls.push_back(GURL("http://" + *it + ":80"));
  }

  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DnsPrefetchMotivatedList(urls, UrlInfo::PAGE_SCAN_MOTIVATED);
}

void Predictor::DnsPrefetchMotivatedList(
    const std::vector<GURL>& urls,
    UrlInfo::ResolutionMotivation motivation) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
         BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!PredictorEnabled())
    return;
  if (!CanPreresolveAndPreconnect())
    return;

  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    ResolveList(urls, motivation);
  } else {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&Predictor::ResolveList, base::Unretained(this), urls,
                       motivation));
  }
}

//-----------------------------------------------------------------------------
// Functions to handle saving of hostnames from one session to the next, to
// expedite startup times.

void Predictor::SaveStateForNextStartup() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!PredictorEnabled())
    return;
  if (!CanPreresolveAndPreconnect())
    return;

  auto startup_list = std::make_unique<base::ListValue>();
  auto referral_list = std::make_unique<base::ListValue>();

  // Get raw pointers to pass to the first task. Ownership of the unique_ptrs
  // will be passed to the reply task.
  base::ListValue* startup_list_raw = startup_list.get();
  base::ListValue* referral_list_raw = referral_list.get();

  // The first post task here is guaranteed to execute before the post task in
  // ShutdownOnUIThread, because the caller has a valid profile.
  BrowserThread::PostTaskAndReply(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&Predictor::WriteDnsPrefetchState, base::Unretained(this),
                     startup_list_raw, referral_list_raw),
      base::BindOnce(&Predictor::UpdatePrefsOnUIThread,
                     ui_weak_factory_->GetWeakPtr(), std::move(startup_list),
                     std::move(referral_list)));
}

void Predictor::UpdatePrefsOnUIThread(
    std::unique_ptr<base::ListValue> startup_list,
    std::unique_ptr<base::ListValue> referral_list) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  user_prefs_->Set(prefs::kDnsPrefetchingStartupList, *startup_list);

  // May be empty if kInitialDnsPrefetchListEnabled is false. Still update the
  // prefs to clear the state.
  user_prefs_->Set(prefs::kDnsPrefetchingHostReferralList, *referral_list);
}

void Predictor::WriteDnsPrefetchState(base::ListValue* startup_list,
                                      base::ListValue* referral_list) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (kInitialDnsPrefetchListEnabled && initial_observer_)
    initial_observer_->GetInitialDnsResolutionList(startup_list);

  SerializeReferrers(referral_list);
}

void Predictor::PreconnectUrl(const GURL& url,
                              const GURL& site_for_cookies,
                              UrlInfo::ResolutionMotivation motivation,
                              bool allow_credentials,
                              int count) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
         BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(url.is_valid());

  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    PreconnectUrlOnIOThread(url, site_for_cookies, motivation,
                            allow_credentials, count);
  } else {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&Predictor::PreconnectUrlOnIOThread,
                       base::Unretained(this), url, site_for_cookies,
                       motivation, allow_credentials, count));
  }
}

void Predictor::PreconnectUrlOnIOThread(
    const GURL& original_url,
    const GURL& site_for_cookies,
    UrlInfo::ResolutionMotivation motivation,
    bool allow_credentials,
    int count) {
  // Skip the HSTS redirect.
  GURL url = GetHSTSRedirectOnIOThread(original_url);

  // TODO(csharrison): The observer should only be notified after the null check
  // for the URLRequestContextGetter. The predictor tests should be fixed to
  // allow for this, as they currently expect a callback with no getter.
  if (observer_) {
    observer_->OnPreconnectUrl(url, site_for_cookies, motivation, count);
  }

  net::URLRequestContextGetter* getter = url_request_context_getter_.get();
  if (!getter)
    return;

  content::PreconnectUrl(getter, url, site_for_cookies, count,
                         allow_credentials);
}

void Predictor::PredictFrameSubresources(const GURL& url,
                                         const GURL& site_for_cookies) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
         BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!PredictorEnabled())
    return;
  if (!CanPreresolveAndPreconnect())
    return;
  DCHECK_EQ(url.GetWithEmptyPath(), url);
  // Add one pass through the message loop to allow current navigation to
  // proceed.
  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    PrepareFrameSubresources(url, site_for_cookies);
  } else {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&Predictor::PrepareFrameSubresources,
                       base::Unretained(this), url, site_for_cookies));
  }
}

bool Predictor::CanPrefetchAndPrerender() const {
  chrome_browser_net::NetworkPredictionStatus status;
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    status = chrome_browser_net::CanPrefetchAndPrerenderUI(user_prefs_);
  } else {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    status = chrome_browser_net::CanPrefetchAndPrerenderIO(profile_io_data_);
  }
  return status == chrome_browser_net::NetworkPredictionStatus::ENABLED;
}

bool Predictor::CanPreresolveAndPreconnect() const {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    return chrome_browser_net::CanPreresolveAndPreconnectUI(user_prefs_);
  } else {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    return chrome_browser_net::CanPreresolveAndPreconnectIO(profile_io_data_);
  }
}

enum SubresourceValue {
  PRECONNECTION,
  PRERESOLUTION,
  TOO_NEW,
  SUBRESOURCE_VALUE_MAX
};

void Predictor::PrepareFrameSubresources(const GURL& original_url,
                                         const GURL& site_for_cookies) {
  // Apply HSTS redirect early so it is taken into account when looking up
  // subresources.
  GURL url = GetHSTSRedirectOnIOThread(original_url);

  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK_EQ(url.GetWithEmptyPath(), url);
  DCHECK(PredictorEnabled());
  // Peek here, and Get after logging the index into the MRU.
  Referrers::iterator it = referrers_.Get(url);
  if (referrers_.end() == it) {
    // Only when we don't know anything about this url, make 2 connections
    // available.  We could do this completely via learning (by prepopulating
    // the referrer_ list with this expected value), but it would swell the
    // size of the list with all the "Leaf" nodes in the tree (nodes that don't
    // load any subresources).  If we learn about this resource, we will instead
    // provide a more carefully estimated preconnection count.
    PreconnectUrlOnIOThread(url, site_for_cookies,
                            UrlInfo::SELF_REFERAL_MOTIVATED,
                            kAllowCredentialsOnPreconnectByDefault, 2);
    return;
  }
  Referrer* referrer = &(it->second);

  referrer->IncrementUseCount();
  const UrlInfo::ResolutionMotivation motivation =
      UrlInfo::LEARNED_REFERAL_MOTIVATED;
  for (std::map<GURL, ReferrerValue>::iterator future_url = referrer->begin();
       future_url != referrer->end();) {
    double connection_expectation = future_url->second.subresource_use_rate();
    future_url->second.ReferrerWasObserved();
    if (connection_expectation > kPreconnectWorthyExpectedValue) {
      future_url->second.IncrementPreconnectionCount();
      int count = static_cast<int>(std::ceil(connection_expectation));
      if (url.host_piece() == future_url->first.host_piece())
        ++count;
      PreconnectUrlOnIOThread(future_url->first, site_for_cookies, motivation,
                              kAllowCredentialsOnPreconnectByDefault, count);
    } else if (connection_expectation > kDNSPreresolutionWorthyExpectedValue) {
      future_url->second.preresolution_increment();
      AppendToResolutionQueue(future_url->first, motivation);
    }
    // Remove future urls that are below the discardable threshold here. This is
    // the only place where the future urls of a referrer are iterated through,
    // so it is the most logical place for trimming.
    if (connection_expectation < kDiscardableExpectedValue) {
      future_url = referrer->erase(future_url);
    } else {
      ++future_url;
    }
  }
  // If the Referrer has no URLs associated with it, remove it from the map.
  if (referrer->empty())
    referrers_.Erase(it);
}

void Predictor::OnLookupFinished(const GURL& url, int result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  LookupFinished(url, result == net::OK);
  if (observer_)
    observer_->OnDnsLookupFinished(url, result == net::OK);
  DCHECK_GT(num_pending_lookups_, 0u);
  num_pending_lookups_--;
  StartSomeQueuedResolutions();
}

void Predictor::LookupFinished(const GURL& url, bool found) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  auto info_it = results_.find(url);
  UrlInfo* info = &info_it->second;
  DCHECK(info->HasUrl(url));
  bool is_marked_to_delete = info->is_marked_to_delete();

  if (found)
    info->SetFoundState();
  else
    info->SetNoSuchNameState();

  if (is_marked_to_delete)
    results_.erase(info_it);
}

bool Predictor::WouldLikelyProxyURL(const GURL& url) {
  if (!proxy_resolution_service_)
    return false;

  net::ProxyInfo info;
  bool synchronous_success = proxy_resolution_service_->TryResolveProxySynchronously(
      url, std::string(), &info, nullptr, net::NetLogWithSource());

  return synchronous_success && !info.is_direct();
}

void Predictor::AppendToResolutionQueue(
    const GURL& url,
    UrlInfo::ResolutionMotivation motivation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(url.has_host());

  if (shutdown_)
    return;

  UrlInfo* info = &results_[url];
  info->SetUrl(url);  // Initialize or DCHECK.
  DCHECK(info->HasUrl(url));

  if (!info->NeedsDnsUpdate()) {
    info->DLogResultsStats("DNS PrefetchNotUpdated");
    return;
  }

  if (WouldLikelyProxyURL(url)) {
    info->DLogResultsStats("DNS PrefetchForProxiedRequest");
    return;
  }

  info->SetQueuedState(motivation);
  work_queue_.Push(url, motivation);

  StartSomeQueuedResolutions();
}

bool Predictor::CongestionControlPerformed(Results::iterator info_it) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  UrlInfo* info = &info_it->second;
  // Note: queue_duration is ONLY valid after we go to assigned state.
  if (info->queue_duration() < max_dns_queue_delay_)
    return false;
  // We need to discard all entries in our queue, as we're keeping them waiting
  // too long.  By doing this, we'll have a chance to quickly service urgent
  // resolutions, and not have a bogged down system.
  while (true) {
    info->RemoveFromQueue();
    results_.erase(info_it);
    if (work_queue_.IsEmpty())
      break;
    info_it = results_.find(work_queue_.Pop());
    info = &info_it->second;
    info->SetAssignedState();
  }
  return true;
}

void Predictor::StartSomeQueuedResolutions() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  while (!work_queue_.IsEmpty() &&
         num_pending_lookups_ < max_concurrent_dns_lookups_) {
    const GURL url(work_queue_.Pop());
    auto info_it = results_.find(url);
    UrlInfo* info = &info_it->second;
    DCHECK(info->HasUrl(url));
    info->SetAssignedState();

    if (CongestionControlPerformed(info_it)) {
      DCHECK(work_queue_.IsEmpty());
      return;
    }

    info->SetPendingDeleteState();
    int status =
        content::PreresolveUrl(url_request_context_getter_.get(), url,
                               base::Bind(&Predictor::OnLookupFinished,
                                          io_weak_factory_->GetWeakPtr(), url));
    if (status == net::ERR_IO_PENDING) {
      // Will complete asynchronously.
      num_pending_lookups_++;
      peak_pending_lookups_ =
          std::max(peak_pending_lookups_, num_pending_lookups_);
    } else {
      // Completed synchronously (was already cached by HostResolver), or else
      // there was (equivalently) some network error that prevents us from
      // finding the name.  Status net::OK means it was "found."
      LookupFinished(url, status == net::OK);
    }
  }
}

GURL Predictor::GetHSTSRedirectOnIOThread(const GURL& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!transport_security_state_)
    return url;
  if (!url.SchemeIs("http"))
    return url;
  if (!transport_security_state_->ShouldUpgradeToSSL(url.host()))
    return url;

  url::Replacements<char> replacements;
  const char kNewScheme[] = "https";
  replacements.SetScheme(kNewScheme, url::Component(0, strlen(kNewScheme)));
  return url.ReplaceComponents(replacements);
}

// ---------------------- End IO methods. -------------------------------------

//-----------------------------------------------------------------------------

bool Predictor::PredictorEnabled() const {
  base::AutoLock lock(predictor_enabled_lock_);
  return predictor_enabled_;
}

void Predictor::SetPredictorEnabledForTest(bool predictor_enabled) {
  base::AutoLock lock(predictor_enabled_lock_);
  predictor_enabled_ = predictor_enabled;
}

Predictor::HostNameQueue::HostNameQueue() {
}

Predictor::HostNameQueue::~HostNameQueue() {
}

void Predictor::HostNameQueue::Push(const GURL& url,
    UrlInfo::ResolutionMotivation motivation) {
  switch (motivation) {
    case UrlInfo::STATIC_REFERAL_MOTIVATED:
    case UrlInfo::LEARNED_REFERAL_MOTIVATED:
    case UrlInfo::MOUSE_OVER_MOTIVATED:
      rush_queue_.push(url);
      break;

    default:
      background_queue_.push(url);
      break;
  }
}

bool Predictor::HostNameQueue::IsEmpty() const {
  return rush_queue_.empty() && background_queue_.empty();
}

GURL Predictor::HostNameQueue::Pop() {
  DCHECK(!IsEmpty());
  base::queue<GURL>* queue(rush_queue_.empty() ? &background_queue_
                                               : &rush_queue_);
  GURL url(queue->front());
  queue->pop();
  return url;
}

//-----------------------------------------------------------------------------
// Member definitions for InitialObserver class.

InitialObserver::InitialObserver() = default;

InitialObserver::~InitialObserver() = default;

void InitialObserver::Append(const GURL& url, Predictor* predictor) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (kStartupResolutionCount <= first_navigations_.size())
    return;

  DCHECK(url.SchemeIsHTTPOrHTTPS());
  DCHECK_EQ(url, Predictor::CanonicalizeUrl(url));
  if (first_navigations_.find(url) == first_navigations_.end())
    first_navigations_[url] = base::TimeTicks::Now();
}

void InitialObserver::GetInitialDnsResolutionList(
    base::ListValue* startup_list) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(startup_list);
  DCHECK(startup_list->empty());
  DCHECK_EQ(0u, startup_list->GetSize());
  startup_list->AppendInteger(kPredictorStartupFormatVersion);
  for (const auto& url_time : first_navigations_) {
    DCHECK(url_time.first == Predictor::CanonicalizeUrl(url_time.first));
    startup_list->AppendString(url_time.first.spec());
  }
}

void InitialObserver::GetFirstResolutionsHtml(std::string* output) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  UrlInfo::UrlInfoTable resolution_list;
  for (const auto& url_time : first_navigations_) {
    UrlInfo info;
    info.SetUrl(url_time.first);
    info.set_time(url_time.second);
    resolution_list.push_back(info);
  }
  UrlInfo::GetHtmlTable(resolution_list,
                        "Future startups will prefetch DNS records for ", false,
                        output);
}

//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------

// static
GURL Predictor::CanonicalizeUrl(const GURL& url) {
  if (!url.has_host())
     return GURL::EmptyGURL();

  std::string scheme;
  if (url.has_scheme()) {
    scheme = url.scheme();
    if (scheme != "http" && scheme != "https")
      return GURL::EmptyGURL();
    if (url.has_port())
      return url.GetWithEmptyPath();
  } else {
    scheme = "http";
  }

  // If we omit a port, it will default to 80 or 443 as appropriate.
  std::string colon_plus_port;
  if (url.has_port())
    colon_plus_port = ":" + url.port();

  return GURL(scheme + "://" + url.host() + colon_plus_port);
}

void SimplePredictor::InitNetworkPredictor(
    PrefService* user_prefs,
    IOThread* io_thread,
    net::URLRequestContextGetter* getter,
    ProfileIOData* profile_io_data) {
  // Empty function for unittests.
}

void SimplePredictor::ShutdownOnUIThread() {
  SetShutdown(true);
}

bool SimplePredictor::CanPrefetchAndPrerender() const { return true; }
bool SimplePredictor::CanPreresolveAndPreconnect() const { return true; }

}  // namespace chrome_browser_net
