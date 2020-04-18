// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_CLIENT_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_CLIENT_H_

#include "base/memory/weak_ptr.h"
#include "components/services/heap_profiling/public/mojom/heap_profiling_client.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/system/handle.h"

namespace heap_profiling {

class SenderPipe;

// The Client listens on the interface for a StartProfiling message. On
// receiving the message, it begins profiling the current process.
//
// The owner of this object is responsible for binding it to the BinderRegistry.
class Client : public mojom::ProfilingClient {
 public:
  Client();
  ~Client() override;

  // mojom::ProfilingClient overrides:
  void StartProfiling(mojom::ProfilingParamsPtr params) override;
  void FlushMemlogPipe(uint32_t barrier_id) override;

  void BindToInterface(mojom::ProfilingClientRequest request);

 private:
  void InitAllocatorShimOnUIThread(mojom::ProfilingParamsPtr params);

  // Ideally, this would be a mojo::Binding that would only keep alive one
  // client request. However, the service that makes the client requests
  // [content_browser] is different from the service that dedupes the client
  // requests [profiling service]. This means that there may be a brief
  // intervals where there are two active bindings, until the profiling service
  // has a chance to figure out which one to keep.
  mojo::BindingSet<mojom::ProfilingClient> bindings_;

  bool started_profiling_;

  std::unique_ptr<SenderPipe> sender_pipe_;

  base::WeakPtrFactory<Client> weak_factory_;
};

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_CLIENT_H_
