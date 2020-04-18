// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_RESOURCE_THROTTLE_H_
#define CONTENT_PUBLIC_BROWSER_RESOURCE_THROTTLE_H_

#include "content/common/content_export.h"

namespace net {
struct RedirectInfo;
}

namespace content {

class AsyncRevalidationDriver;
class ResourceController;
class ThrottlingResourceHandler;

// A ResourceThrottle gets notified at various points during the process of
// loading a resource.  At each stage, it has the opportunity to defer the
// resource load.  The ResourceController interface may be used to resume a
// deferred resource load, or it may be used to cancel a resource load at any
// time.
class CONTENT_EXPORT ResourceThrottle {
 public:
  // An interface to get notified when the throttle implementation considers
  // it's ready to resume the deferred resource load. The throttle
  // implementation may also tell the delegate to cancel the resource loading.
  class CONTENT_EXPORT Delegate {
   public:
    // Cancels the resource load.
    virtual void Cancel() = 0;
    // Cancels the resource load with the specified error code.
    virtual void CancelWithError(int error_code) = 0;
    // Tells the delegate to resume the deferred resource load.
    virtual void Resume() = 0;

   protected:
    virtual ~Delegate() {}
  };

  virtual ~ResourceThrottle() {}

  // Called before the resource request is started.
  virtual void WillStartRequest(bool* defer) {}

  // Called when the request was redirected.  |redirect_info| contains the
  // redirect responses's HTTP status code and some information about the new
  // request that will be sent if the redirect is followed, including the new
  // URL and new method.
  virtual void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                                   bool* defer) {}

  // Called when the response headers and meta data are available.
  virtual void WillProcessResponse(bool* defer) {}

  // Returns the name of the throttle, as a UTF-8 C-string, for logging
  // purposes.  nullptr is not allowed.  Caller does *not* take ownership of the
  // returned string.
  virtual const char* GetNameForLogging() const = 0;

  // Whether this ResourceThrottle needs to execute WillProcessResponse before
  // any part of the response body is read. Normally this is false. This should
  // be set to true if the ResourceThrottle wants to ensure that no part of the
  // response body will be cached if the request is canceled in
  // WillProcessResponse.
  virtual bool MustProcessResponseBeforeReadingBody();

  void set_delegate_for_testing(Delegate* delegate) { delegate_ = delegate; }

 protected:
  ResourceThrottle() : delegate_(nullptr) {}

  // Helper methods for subclasses. When these methods are called, methods with
  // the same name on |delegate_| are called.
  void Cancel();
  void CancelWithError(int error_code);
  void Resume();

 private:
  friend class AsyncRevalidationDriver;
  friend class ThrottlingResourceHandler;

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  Delegate* delegate_;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_RESOURCE_THROTTLE_H_
