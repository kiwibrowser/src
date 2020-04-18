// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MOJO_BLINK_INTERFACE_PROVIDER_IMPL_H_
#define CONTENT_RENDERER_MOJO_BLINK_INTERFACE_PROVIDER_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "third_party/blink/public/platform/interface_provider.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace service_manager {
class Connector;
}

namespace content {

// An implementation of blink::InterfaceProvider that forwards to a
// service_manager::InterfaceProvider.
class CONTENT_EXPORT BlinkInterfaceProviderImpl final
    : public blink::InterfaceProvider {
 public:
  explicit BlinkInterfaceProviderImpl(service_manager::Connector* connector);
  ~BlinkInterfaceProviderImpl();

  // blink::InterfaceProvider override.
  void GetInterface(const char* name,
                    mojo::ScopedMessagePipeHandle handle) override;

 private:
  const base::WeakPtr<service_manager::Connector> connector_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(BlinkInterfaceProviderImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MOJO_BLINK_INTERFACE_PROVIDER_IMPL_H_
