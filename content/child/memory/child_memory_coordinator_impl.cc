// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/memory/child_memory_coordinator_impl.h"

#include "base/lazy_instance.h"
#include "base/memory/memory_coordinator_client_registry.h"
#include "base/memory/ptr_util.h"
#include "base/synchronization/lock.h"
#include "base/trace_event/trace_event.h"

namespace content {

namespace {

base::LazyInstance<base::Lock>::Leaky g_lock = LAZY_INSTANCE_INITIALIZER;
ChildMemoryCoordinatorImpl* g_child_memory_coordinator = nullptr;

base::MemoryState ToBaseMemoryState(mojom::MemoryState state) {
  switch (state) {
    case mojom::MemoryState::UNKNOWN:
      return base::MemoryState::UNKNOWN;
    case mojom::MemoryState::NORMAL:
      return base::MemoryState::NORMAL;
    case mojom::MemoryState::THROTTLED:
      return base::MemoryState::THROTTLED;
    case mojom::MemoryState::SUSPENDED:
      return base::MemoryState::SUSPENDED;
    default:
      NOTREACHED();
      return base::MemoryState::UNKNOWN;
  }
}

}  // namespace

// static
ChildMemoryCoordinatorImpl* ChildMemoryCoordinatorImpl::GetInstance() {
  base::AutoLock lock(*g_lock.Pointer());
  return g_child_memory_coordinator;
}

ChildMemoryCoordinatorImpl::ChildMemoryCoordinatorImpl(
    mojom::MemoryCoordinatorHandlePtr parent,
    ChildMemoryCoordinatorDelegate* delegate)
    : binding_(this), parent_(std::move(parent)), delegate_(delegate) {
  base::AutoLock lock(*g_lock.Pointer());
  DCHECK(delegate_);
  DCHECK(!g_child_memory_coordinator);
  g_child_memory_coordinator = this;
  mojom::ChildMemoryCoordinatorPtr child;
  binding_.Bind(mojo::MakeRequest(&child));
  parent_->AddChild(std::move(child));
  base::MemoryCoordinatorProxy::SetMemoryCoordinator(this);
}

ChildMemoryCoordinatorImpl::~ChildMemoryCoordinatorImpl() {
  base::AutoLock lock(*g_lock.Pointer());
  base::MemoryCoordinatorProxy::SetMemoryCoordinator(nullptr);
  DCHECK(g_child_memory_coordinator == this);
  g_child_memory_coordinator = nullptr;
}

base::MemoryState ChildMemoryCoordinatorImpl::GetCurrentMemoryState() const {
  return current_state_;
}

void ChildMemoryCoordinatorImpl::PurgeMemory() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("memory_coordinator"),
               "ChildMemoryCoordinatorImpl::PurgeMemory");
  base::MemoryCoordinatorClientRegistry::GetInstance()->PurgeMemory();
}

void ChildMemoryCoordinatorImpl::OnStateChange(mojom::MemoryState state) {
  current_state_ = ToBaseMemoryState(state);
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("memory_coordinator"),
               "ChildMemoryCoordinatorImpl::OnStateChange", "state",
               MemoryStateToString(current_state_));
  base::MemoryCoordinatorClientRegistry::GetInstance()->Notify(current_state_);
}

#if !defined(OS_ANDROID)
std::unique_ptr<ChildMemoryCoordinatorImpl> CreateChildMemoryCoordinator(
    mojom::MemoryCoordinatorHandlePtr parent,
    ChildMemoryCoordinatorDelegate* delegate) {
  return base::WrapUnique(
      new ChildMemoryCoordinatorImpl(std::move(parent), delegate));
}
#endif

}  // namespace content
