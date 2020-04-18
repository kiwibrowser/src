// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/net/request_tracker_impl.h"

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/macros.h"
#include "base/strings/sys_string_conversions.h"
#include "base/synchronization/lock.h"
#include "ios/web/history_state_util.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/certificate_policy_cache.h"
#include "ios/web/public/url_util.h"
#include "ios/web/public/web_thread.h"
#import "net/base/mac/url_conversions.h"
#include "net/url_request/url_request.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {


// A map of all RequestTrackerImpls for tabs that are:
// * Currently open
// * Recently closed waiting for all their network operations to finish.
// The code accesses this variable from two threads: the consumer is expected to
// always access it from the main thread, the provider is accessing it from the
// WebThread, a thread created by the UIWebView/CFURL. For this reason access to
// this variable must always gated by |g_trackers_lock|.
typedef base::hash_map<std::string, web::RequestTrackerImpl*> TrackerMap;

TrackerMap* g_trackers = NULL;
base::Lock* g_trackers_lock = NULL;
pthread_once_t g_once_control = PTHREAD_ONCE_INIT;

// Flag, lock, and function to implement BlockUntilTrackersShutdown().
// |g_waiting_on_io_thread| is guarded by |g_waiting_on_io_thread_lock|;
// it is set to true when the shutdown wait starts, then a call to
// StopIOThreadWaiting is posted to the IO thread (enqueued after any pending
// request terminations) while the posting method loops over a check on the
// |g_waiting_on_io_thread|.
static bool g_waiting_on_io_thread = false;
base::Lock* g_waiting_on_io_thread_lock = NULL;
void StopIOThreadWaiting() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  base::AutoLock scoped_lock(*g_waiting_on_io_thread_lock);
  g_waiting_on_io_thread = false;
}

// Initialize global state. Calls to this should be conditional on
// |g_once_control| (that is, this should only be called once, across all
// threads).
void InitializeGlobals() {
  g_trackers = new TrackerMap;
  g_trackers_lock = new base::Lock;
  g_waiting_on_io_thread_lock = new base::Lock;
}

// Each request tracker get a unique increasing number, used anywhere an
// identifier is needed for tracker (e.g. storing certs).
int g_next_request_tracker_id = 0;

// Add |tracker| to |g_trackers| under |key|.
static void RegisterTracker(web::RequestTrackerImpl* tracker, NSString* key) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  pthread_once(&g_once_control, &InitializeGlobals);
  {
    std::string scoped_key = base::SysNSStringToUTF8(key);
    base::AutoLock scoped_lock(*g_trackers_lock);
    DCHECK(!g_trackers->count(scoped_key));
    (*g_trackers)[scoped_key] = tracker;
  }
}

}  // namespace

// The structure used to gather the information about the resources loaded.
struct TrackerCounts {
 public:
  TrackerCounts(const GURL& tracked_url, const net::URLRequest* tracked_request)
      : url(tracked_url),
        site_for_cookies_origin(
            tracked_request->site_for_cookies().GetOrigin()),
        request(tracked_request),
        ssl_info(net::SSLInfo()),
        ssl_judgment(web::CertPolicy::ALLOWED),
        allowed_by_user(false),
        expected_length(0),
        processed(0),
        done(false) {
    DCHECK_CURRENTLY_ON(web::WebThread::IO);
    is_subrequest =
        tracked_request->site_for_cookies().is_valid() &&
        tracked_request->url() != tracked_request->site_for_cookies();
  };

  // The resource url.
  const GURL url;
  // The origin of the url of the top level document of the resource. This is
  // used to ignore request coming from an old document when detecting mixed
  // content.
  const GURL site_for_cookies_origin;
  // The request associated with this struct. As a void* to prevent access from
  // the wrong thread.
  const void* request;
  // SSLInfo for the request.
  net::SSLInfo ssl_info;
  // Is the SSL request blocked waiting for user choice.
  web::CertPolicy::Judgment ssl_judgment;
  // True if |ssl_judgment| is ALLOWED as the result of a user choice.
  bool allowed_by_user;
  // block to call to cancel or authorize a blocked request.
  net::RequestTracker::SSLCallback ssl_callback;
  // If known, the expected length of the resource in bytes.
  uint64_t expected_length;
  // Number of bytes loaded so far.
  uint64_t processed;
  // Set to true is the resource is fully loaded.
  bool done;
  // Set to true if the request has a main request set.
  bool is_subrequest;

  NSString* Description() {
    NSString* spec = base::SysUTF8ToNSString(url.spec());
    NSString* status = nil;
    if (done) {
      status = [NSString stringWithFormat:@"\t-- Done -- (%04qu) bytes",
          processed];
    } else if (!expected_length) {
      status = [NSString stringWithFormat:@"\t>> Loading (%04qu) bytes",
          processed];
    } else {
      status = [NSString stringWithFormat:@"\t>> Loading (%04qu/%04qu)",
          processed, expected_length];
    }

    NSString* ssl = @"";
    if (ssl_info.is_valid()) {
      NSString* subject = base::SysUTF8ToNSString(
          ssl_info.cert.get()->subject().GetDisplayName());
      NSString* issuer = base::SysUTF8ToNSString(
          ssl_info.cert.get()->issuer().GetDisplayName());

      ssl = [NSString stringWithFormat:
          @"\n\t\tcert for '%@' issued by '%@'", subject, issuer];

      if (!net::IsCertStatusMinorError(ssl_info.cert_status)) {
        ssl = [NSString stringWithFormat:@"%@ (status: %0xd)",
            ssl, ssl_info.cert_status];
      }
    }
    return [NSString stringWithFormat:@"%@\n\t\t%@%@", status, spec, ssl];
  }

  DISALLOW_COPY_AND_ASSIGN(TrackerCounts);
};

namespace web {

#pragma mark Consumer API

// static
scoped_refptr<RequestTrackerImpl>
RequestTrackerImpl::CreateTrackerForRequestGroupID(
    NSString* request_group_id,
    BrowserState* browser_state,
    net::URLRequestContextGetter* context_getter) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(request_group_id);

  scoped_refptr<RequestTrackerImpl> tracker =
      new RequestTrackerImpl(request_group_id, context_getter);

  scoped_refptr<CertificatePolicyCache> policy_cache =
      BrowserState::GetCertificatePolicyCache(browser_state);
  DCHECK(policy_cache);

  // Take care of the IO-thread init.
  web::WebThread::PostTask(
      web::WebThread::IO, FROM_HERE,
      base::Bind(&RequestTrackerImpl::InitOnIOThread, tracker, policy_cache));
  RegisterTracker(tracker.get(), request_group_id);
  return tracker;
}

void RequestTrackerImpl::StartPageLoad(const GURL& url, id user_info) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  id scoped_user_info = user_info;
  web::WebThread::PostTask(
      web::WebThread::IO, FROM_HERE,
      base::Bind(&RequestTrackerImpl::TrimToURL, this, url, scoped_user_info));
}

void RequestTrackerImpl::FinishPageLoad(const GURL& url, bool load_success) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  web::WebThread::PostTask(
      web::WebThread::IO, FROM_HERE,
      base::Bind(&RequestTrackerImpl::StopPageLoad, this, url, load_success));
}

void RequestTrackerImpl::HistoryStateChange(const GURL& url) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  web::WebThread::PostTask(
      web::WebThread::IO, FROM_HERE,
      base::Bind(&RequestTrackerImpl::HistoryStateChangeToURL, this, url));
}

// Close is called when an owning object (a Tab or something that acts like
// it) is done with the RequestTrackerImpl. There may still be queued calls on
// the UI thread that will make use of the fields being cleaned-up here; they
// must ensure they they operate without crashing with the cleaned-up values.
void RequestTrackerImpl::Close() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  // Mark the tracker as closing on the IO thread. Note that because the local
  // scoped_refptr here retains |this|, we a are guaranteed that destruiction
  // won't begin until the block completes, and thus |is_closing_| will always
  // be set before destruction begins.
  web::WebThread::PostTask(web::WebThread::IO, FROM_HERE,
                           base::Bind(
                               [](RequestTrackerImpl* tracker) {
                                 tracker->is_closing_ = true;
                               },
                               base::RetainedRef(this)));

  // The user_info is no longer needed.
  user_info_ = nil;
}

// static
void RequestTrackerImpl::RunAfterRequestsCancel(const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  // Post a no-op to the IO thread, and after that has executed, run |callback|.
  // This ensures that |callback| runs after anything elese queued on the IO
  // thread, in particular CancelRequest() calls made from closing trackers.
  web::WebThread::PostTaskAndReply(web::WebThread::IO, FROM_HERE,
                                   base::DoNothing(), callback);
}

// static
void RequestTrackerImpl::BlockUntilTrackersShutdown() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  // Initialize the globals as part of the shutdown to prevent a crash when
  // trying to acquire the lock if it was never initialised. It can happen
  // if the application is terminated when no RequestTracker has been created
  // (which can happen if no UIWebView has been created). See crbug.com/684611
  // for information on such a crash.
  //
  // As RequestTracker are deprecated and are only used in Sign-In workflow
  // it is simpler to just do the initialisation here than tracking whether
  // the method should be called or not by client code.
  pthread_once(&g_once_control, &InitializeGlobals);
  {
    base::AutoLock scoped_lock(*g_waiting_on_io_thread_lock);
    g_waiting_on_io_thread = true;
  }
  web::WebThread::PostTask(web::WebThread::IO, FROM_HERE,
                           base::Bind(&StopIOThreadWaiting));

  // Poll endlessly until the wait flag is unset on the IO thread by
  // StopIOThreadWaiting().
  // (Consider instead having a hard time cap, like 100ms or so, after which
  // we stop blocking. In that case this method would return a boolean
  // indicating if the wait completed or not).
  while (1) {
    base::AutoLock scoped_lock(*g_waiting_on_io_thread_lock);
    if (!g_waiting_on_io_thread)
      return;
    // Ensure that other threads have a chance to run even on a single-core
    // devices.
    pthread_yield_np();
  }
}

#pragma mark Provider API

// static
RequestTrackerImpl* RequestTrackerImpl::GetTrackerForRequestGroupID(
    NSString* request_group_id) {
  DCHECK(request_group_id);
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  RequestTrackerImpl* tracker = nullptr;
  TrackerMap::iterator map_it;
  pthread_once(&g_once_control, &InitializeGlobals);
  {
    base::AutoLock scoped_lock(*g_trackers_lock);
    map_it = g_trackers->find(base::SysNSStringToUTF8(request_group_id));
    if (map_it != g_trackers->end())
      tracker = map_it->second;
  }
  return tracker;
}

net::URLRequestContext* RequestTrackerImpl::GetRequestContext() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  return request_context_getter_->GetURLRequestContext();
}

void RequestTrackerImpl::StartRequest(net::URLRequest* request) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  DCHECK(!counts_by_request_.count(request));
  DCHECK(!request->url().SchemeIsFile());

  if (new_estimate_round_) {
    // Starting a new estimate round. Ignore the previous requests for the
    // calculation.
    counts_by_request_.clear();
    estimate_start_index_ = counts_.size();
    new_estimate_round_ = false;
  }
  const GURL& url = request->original_url();
  auto counts =
      std::make_unique<TrackerCounts>(GURLByRemovingRefFromGURL(url), request);
  counts_by_request_[request] = counts.get();
  counts_.push_back(std::move(counts));
  if (page_url_.SchemeIsCryptographic() && !url.SchemeIsCryptographic())
    has_mixed_content_ = true;
  Notify();
}

void RequestTrackerImpl::CaptureExpectedLength(const net::URLRequest* request,
                                               uint64_t length) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  if (counts_by_request_.count(request)) {
    TrackerCounts* counts = counts_by_request_[request];
    DCHECK(!counts->done);
    if (length < counts->processed) {
      // Something is wrong with the estimate. Ignore it.
      counts->expected_length = 0;
    } else {
      counts->expected_length = length;
    }
    Notify();
  }
}

void RequestTrackerImpl::CaptureReceivedBytes(const net::URLRequest* request,
                                              uint64_t byte_count) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  if (counts_by_request_.count(request)) {
    TrackerCounts* counts = counts_by_request_[request];
    DCHECK(!counts->done);
    const net::SSLInfo& ssl_info = request->ssl_info();
    if (ssl_info.is_valid())
      counts->ssl_info = ssl_info;
    counts->processed += byte_count;
    if (counts->expected_length > 0 &&
        counts->expected_length < counts->processed) {
      // Something is wrong with the estimate, it is too low. Ignore it.
      counts->expected_length = 0;
    }
    Notify();
  }
}

void RequestTrackerImpl::StopRequest(net::URLRequest* request) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);

  if (counts_by_request_.count(request)) {
    StopRedirectedRequest(request);
    Notify();
  }
}

void RequestTrackerImpl::StopRedirectedRequest(net::URLRequest* request) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);

  if (counts_by_request_.count(request)) {
    TrackerCounts* counts = counts_by_request_[request];
    DCHECK(!counts->done);
    const net::SSLInfo& ssl_info = request->ssl_info();
    if (ssl_info.is_valid())
      counts->ssl_info = ssl_info;
    counts->done = true;
    counts_by_request_.erase(request);
  }
}

void RequestTrackerImpl::CaptureCertificatePolicyCache(
    const net::URLRequest* request,
    const RequestTracker::SSLCallback& should_continue) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  std::string host = request->url().host();
  CertPolicy::Judgment judgment = policy_cache_->QueryPolicy(
      request->ssl_info().cert.get(), host, request->ssl_info().cert_status);
  if (judgment == CertPolicy::UNKNOWN) {
    // The request comes from the cache, and has been loaded even though the
    // policy is UNKNOWN. Display the interstitial page now.
    OnSSLCertificateError(request, request->ssl_info(), true, should_continue);
    return;
  }

  // Notify the  delegate that a judgment has been used.
  DCHECK(judgment == CertPolicy::ALLOWED);
  should_continue.Run(true);
}

void RequestTrackerImpl::OnSSLCertificateError(
    const net::URLRequest* request,
    const net::SSLInfo& ssl_info,
    bool recoverable,
    const RequestTracker::SSLCallback& should_continue) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  DCHECK(ssl_info.is_valid());

  if (counts_by_request_.count(request)) {
    TrackerCounts* counts = counts_by_request_[request];

    DCHECK(!counts->done);
    // Store the ssl error.
    counts->ssl_info = ssl_info;
    counts->ssl_callback = should_continue;
    counts->ssl_judgment =
        recoverable ? CertPolicy::UNKNOWN : CertPolicy::DENIED;
    ReevaluateCallbacksForAllCounts();
  }
}

#pragma mark Client utility methods.

void RequestTrackerImpl::PostUITaskIfOpen(const base::Closure& task) {
  PostTask(task, web::WebThread::UI);
}

// static
void RequestTrackerImpl::PostUITaskIfOpen(
    const base::WeakPtr<RequestTracker> tracker,
    const base::Closure& task) {
  if (!tracker)
    return;
  RequestTrackerImpl* tracker_impl =
      static_cast<RequestTrackerImpl*>(tracker.get());
  tracker_impl->PostUITaskIfOpen(task);
}

void RequestTrackerImpl::PostIOTask(const base::Closure& task) {
  PostTask(task, web::WebThread::IO);
}

void RequestTrackerImpl::ScheduleIOTask(const base::Closure& task) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  web::WebThread::PostTask(web::WebThread::IO, FROM_HERE, task);
}

#pragma mark Private Object Lifecycle API

RequestTrackerImpl::RequestTrackerImpl(
    NSString* request_group_id,
    net::URLRequestContextGetter* context_getter)
    : previous_estimate_(0.0f),  // Not active by default.
      estimate_start_index_(0),
      notification_depth_(0),
      has_mixed_content_(false),
      is_loading_(false),
      new_estimate_round_(true),
      request_context_getter_(context_getter),
      identifier_(++g_next_request_tracker_id),
      request_group_id_([request_group_id copy]),
      is_closing_(false) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
}

void RequestTrackerImpl::InitOnIOThread(
    const scoped_refptr<CertificatePolicyCache>& policy_cache) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  Init();
  DCHECK(policy_cache);
  policy_cache_ = policy_cache;
}

RequestTrackerImpl::~RequestTrackerImpl() {
}

void RequestTrackerImplTraits::Destruct(const RequestTrackerImpl* t) {
  // RefCountedThreadSafe assumes we can do all the destruct tasks with a
  // const pointer, but we actually can't.
  RequestTrackerImpl* inconstant_t = const_cast<RequestTrackerImpl*>(t);
  if (web::WebThread::CurrentlyOn(web::WebThread::IO)) {
    inconstant_t->Destruct();
  } else {
    // Use BindBlock rather than Bind to avoid creating another scoped_refpter
    // to |this|. |inconstant_t| isn't retained by the block, but since this
    // method is the mechanism by which all RequestTrackerImpl instances are
    // destroyed, the object inconstant_t points to won't be deleted while
    // the block is executing (and Destruct() itself will do the deleting).
    web::WebThread::PostTask(web::WebThread::IO, FROM_HERE,
                             base::BindBlockArc(^{
                               inconstant_t->Destruct();
                             }));
  }
}

void RequestTrackerImpl::Destruct() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  DCHECK(is_closing_);

  pthread_once(&g_once_control, &InitializeGlobals);
  {
    base::AutoLock scoped_lock(*g_trackers_lock);
    g_trackers->erase(base::SysNSStringToUTF8(request_group_id_));
  }
  InvalidateWeakPtrs();
  // Delete on the UI thread.
  web::WebThread::PostTask(web::WebThread::UI, FROM_HERE, base::BindBlockArc(^{
                             delete this;
                           }));
}

#pragma mark Other private methods

void RequestTrackerImpl::Notify() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  if (is_closing_)
    return;
  // Notify() is called asynchronously, it runs later on the same
  // thread. This is used to collate notifications together, avoiding
  // blanketing the UI with a stream of information.
  notification_depth_ += 1;
  web::WebThread::PostTask(
      web::WebThread::IO, FROM_HERE,
      base::Bind(&RequestTrackerImpl::StackNotification, this));
}

void RequestTrackerImpl::StackNotification() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  if (is_closing_)
    return;

  // There is no point in sending the notification if there is another one
  // already queued. This queue is processing very lightweight changes and
  // should be exhausted very easily.
  --notification_depth_;
  if (notification_depth_)
    return;
}

void RequestTrackerImpl::EvaluateSSLCallbackForCounts(TrackerCounts* counts) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  DCHECK(policy_cache_);

  // Ignore non-SSL requests.
  if (!counts->ssl_info.is_valid())
    return;

  CertPolicy::Judgment judgment =
      policy_cache_->QueryPolicy(counts->ssl_info.cert.get(),
                                 counts->url.host(),
                                 counts->ssl_info.cert_status);

  if (judgment != CertPolicy::ALLOWED) {
    // Apply some fine tuning.
    // TODO(droger): This logic is duplicated from SSLPolicy. Sharing the code
    // would be better.
    switch (net::MapCertStatusToNetError(counts->ssl_info.cert_status)) {
      case net::ERR_CERT_NO_REVOCATION_MECHANISM:
        // Ignore this error.
        judgment = CertPolicy::ALLOWED;
        break;
      case net::ERR_CERT_UNABLE_TO_CHECK_REVOCATION:
        // We ignore this error but it will show a warning status in the
        // location bar.
        judgment = CertPolicy::ALLOWED;
        break;
      case net::ERR_CERT_CONTAINS_ERRORS:
      case net::ERR_CERT_REVOKED:
      case net::ERR_CERT_INVALID:
      case net::ERR_SSL_WEAK_SERVER_EPHEMERAL_DH_KEY:
      case net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN:
        judgment = CertPolicy::DENIED;
        break;
      case net::ERR_CERT_COMMON_NAME_INVALID:
      case net::ERR_CERT_DATE_INVALID:
      case net::ERR_CERT_AUTHORITY_INVALID:
      case net::ERR_CERT_WEAK_SIGNATURE_ALGORITHM:
      case net::ERR_CERT_WEAK_KEY:
      case net::ERR_CERT_NAME_CONSTRAINT_VIOLATION:
      case net::ERR_CERT_VALIDITY_TOO_LONG:
        // Nothing. If DENIED it will stay denied. If UNKNOWN it will be
        // shown to the user for decision.
        break;
      default:
        NOTREACHED();
        judgment = CertPolicy::DENIED;
        break;
    }
  }

  counts->ssl_judgment = judgment;

  switch (judgment) {
    case CertPolicy::UNKNOWN:
    case CertPolicy::DENIED:
      // Simply cancel the request.
      CancelRequestForCounts(counts);
      break;
    case CertPolicy::ALLOWED:
      counts->ssl_callback.Run(YES);
      counts->ssl_callback = base::DoNothing();
      break;
    default:
      NOTREACHED();
      // For now simply cancel the request.
      CancelRequestForCounts(counts);
      break;
  }
}

void RequestTrackerImpl::ReevaluateCallbacksForAllCounts() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  if (is_closing_)
    return;

  for (const auto& tracker_count : counts_) {
    // Check if the value hasn't changed via a user action.
    if (tracker_count->ssl_judgment == CertPolicy::UNKNOWN)
      EvaluateSSLCallbackForCounts(tracker_count.get());
  }
}

void RequestTrackerImpl::CancelRequestForCounts(TrackerCounts* counts) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  // Cancel the request.
  counts->done = true;
  counts_by_request_.erase(counts->request);
  counts->ssl_callback.Run(NO);
  counts->ssl_callback = base::DoNothing();
  Notify();
}

PageCounts RequestTrackerImpl::pageCounts() {
  DCHECK_GE(counts_.size(), estimate_start_index_);

  PageCounts page_counts;

  for (const auto& tracker_count : counts_) {
    if (tracker_count->done) {
      uint64_t size = tracker_count->processed;
      page_counts.finished += 1;
      page_counts.finished_bytes += size;
      if (page_counts.largest_byte_size_known < size) {
        page_counts.largest_byte_size_known = size;
      }
    } else {
      page_counts.unfinished += 1;
      if (tracker_count->expected_length) {
        uint64_t size = tracker_count->expected_length;
        page_counts.unfinished_estimate_bytes_done += tracker_count->processed;
        page_counts.unfinished_estimated_bytes_left += size;
        if (page_counts.largest_byte_size_known < size) {
          page_counts.largest_byte_size_known = size;
        }
      } else {
        page_counts.unfinished_no_estimate += 1;
        page_counts.unfinished_no_estimate_bytes_done +=
            tracker_count->processed;
      }
    }
  }

  return page_counts;
}

float RequestTrackerImpl::EstimatedProgress() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);

  const PageCounts page_counts = pageCounts();

  // Nothing in progress and the last time was the same.
  if (!page_counts.unfinished && previous_estimate_ == 0.0f)
    return -1.0f;

  // First request.
  if (previous_estimate_ == 0.0f) {
    // start low.
    previous_estimate_ = 0.1f;
    return previous_estimate_;  // Return the just started status.
  }

  // The very simple case where everything is probably done and dusted.
  if (!page_counts.unfinished) {
    // Add 60%, and return. Another task is going to finish this.
    float bump = (1.0f - previous_estimate_) * 0.6f;
    previous_estimate_ += bump;
    return previous_estimate_;
  }

  // Calculate some ratios.
  // First the ratio of the finished vs the unfinished counts of resources
  // loaded.
  float unfinishedRatio =
      static_cast<float>(page_counts.finished) /
      static_cast<float>(page_counts.unfinished + page_counts.finished);

  // The ratio of bytes left vs bytes already downloaded for the resources where
  // no estimates of final size are known. For this ratio it is assumed the size
  // of a resource not downloaded yet is the maximum size of all the resources
  // seen so far.
  float noEstimateRatio = (!page_counts.unfinished_no_estimate_bytes_done) ?
      0.0f :
      static_cast<float>(page_counts.unfinished_no_estimate *
                         page_counts.largest_byte_size_known) /
          static_cast<float>(page_counts.finished_bytes +
                             page_counts.unfinished_no_estimate_bytes_done);

  // The ratio of bytes left vs bytes already downloaded for the resources with
  // available estimated size.
  float estimateRatio = (!page_counts.unfinished_estimated_bytes_left) ?
      noEstimateRatio :
      static_cast<float>(page_counts.unfinished_estimate_bytes_done) /
  static_cast<float>(page_counts.unfinished_estimate_bytes_done +
                     page_counts.unfinished_estimated_bytes_left);

  // Reassemble all of this.
  float total =
      0.1f +  // Minimum value.
      unfinishedRatio * 0.6f +
      estimateRatio * 0.3f;

  if (previous_estimate_ >= total)
    return -1.0f;

  // 10% of what's left.
  float maxBump = (1.0f - previous_estimate_) / 10.0f;
  // total is greater than previous estimate, need to bump the estimate up.
  if ((previous_estimate_ + maxBump) > total) {
    // Less than a 10% bump, bump to the new value.
    previous_estimate_ = total;
  } else {
    // Just bump by 10% toward the total.
    previous_estimate_ += maxBump;
  }

  return previous_estimate_;
}

void RequestTrackerImpl::RecomputeMixedContent(
    const TrackerCounts* split_position) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  // Check if the mixed content before trimming was correct.
  if (page_url_.SchemeIsCryptographic() && has_mixed_content_) {
    bool old_url_has_mixed_content = false;
    const GURL origin = page_url_.GetOrigin();
    auto it = counts_.begin();
    while (it != counts_.end() && it->get() != split_position) {
      if (!(*it)->url.SchemeIsCryptographic() &&
          origin == (*it)->site_for_cookies_origin) {
        old_url_has_mixed_content = true;
        break;
      }
      ++it;
    }
    if (!old_url_has_mixed_content) {
      // We marked the previous page with incorrect data about its mixed
      // content. Turns out that the elements that triggered that condition
      // where in fact in a subsequent page. Duh.
      // Resend a notification for the |page_url_| informing the upper layer
      // that the mixed content was a red herring.
      has_mixed_content_ = false;
    }
  }
}

void RequestTrackerImpl::HistoryStateChangeToURL(const GURL& full_url) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  GURL url = GURLByRemovingRefFromGURL(full_url);

  if (is_loading_ &&
      web::history_state_util::IsHistoryStateChangeValid(url, page_url_)) {
    page_url_ = url;
  }
}

void RequestTrackerImpl::TrimToURL(const GURL& full_url, id user_info) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);

  GURL url = GURLByRemovingRefFromGURL(full_url);

  // Locate the request with this url, if present.
  bool new_url_has_mixed_content = false;
  bool url_scheme_is_secure = url.SchemeIsCryptographic();
  auto rit = counts_.rbegin();
  while (rit != counts_.rend() && (*rit)->url != url) {
    if (url_scheme_is_secure && !(*rit)->url.SchemeIsCryptographic() &&
        (*rit)->site_for_cookies_origin == url.GetOrigin()) {
      new_url_has_mixed_content = true;
    }
    ++rit;
  }

  // |split_position| will be set to the count for the passed url if it exists.
  TrackerCounts* split_position = NULL;
  if (rit != counts_.rend()) {
    split_position = rit->get();
  } else {
    // The URL was not found, everything will be trimmed. The mixed content
    // calculation is invalid.
    new_url_has_mixed_content = false;

    // In the case of a page loaded via a HTML5 manifest there is no page
    // boundary to be found. However the latest count is a request for a
    // manifest. This tries to detect this peculiar case.
    // This is important as if this request for the manifest is on the same
    // domain as the page itself this will allow retrieval of the SSL
    // information.
    if (url_scheme_is_secure && counts_.size()) {
      TrackerCounts* back = counts_.back().get();
      const GURL& back_url = back->url;
      if (back_url.SchemeIsCryptographic() &&
          back_url.GetOrigin() == url.GetOrigin() && !back->is_subrequest) {
        split_position = back;
      }
    }
  }
  RecomputeMixedContent(split_position);

  // Trim up to that element.
  auto it = counts_.begin();
  while (it != counts_.end() && it->get() != split_position) {
    if (!(*it)->done) {
      // This is for an unfinished request on a previous page. We do not care
      // about those anymore. Cancel the request.
      if ((*it)->ssl_judgment == CertPolicy::UNKNOWN)
        CancelRequestForCounts(it->get());
      counts_by_request_.erase((*it)->request);
    }
    it = counts_.erase(it);
  }

  has_mixed_content_ = new_url_has_mixed_content;
  page_url_ = url;
  user_info_ = user_info;
  estimate_start_index_ = 0;
  is_loading_ = true;
  previous_estimate_ = 0.0f;
  new_estimate_round_ = true;
  ReevaluateCallbacksForAllCounts();
  Notify();
}

void RequestTrackerImpl::StopPageLoad(const GURL& url, bool load_success) {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);
  DCHECK(page_url_ == GURLByRemovingRefFromGURL(url));
  is_loading_ = false;
}

#pragma mark Internal utilities for task posting

void RequestTrackerImpl::PostTask(const base::Closure& task,
                                  web::WebThread::ID thread) {
  // Absolute sanity test: |thread| is one of {UI, IO}
  DCHECK(thread == web::WebThread::UI || thread == web::WebThread::IO);
  // Check that we're on the counterpart thread to the one we're posting to.
  DCHECK_CURRENTLY_ON(thread == web::WebThread::IO ? web::WebThread::UI
                                                   : web::WebThread::IO);
  // Don't post if the tracker is closing and we're on the IO thread.
  // (there should be no way to call anything from the UI thread if
  // the tracker is closing).
  if (is_closing_ && web::WebThread::CurrentlyOn(web::WebThread::IO))
    return;
  web::WebThread::PostTask(thread, FROM_HERE, task);
}

#pragma mark Other internal methods.

NSString* RequestTrackerImpl::UnsafeDescription() {
  DCHECK_CURRENTLY_ON(web::WebThread::IO);

  NSMutableArray* urls = [NSMutableArray array];
  for (const auto& tracker_count : counts_)
    [urls addObject:tracker_count->Description()];

  return [NSString stringWithFormat:@"RequestGroupID %@\n%@\n%@",
                                    request_group_id_,
                                    net::NSURLWithGURL(page_url_),
                                    [urls componentsJoinedByString:@"\n"]];
}

void RequestTrackerImpl::SetCertificatePolicyCacheForTest(
    web::CertificatePolicyCache* cache) {
  policy_cache_ = cache;
}

}  // namespace web
