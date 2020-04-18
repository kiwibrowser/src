// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CONTENT_PROTOCOL_HANDLER_IMPL_H_
#define CONTENT_BROWSER_ANDROID_CONTENT_PROTOCOL_HANDLER_IMPL_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/android/content_protocol_handler.h"

class GURL;

namespace base {
class TaskRunner;
}

namespace net {
class NetworkDelegate;
class URLRequestJob;
}

namespace content {

// Implements a ProtocolHandler for content scheme jobs. If |network_delegate_|
// is NULL, then all file requests will fail with ERR_ACCESS_DENIED.
class ContentProtocolHandlerImpl : public ContentProtocolHandler {
 public:
  explicit ContentProtocolHandlerImpl(
      const scoped_refptr<base::TaskRunner>& content_task_runner);
  ~ContentProtocolHandlerImpl() override;
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;
  bool IsSafeRedirectTarget(const GURL& location) const override;

 private:
  const scoped_refptr<base::TaskRunner> content_task_runner_;
  DISALLOW_COPY_AND_ASSIGN(ContentProtocolHandlerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_CONTENT_PROTOCOL_HANDLER_IMPL_H_
