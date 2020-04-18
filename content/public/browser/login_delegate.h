// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_LOGIN_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_LOGIN_DELEGATE_H_

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"

namespace content {

// Interface for getting login credentials for HTTP auth requests. If the
// login delegate obtains credentials, it should call the URLRequest's SetAuth
// method. If the user cancels, the login delegate should call the URLRequest's
// CancelAuth instead. And in either case, it must make a call to
// ResourceDispatcherHost::ClearLoginDelegateForRequest.
class CONTENT_EXPORT LoginDelegate
    : public base::RefCountedThreadSafe<LoginDelegate> {
 public:
  // Notify the login delegate that the request was cancelled.
  // This function can only be called from the IO thread.
  virtual void OnRequestCancelled() = 0;

 protected:
  friend class base::RefCountedThreadSafe<LoginDelegate>;
  virtual ~LoginDelegate() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_LOGIN_DELEGATE_H_
