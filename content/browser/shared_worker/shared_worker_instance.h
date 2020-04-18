// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_INSTANCE_H_
#define CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_INSTANCE_H_

#include <string>

#include "content/common/content_export.h"
#include "third_party/blink/public/mojom/net/ip_address_space.mojom.h"
#include "third_party/blink/public/mojom/shared_worker/shared_worker_creation_context_type.mojom.h"
#include "third_party/blink/public/platform/web_content_security_policy.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

// SharedWorkerInstance is copyable value-type data type. It could be passed to
// the UI thread and be used for comparison in SharedWorkerDevToolsManager.
class CONTENT_EXPORT SharedWorkerInstance {
 public:
  SharedWorkerInstance(
      const GURL& url,
      const std::string& name,
      const url::Origin& constructor_origin,
      const std::string& content_security_policy,
      blink::WebContentSecurityPolicyType content_security_policy_type,
      blink::mojom::IPAddressSpace creation_address_space,
      blink::mojom::SharedWorkerCreationContextType creation_context_type);
  SharedWorkerInstance(const SharedWorkerInstance& other);
  ~SharedWorkerInstance();

  // Checks if this SharedWorkerInstance matches the passed url, name, and
  // constructor origin params according to the SharedWorker constructor steps
  // in the HTML spec:
  // https://html.spec.whatwg.org/multipage/workers.html#shared-workers-and-the-sharedworker-interface
  bool Matches(const GURL& url,
               const std::string& name,
               const url::Origin& constructor_origin) const;
  bool Matches(const SharedWorkerInstance& other) const;

  // Accessors.
  const GURL& url() const { return url_; }
  const std::string name() const { return name_; }
  const url::Origin& constructor_origin() const { return constructor_origin_; }
  const std::string content_security_policy() const {
    return content_security_policy_;
  }
  blink::WebContentSecurityPolicyType content_security_policy_type() const {
    return content_security_policy_type_;
  }
  blink::mojom::IPAddressSpace creation_address_space() const {
    return creation_address_space_;
  }
  blink::mojom::SharedWorkerCreationContextType creation_context_type() const {
    return creation_context_type_;
  }

 private:
  const GURL url_;
  const std::string name_;

  // The origin of the document that created this shared worker instance. Used
  // for security checks. See Matches() for details.
  // https://html.spec.whatwg.org/multipage/workers.html#concept-sharedworkerglobalscope-constructor-origin
  const url::Origin constructor_origin_;

  const std::string content_security_policy_;
  const blink::WebContentSecurityPolicyType content_security_policy_type_;
  const blink::mojom::IPAddressSpace creation_address_space_;
  const blink::mojom::SharedWorkerCreationContextType creation_context_type_;
};

}  // namespace content


#endif  // CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_INSTANCE_H_
