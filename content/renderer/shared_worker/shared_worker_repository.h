// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SHARED_WORKER_SHARED_WORKER_REPOSITORY_H_
#define CONTENT_RENDERER_SHARED_WORKER_SHARED_WORKER_REPOSITORY_H_

#include <list>
#include <map>
#include <memory>

#include "base/macros.h"
#include "content/common/shared_worker/shared_worker_connector.mojom.h"
#include "content/renderer/shared_worker/shared_worker_client_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "third_party/blink/public/platform/web_content_security_policy.h"
#include "third_party/blink/public/web/web_shared_worker_repository_client.h"

namespace blink {
class MessagePortChannel;
}

namespace service_manager {
class InterfaceProvider;
}

namespace content {

class SharedWorkerRepository final
    : public blink::WebSharedWorkerRepositoryClient {
 public:
  explicit SharedWorkerRepository(
      service_manager::InterfaceProvider* interface_provider);
  ~SharedWorkerRepository();

  // WebSharedWorkerRepositoryClient overrides.
  void Connect(
      const blink::WebURL& url,
      const blink::WebString& name,
      DocumentID document_id,
      const blink::WebString& content_security_policy,
      blink::WebContentSecurityPolicyType,
      blink::mojom::IPAddressSpace,
      blink::mojom::SharedWorkerCreationContextType,
      blink::MessagePortChannel channel,
      std::unique_ptr<blink::WebSharedWorkerConnectListener> listener) override;
  void DocumentDetached(DocumentID document_id) override;

 private:
  void AddWorker(DocumentID document_id,
                 std::unique_ptr<mojom::SharedWorkerClient> impl,
                 mojom::SharedWorkerClientRequest request);

  service_manager::InterfaceProvider* interface_provider_;

  mojom::SharedWorkerConnectorPtr connector_;

  using ClientSet = mojo::StrongBindingSet<mojom::SharedWorkerClient>;
  using ClientMap = std::map<DocumentID, std::unique_ptr<ClientSet>>;
  ClientMap client_map_;

  DISALLOW_COPY_AND_ASSIGN(SharedWorkerRepository);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SHARED_WORKER_SHARED_WORKER_REPOSITORY_H_
