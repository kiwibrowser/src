// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_HSTS_QUERY_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_HSTS_QUERY_H_

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_request_context_getter.h"

class GURL;

namespace password_manager {

using HSTSCallback = base::Callback<void(bool)>;

// Checks asynchronously whether HTTP Strict Transport Security (HSTS) is active
// for the host of the given origin for a given request context.  Notifies
// |callback| with the result on the calling thread.
void PostHSTSQueryForHostAndRequestContext(
    const GURL& origin,
    const scoped_refptr<net::URLRequestContextGetter>& request_context,
    const HSTSCallback& callback);

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_HSTS_QUERY_H_
