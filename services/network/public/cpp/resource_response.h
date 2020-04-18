// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See
// http://dev.chromium.org/developers/design-documents/multi-process-resource-loading

#ifndef SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_RESPONSE_H_
#define SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_RESPONSE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_request_status.h"
#include "services/network/public/cpp/resource_response_info.h"
#include "url/gurl.h"

namespace network {

// Parameters for a resource response header.
struct COMPONENT_EXPORT(NETWORK_CPP_BASE) ResourceResponseHead
    : ResourceResponseInfo {
  // TimeTicks::Now() when the browser received the request from the renderer.
  base::TimeTicks request_start;
  // TimeTicks::Now() when the browser sent the response to the renderer.
  base::TimeTicks response_start;
};

// Simple wrapper that refcounts ResourceResponseHead.
// Inherited, rather than typedef'd, to allow forward declarations.
struct COMPONENT_EXPORT(NETWORK_CPP_BASE) ResourceResponse
    : public base::RefCountedThreadSafe<ResourceResponse> {
 public:
  ResourceResponseHead head;

  // Performs a deep copy of the ResourceResponse and all fields in it, safe to
  // pass across threads.
  //
  // TODO(davidben): This structure should be passed along in a scoped_ptr. It's
  // currently reference-counted to avoid copies, but may be
  // modified. https://crbug.com/416050
  scoped_refptr<ResourceResponse> DeepCopy() const;

 private:
  friend class base::RefCountedThreadSafe<ResourceResponse>;
  ~ResourceResponse() {}
};

}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_RESPONSE_H_
