// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_CONNECTOR_IMPL_H_
#define CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_CONNECTOR_IMPL_H_

#include "content/common/content_export.h"
#include "content/common/shared_worker/shared_worker_connector.mojom.h"

namespace content {

// Instances of this class live on the UI thread and have their lifetime bound
// to a Mojo connection.
class CONTENT_EXPORT SharedWorkerConnectorImpl
    : public mojom::SharedWorkerConnector {
 public:
  static void Create(int process_id,
                     int frame_id,
                     mojom::SharedWorkerConnectorRequest request);

 private:
  SharedWorkerConnectorImpl(int process_id, int frame_id);

  // mojom::SharedWorkerConnector methods:
  void Connect(
      mojom::SharedWorkerInfoPtr info,
      mojom::SharedWorkerClientPtr client,
      blink::mojom::SharedWorkerCreationContextType creation_context_type,
      mojo::ScopedMessagePipeHandle message_port) override;

  const int process_id_;
  const int frame_id_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SHARED_WORKER_SHARED_WORKER_CONNECTOR_IMPL_H_
