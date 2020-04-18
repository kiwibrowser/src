// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NET_REQUEST_TRACKER_IMPL_H_
#define IOS_WEB_NET_REQUEST_TRACKER_IMPL_H_

#import <Foundation/Foundation.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#import "ios/net/request_tracker.h"
#include "ios/web/public/web_thread.h"
#include "net/cert/cert_status_flags.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

@class SSLCarrier;
struct TrackerCounts;

namespace net {
class URLRequest;
class URLRequestContext;
class SSLInfo;
}

namespace web {

class BrowserState;
class CertificatePolicyCache;

// Structure to capture the current state of a page.
struct PageCounts {
 public:
  PageCounts() : finished(0),
                 finished_bytes(0),
                 unfinished(0),
                 unfinished_no_estimate(0),
                 unfinished_no_estimate_bytes_done(0),
                 unfinished_estimated_bytes_left(0),
                 unfinished_estimate_bytes_done(0),
                 largest_byte_size_known(0) {
  };

  // Count of finished requests.
  uint64_t finished;
  // Total bytes count dowloaded for all finished requests.
  uint64_t finished_bytes;
  // Count  of unfinished requests.
  uint64_t unfinished;
  // Count of unfinished requests with unknown size.
  uint64_t unfinished_no_estimate;
  // Total bytes count dowloaded for unfinished requests of unknown size.
  uint64_t unfinished_no_estimate_bytes_done;
  // Count of unfinished requests with an estimated size.
  uint64_t unfinished_estimated_bytes_left;
  // Total bytes count dowloaded for unfinished requests with an estimated size.
  uint64_t unfinished_estimate_bytes_done;
  // Size of the request with the most bytes on the page.
  uint64_t largest_byte_size_known;
};

// RequestTrackerImpl captures and stores all the network requests that
// initiated from a particular tab. It only keeps the URLs and eventually, if
// available, the expected length of the result and the length of the received
// data so far as this is used to build a progress bar for a page.
// Note that the Request tracker has no notion of a page, it only tracks the
// requests by tab. In order for the tracker to know that a request is for a
// page or a subresource it is necessary for the tab to call StartPageLoad()
// with the URL of the page once it is known to avoid storing all the requests
// forever.
//
// The consumer needs to implement the CRWRequestTrackerImplDelegate protocol
// and needs to call StartPageLoad() and FinishPageLoad() to indicate the page
// boundaries. StartPageLoad() will also have the side effect of clearing past
// requests from memory. The consumer is assumed to be on the UI thread at all
// times.
//
// RequestTrackerImpl objects are created and destroyed on the UI thread and
// must be owned by some other object on the UI thread by way of a
// scoped_refptr, as returned by the public static constructor method,
// CreateTrackerForRequestGroupID. All consumer API methods will be called
// through this pointer.

class RequestTrackerImpl;

struct RequestTrackerImplTraits {
  static void Destruct(const RequestTrackerImpl* t);
};

class RequestTrackerImpl
    : public base::RefCountedThreadSafe<RequestTrackerImpl,
                                        RequestTrackerImplTraits>,
      public net::RequestTracker {
 public:
#pragma mark Public Consumer API
  // Consumer API methods should only be called on the UI thread.

  // Create a new RequestTrackerImpl associated with a particular tab. The
  // profile must be the one associated to the given tab. This method has to be
  // called *once* per tab and needs to be called before triggering any network
  // request. The caller of CreateTrackerForRequestGroupID owns the tracker, and
  // this class also keeps a global map of all active trackers. When the owning
  // object releases it, the class removes it from the global map.
  static scoped_refptr<RequestTrackerImpl> CreateTrackerForRequestGroupID(
      NSString* request_group_id,
      BrowserState* browser_state,
      net::URLRequestContextGetter* context_getter);

  // The network layer has no way to know which network request is the primary
  // one for a page load. The tab knows, either because it initiated the page
  // load via the URL or received a callback informing it of the page change.
  // Every time this happens the tab should call this method to clear the
  // resources tracked.
  // This will forget all the finished requests made before this URL in history.
  // user_info is to be used by the consumer to store more additional specific
  // info about the page, as an URL is not unique.
  void StartPageLoad(const GURL& url, id user_info);

  // In order to properly provide progress information the tracker needs to know
  // when the page is fully loaded. |load_success| indicates if the page
  // successfully loaded.
  void FinishPageLoad(const GURL& url, bool load_success);

  // Tells the tracker that history.pushState() or history.replaceState()
  // changed the page URL.
  void HistoryStateChange(const GURL& url);

  // Marks the tracker as closed. An owner must call this before the tracker is
  // deleted. Once closed, no further calls will be made to the delegate.
  void Close();

  // Call |callback| on the UI thread after any pending request cancellations
  // have completed on the IO thread.
  // This should be used to delete a profile for which all of the trackers
  // that use the profile's request context are closed.
  static void RunAfterRequestsCancel(const base::Closure& callback);

  // Block until all pending IO thread activity has completed. This should only
  // be used when Chrome is shutting down, and after all request trackers have
  // had Close() called on them.
  static void BlockUntilTrackersShutdown();

#pragma mark Client utility methods.

  // Finds the tracker given the tab ID. As calling this method involves a lock
  // it is expected that the provider will call it only once.
  // Returns a weak pointer, which should only be dereferenced on the IO thread.
  // Returns NULL if no tracker exists for |request_group_id|.
  static RequestTrackerImpl* GetTrackerForRequestGroupID(
      NSString* request_group_id);

  // Utility method for clients to post tasks to the IO thread from the UI
  // thread.
  void PostIOTask(const base::Closure& task);

  // Utility method for clients to post tasks to the IO thread from the IO
  // thread.
  void ScheduleIOTask(const base::Closure& task);

  // Utility method for clients to conditionally post tasks to the UI thread
  // from the IO thread. The task will not be posted if the request tracker
  // is in the process of closing (thus it "is open").
  void PostUITaskIfOpen(const base::Closure& task);
  // Static version of the method, where |tracker| is a RequestTrackerImpl
  // passed as a base::WeakPtr<RequestTracker>.
  static void PostUITaskIfOpen(const base::WeakPtr<RequestTracker> tracker,
                               const base::Closure& task);

#pragma mark Testing methods

  void SetCertificatePolicyCacheForTest(web::CertificatePolicyCache* cache);

#pragma mark Accessors used by internal classes and network clients.
  int identifier() { return identifier_; }
  bool has_mixed_content() { return has_mixed_content_; }

  // RequestTracker implementation.
  void StartRequest(net::URLRequest* request) override;
  void CaptureExpectedLength(const net::URLRequest* request,
                             uint64_t length) override;
  void CaptureReceivedBytes(const net::URLRequest* request,
                            uint64_t byte_count) override;
  void CaptureCertificatePolicyCache(
      const net::URLRequest* request,
      const SSLCallback& should_continue) override;
  void StopRequest(net::URLRequest* request) override;
  void StopRedirectedRequest(net::URLRequest* request) override;
  void OnSSLCertificateError(const net::URLRequest* request,
                             const net::SSLInfo& ssl_info,
                             bool recoverable,
                             const SSLCallback& should_continue) override;
  net::URLRequestContext* GetRequestContext() override;

 private:
  friend class base::RefCountedThreadSafe<RequestTrackerImpl>;
  friend struct RequestTrackerImplTraits;

#pragma mark Object lifecycle API
  // Private. RequestTrackerImpls are created through
  // CreateTrackerForRequestGroupID().
  RequestTrackerImpl(NSString* request_group_id,
                     net::URLRequestContextGetter* context_getter);

  void InitOnIOThread(
      const scoped_refptr<web::CertificatePolicyCache>& policy_cache);

  // Private destructor because the object is reference counted. A no-op; the
  // useful destruction work happens in Destruct().
  ~RequestTrackerImpl() override;

  // Handles pre-destruction destruction tasks. This is invoked by
  // RequestTrackerImplTraits::Destruct whenever the reference count of a
  // RequestTrackerImpl is zero, and this will untimately delete the
  // RequestTrackerImpl.
  void Destruct();

#pragma mark Private Provider API
  // Private methods that implement provider API features. All are only called
  // on the IO thread.

  // Called when something has changed (network load progress or SSL status)
  // that the consumer should know about. Notifications are asynchronous and
  // batched.
  void Notify();

  // If no other notifications are pending, notifies the consumer of SSL status
  // and load progress.
  void StackNotification();

  // If the counts is for a request currently waiting for the user to approve it
  // will reevaluate the approval.
  void EvaluateSSLCallbackForCounts(TrackerCounts* counts);

  // Loop through all the requests waiting for approval and invoke
  // |-evaluateSSLCallbackForCounts:| on all the ones with an |UNKNOWN|
  // judgment.
  void ReevaluateCallbacksForAllCounts();

  // To cancel a rejected request due to a SSL issue.
  void CancelRequestForCounts(TrackerCounts* counts);

  // Estimate the page load progress. Returns -1 if the progress didn't change
  // since the last time this method was invoked.
  float EstimatedProgress();

  // The URL change notification is often late, therefore the mixed content
  // status and the certificate policies may need to be recomputed.
  void RecomputeMixedContent(const TrackerCounts* split_position);

  // Remove all finished request up to the last instance of |url|. If url is not
  // found, this will clear all the requests.
  void TrimToURL(const GURL& url, id user_info);

  // Sets page_url_ to the new URL if it's a valid history state change (i.e.
  // the URL's have the same origin) and if the tab is currently loading.
  void HistoryStateChangeToURL(const GURL& full_url);

  // Note that the page started by a call to Trim is no longer loading.
  // |load_success| indicates if the page successfully loaded.
  void StopPageLoad(const GURL& url, bool load_success);

#pragma mark Internal utilities for task posting
  // Posts |task| to |thread|. Must not be called from |thread|. If |thread| is
  // the IO thread, silently returns if |is_closing_| is true.
  void PostTask(const base::Closure& task, web::WebThread::ID thread);

  // Posts |block| to |thread|, safely passing in |caller| to |block|.
  void PostBlock(id caller, void (^block)(id), web::WebThread::ID thread);

#pragma mark Other internal methods.
  // Returns the current state of the page.
  PageCounts pageCounts();

  // Like description, but cannot be called from any thread. It must be called
  // only from the IO thread.
  NSString* UnsafeDescription();

#pragma mark Non thread-safe fields, only accessed from the IO thread.
  // All the tracked requests for the page, indexed by net::URLRequest (Cast as
  // a void* to avoid the temptation of accessing it from the wrong thread).
  // This map is not exhaustive: it is only meant to estimate the loading
  // progress, and thus requests corresponding to old navigation events are not
  // in it.
  std::map<const void*, TrackerCounts*> counts_by_request_;
  // A list of all the TrackerCounts, including the finished ones.
  std::vector<std::unique_ptr<TrackerCounts>> counts_;
  // The system shall never allow the page load estimate to go back.
  float previous_estimate_;
  // Index of the first request to consider for building the estimation.
  unsigned int estimate_start_index_;
  // How many notifications are currently queued, to avoid notifying too often.
  int notification_depth_;
  // Set to |YES| if the page has mixed content
  bool has_mixed_content_;
  // Set to true if between TrimToURL and StopPageLoad.
  bool is_loading_;
  // Set to true in TrimToURL if starting a new estimate round. Set to false by
  // StartRequest once the new round is started.
  bool new_estimate_round_;

#pragma mark Other fields.
  scoped_refptr<web::CertificatePolicyCache> policy_cache_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  // Current page URL, as far as we know.
  GURL page_url_;
  // Userinfo attached to the page, passed back by the delegate.
  id user_info_;
  // A tracker identifier (a simple increasing number) used to store
  // certificates.
  int identifier_;
  // The string that identifies the tab this tracker serves. Used to index
  // g_trackers.
  NSString* request_group_id_;
  // Flag to synchronize deletion and callback creation. Lives on the IO thread.
  // True when this tracker has beed Close()d. If this is the case, no further
  // references to it should be generated (for example by binding it into a
  // callback), and the expectation is that it will soon be deleted.
  bool is_closing_;
};

}  // namespace web

#endif  // IOS_WEB_NET_REQUEST_TRACKER_IMPL_H_
