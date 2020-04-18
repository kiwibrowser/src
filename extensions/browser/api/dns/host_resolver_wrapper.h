// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DNS_HOST_RESOLVER_WRAPPER_H_
#define EXTENSIONS_BROWSER_API_DNS_HOST_RESOLVER_WRAPPER_H_

#include "base/macros.h"
#include "base/memory/singleton.h"

namespace content {
class ResourceContext;
}

namespace net {
class HostResolver;
}

namespace extensions {

// Used for testing. In production code, this class does nothing interesting.
// This class is a singleton that holds a pointer to a mock HostResolver, or
// else to NULL. API classes that need to resolve hostnames ask this class for
// the correct HostResolver to use, passing in the one that they want to use,
// thereby avoiding most lifetime issues, and it will reply with either that
// same one, or else the test version to use instead.
//
// This is a pretty complicated way to replace a single pointer with another.
// TODO(miket): make the previous statement obsolete.
class HostResolverWrapper {
 public:
  static HostResolverWrapper* GetInstance();

  // Given a pointer to a ResourceContext, returns its HostResolver if
  // SetHostResolverForTesting() hasn't been called, or else a
  // a substitute MockHostResolver to use instead.
  net::HostResolver* GetHostResolver(content::ResourceContext* context);

  // Sets the MockHostResolver to return in GetHostResolver().
  void SetHostResolverForTesting(net::HostResolver* mock_resolver);

 private:
  HostResolverWrapper();
  friend struct base::DefaultSingletonTraits<HostResolverWrapper>;

  net::HostResolver* resolver_;

  DISALLOW_COPY_AND_ASSIGN(HostResolverWrapper);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DNS_HOST_RESOLVER_WRAPPER_H_
