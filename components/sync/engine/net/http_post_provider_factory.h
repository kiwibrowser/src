// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_NET_HTTP_POST_PROVIDER_FACTORY_H_
#define COMPONENTS_SYNC_ENGINE_NET_HTTP_POST_PROVIDER_FACTORY_H_

#include <string>

#include "base/callback.h"

namespace net {
class URLFetcher;
}

namespace syncer {

using BindToTrackerCallback = base::Callback<void(net::URLFetcher*)>;

class HttpPostProviderInterface;

// A factory to create HttpPostProviders to hide details about the
// implementations and dependencies.
// A factory instance itself should be owned by whomever uses it to create
// HttpPostProviders.
class HttpPostProviderFactory {
 public:
  virtual ~HttpPostProviderFactory() {}

  virtual void Init(const std::string& user_agent,
                    const BindToTrackerCallback& bind_to_tracker_callback) = 0;

  // Obtain a new HttpPostProviderInterface instance, owned by caller.
  virtual HttpPostProviderInterface* Create() = 0;

  // When the interface is no longer needed (ready to be cleaned up), clients
  // must call Destroy().
  // This allows actual HttpPostProvider subclass implementations to be
  // reference counted, which is useful if a particular implementation uses
  // multiple threads to serve network requests.
  virtual void Destroy(HttpPostProviderInterface* http) = 0;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_NET_HTTP_POST_PROVIDER_FACTORY_H_
