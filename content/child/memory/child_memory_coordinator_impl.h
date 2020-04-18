// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_MEMORY_CHILD_MEMORY_COORDINATOR_IMPL_H_
#define CONTENT_CHILD_MEMORY_CHILD_MEMORY_COORDINATOR_IMPL_H_

#include "base/compiler_specific.h"
#include "base/memory/memory_coordinator_client.h"
#include "base/memory/memory_coordinator_proxy.h"
#include "content/common/child_memory_coordinator.mojom.h"
#include "content/common/content_export.h"
#include "content/common/memory_coordinator.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

class CONTENT_EXPORT ChildMemoryCoordinatorDelegate {
 public:
  virtual ~ChildMemoryCoordinatorDelegate() {}

  // Called when the system requests immediate actions to free memory.
  virtual void OnTrimMemoryImmediately() = 0;
};

// ChildMemoryCoordinatorImpl is the implementation of ChildMemoryCoordinator.
// It lives in child processes and is responsible for dispatching memory events
// to its clients.
class CONTENT_EXPORT ChildMemoryCoordinatorImpl
    : base::MemoryCoordinator,
      public mojom::ChildMemoryCoordinator {
 public:
  // Returns the instance of ChildMemoryCoordinatorImpl. Could be nullptr.
  static ChildMemoryCoordinatorImpl* GetInstance();

  ChildMemoryCoordinatorImpl(mojom::MemoryCoordinatorHandlePtr parent,
                             ChildMemoryCoordinatorDelegate* delegate);
  ~ChildMemoryCoordinatorImpl() override;

  // base::MemoryCoordinator implementations:
  base::MemoryState GetCurrentMemoryState() const override;

  // mojom::ChildMemoryCoordinator implementations:
  void OnStateChange(mojom::MemoryState state) override;
  void PurgeMemory() override;

 protected:
  ChildMemoryCoordinatorDelegate* delegate() { return delegate_; }

 private:
  friend class ChildMemoryCoordinatorImplTest;

  mojo::Binding<mojom::ChildMemoryCoordinator> binding_;
  base::MemoryState current_state_ = base::MemoryState::NORMAL;
  mojom::MemoryCoordinatorHandlePtr parent_;
  ChildMemoryCoordinatorDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(ChildMemoryCoordinatorImpl);
};

// Factory function for creating a ChildMemoryCoordinator for the current
// platform. Doesn't take the ownership of |delegate|.
CONTENT_EXPORT std::unique_ptr<ChildMemoryCoordinatorImpl>
CreateChildMemoryCoordinator(mojom::MemoryCoordinatorHandlePtr parent,
                             ChildMemoryCoordinatorDelegate* delegate);

}  // namespace content

#endif  // CONTENT_CHILD_MEMORY_CHILD_MEMORY_COORDINATOR_IMPL_H_
