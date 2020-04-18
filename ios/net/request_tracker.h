// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_REQUEST_TRACKER_H_
#define IOS_NET_REQUEST_TRACKER_H_

#import <Foundation/Foundation.h>
#include <stdint.h>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"

namespace net {
class SSLInfo;
class URLRequest;
class URLRequestContext;
}

namespace net {

// RequestTracker can be used to observe the network requests.
// Each network request can be associated with a RequestTracker through the
// GetRequestTracker().
// RequestTracker requires a RequestTrackerFactory.
// The RequestTracker can be created on one thread and used on a different one.
class RequestTracker {
 public:
  typedef base::Callback<void(bool)> SSLCallback;

  class RequestTrackerFactory {
   public:
    virtual ~RequestTrackerFactory();

    // Returns false if |request| is associated to an invalid tracker and should
    // be cancelled. In this case |tracker| is set to nullptr.
    // Returns true if |request| is associated with a valid tracker or if the
    // request is not associated to any tracker.
    virtual bool GetRequestTracker(NSURLRequest* request,
                                   base::WeakPtr<RequestTracker>* tracker) = 0;
  };

  // Sets the RequestTrackerFactory. The factory has to be set before the
  // GetRequestTracker() function can be called.
  // Does not take ownership of |factory|.
  static void SetRequestTrackerFactory(RequestTrackerFactory* factory);

  // Returns false if |request| is associated to an invalid tracker and should
  // be cancelled. In this case |tracker| is set to nullptr.
  // Returns true if |request| is associated with a valid tracker or if the
  // request is not associated to any tracker.
  // Internally calls the RequestTrackerFactory.
  static bool GetRequestTracker(NSURLRequest* request,
                                base::WeakPtr<RequestTracker>* tracker);

  RequestTracker();

  base::WeakPtr<RequestTracker> GetWeakPtr();

  // This function has to be called before using the tracker.
  virtual void Init();

  // Gets the request context associated with the tracker.
  virtual URLRequestContext* GetRequestContext() = 0;

  // Informs the tracker that a request has started.
  virtual void StartRequest(URLRequest* request) = 0;

  // Informs the tracker the expected length of the result, if known.
  virtual void CaptureExpectedLength(const URLRequest* request,
                                     uint64_t length) = 0;

  // Informs the tracker that a request received par_trackert of its data.
  virtual void CaptureReceivedBytes(const URLRequest* request,
                                    uint64_t byte_count) = 0;

  // Informs the tracker that a certificate has been used.
  virtual void CaptureCertificatePolicyCache(
      const URLRequest* request,
      const SSLCallback& should_continue) = 0;

  // Notifies of the completion of a request. Success or failure.
  virtual void StopRequest(URLRequest* request) = 0;

  // Special case for a redirect as we fully expect another request to follow
  // very shortly.
  virtual void StopRedirectedRequest(URLRequest* request) = 0;

  // Called when there is an issue on the SSL certificate. The user must be
  // informed and if |recoverable| is YES the user decision to continue or not
  // will be send back via the |callback|. The callback must be safe to call
  // from any thread. If recoverable is NO, invoking the callback should be a
  // noop.
  virtual void OnSSLCertificateError(const URLRequest* request,
                                     const SSLInfo& ssl_info,
                                     bool recoverable,
                                     const SSLCallback& should_continue) = 0;

 protected:
  virtual ~RequestTracker();

  void InvalidateWeakPtrs();

 private:
  bool initialized_;
  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<RequestTracker> weak_ptr_factory_;
};

}  // namespace net

#endif  // IOS_NET_REQUEST_TRACKER_H_
