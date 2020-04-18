// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_RESOURCE_HINTS_H_
#define CONTENT_PUBLIC_BROWSER_RESOURCE_HINTS_H_

#include "content/common/content_export.h"
#include "net/base/completion_callback.h"
#include "net/http/http_request_info.h"
#include "url/gurl.h"

class GURL;

namespace net {
class URLRequestContextGetter;
}

namespace content {

// A Preconnect instance maintains state while a TCP/IP connection is made, and
// then released into the pool of available connections for future use.

// Tries to preconnect. |count| may be used to request more than one connection
// be established in parallel. Note that the net stack has logic so multiple
// calls to preconnect a given url within a short time frame will automatically
// coalesce. |allow_credentials| corresponds to the fetch spec.
// Note: This should only be called on the IO thread, with a valid
// URLRequestContextGetter.
CONTENT_EXPORT void PreconnectUrl(net::URLRequestContextGetter* getter,
                                  const GURL& url,
                                  const GURL& site_for_cookies,
                                  int count,
                                  bool allow_credentials);

// Issues a DNS request to |url|. Note that these requests are sent to the host
// resolver with priority net::IDLE.
CONTENT_EXPORT int PreresolveUrl(net::URLRequestContextGetter* getter,
                                 const GURL& url,
                                 const net::CompletionCallback& callback);

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_RESOURCE_HINTS_H_
