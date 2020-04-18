// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_instance.h"

#include "base/logging.h"

namespace content {

SharedWorkerInstance::SharedWorkerInstance(
    const GURL& url,
    const std::string& name,
    const url::Origin& constructor_origin,
    const std::string& content_security_policy,
    blink::WebContentSecurityPolicyType security_policy_type,
    blink::mojom::IPAddressSpace creation_address_space,
    blink::mojom::SharedWorkerCreationContextType creation_context_type)
    : url_(url),
      name_(name),
      constructor_origin_(constructor_origin),
      content_security_policy_(content_security_policy),
      content_security_policy_type_(security_policy_type),
      creation_address_space_(creation_address_space),
      creation_context_type_(creation_context_type) {}

SharedWorkerInstance::SharedWorkerInstance(const SharedWorkerInstance& other) =
    default;

SharedWorkerInstance::~SharedWorkerInstance() = default;

bool SharedWorkerInstance::Matches(
    const GURL& url,
    const std::string& name,
    const url::Origin& constructor_origin) const {
  // |url| and |constructor_origin| should be in the same origin, or |url|
  // should be a data: URL.
  DCHECK(url::Origin::Create(url).IsSameOriginWith(constructor_origin) ||
         url.SchemeIs(url::kDataScheme));

  // Step 11.2: "If there exists a SharedWorkerGlobalScope object whose closing
  // flag is false, constructor origin is same origin with outside settings's
  // origin, constructor url equals urlRecord, and name equals the value of
  // options's name member, then set worker global scope to that
  // SharedWorkerGlobalScope object."
  if (!constructor_origin_.IsSameOriginWith(constructor_origin) ||
      url_ != url || name_ != name)
    return false;

  // TODO(https://crbug.com/794098): file:// URLs should be treated as opaque
  // origins, but not in url::Origin. Therefore, we manually check it here.
  if (url.SchemeIsFile() || constructor_origin.scheme() == url::kFileScheme)
    return false;

  return true;
}

bool SharedWorkerInstance::Matches(const SharedWorkerInstance& other) const {
  return Matches(other.url(), other.name(), other.constructor_origin());
}

}  // namespace content
