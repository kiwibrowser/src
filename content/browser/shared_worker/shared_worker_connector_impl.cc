// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_connector_impl.h"

#include "base/memory/ptr_util.h"
#include "content/browser/shared_worker/shared_worker_service_impl.h"
#include "content/browser/storage_partition_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/blink/public/common/message_port/message_port_channel.h"

namespace content {

// static
void SharedWorkerConnectorImpl::Create(
    int process_id,
    int frame_id,
    mojom::SharedWorkerConnectorRequest request) {
  mojo::MakeStrongBinding(
      base::WrapUnique(new SharedWorkerConnectorImpl(process_id, frame_id)),
      std::move(request));
}

SharedWorkerConnectorImpl::SharedWorkerConnectorImpl(int process_id,
                                                     int frame_id)
    : process_id_(process_id), frame_id_(frame_id) {}

void SharedWorkerConnectorImpl::Connect(
    mojom::SharedWorkerInfoPtr info,
    mojom::SharedWorkerClientPtr client,
    blink::mojom::SharedWorkerCreationContextType creation_context_type,
    mojo::ScopedMessagePipeHandle message_port) {
  RenderProcessHost* host = RenderProcessHost::FromID(process_id_);
  // The render process was already terminated.
  if (!host) {
    client->OnScriptLoadFailed();
    return;
  }
  SharedWorkerServiceImpl* service =
      static_cast<StoragePartitionImpl*>(host->GetStoragePartition())
          ->GetSharedWorkerService();
  service->ConnectToWorker(process_id_, frame_id_, std::move(info),
                           std::move(client), creation_context_type,
                           blink::MessagePortChannel(std::move(message_port)));
}

}  // namespace content
