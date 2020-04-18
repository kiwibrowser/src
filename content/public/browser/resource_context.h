// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_RESOURCE_CONTEXT_H_
#define CONTENT_PUBLIC_BROWSER_RESOURCE_CONTEXT_H_

#include "base/supports_user_data.h"
#include "content/common/content_export.h"

namespace net {
class HostResolver;
class URLRequestContext;
}

namespace content {

// ResourceContext contains the relevant context information required for
// resource loading. It lives on the IO thread, although it is constructed on
// the UI thread. It must be destructed on the IO thread.
// TODO(mmenke):  Get rid of this class.
class CONTENT_EXPORT ResourceContext : public base::SupportsUserData {
 public:
  ResourceContext();
  ~ResourceContext() override;
  virtual net::HostResolver* GetHostResolver() = 0;

  // DEPRECATED: This is no longer a valid given isolated apps/sites and
  // storage partitioning. This getter returns the default context associated
  // with a BrowsingContext.
  virtual net::URLRequestContext* GetRequestContext() = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_RESOURCE_CONTEXT_H_
