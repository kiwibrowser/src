// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/platform_handle_dispatcher.h"

#include "base/synchronization/lock.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace mojo {
namespace edk {

// static
scoped_refptr<PlatformHandleDispatcher> PlatformHandleDispatcher::Create(
    ScopedInternalPlatformHandle platform_handle) {
  return new PlatformHandleDispatcher(std::move(platform_handle));
}

ScopedInternalPlatformHandle
PlatformHandleDispatcher::PassInternalPlatformHandle() {
  return std::move(platform_handle_);
}

Dispatcher::Type PlatformHandleDispatcher::GetType() const {
  return Type::PLATFORM_HANDLE;
}

MojoResult PlatformHandleDispatcher::Close() {
  base::AutoLock lock(lock_);
  if (is_closed_ || in_transit_)
    return MOJO_RESULT_INVALID_ARGUMENT;
  is_closed_ = true;
  platform_handle_.reset();
  return MOJO_RESULT_OK;
}

void PlatformHandleDispatcher::StartSerialize(uint32_t* num_bytes,
                                              uint32_t* num_ports,
                                              uint32_t* num_handles) {
  *num_bytes = 0;
  *num_ports = 0;
  *num_handles = 1;
}

bool PlatformHandleDispatcher::EndSerialize(
    void* destination,
    ports::PortName* ports,
    ScopedInternalPlatformHandle* handles) {
  base::AutoLock lock(lock_);
  if (is_closed_)
    return false;
  handles[0] = ScopedInternalPlatformHandle(platform_handle_.get());
  return true;
}

bool PlatformHandleDispatcher::BeginTransit() {
  base::AutoLock lock(lock_);
  if (in_transit_)
    return false;
  in_transit_ = !is_closed_;
  return in_transit_;
}

void PlatformHandleDispatcher::CompleteTransitAndClose() {
  base::AutoLock lock(lock_);

  in_transit_ = false;
  is_closed_ = true;

  // The system has taken ownership of our handle.
  ignore_result(platform_handle_.release());
}

void PlatformHandleDispatcher::CancelTransit() {
  base::AutoLock lock(lock_);
  in_transit_ = false;
}

// static
scoped_refptr<PlatformHandleDispatcher> PlatformHandleDispatcher::Deserialize(
    const void* bytes,
    size_t num_bytes,
    const ports::PortName* ports,
    size_t num_ports,
    ScopedInternalPlatformHandle* handles,
    size_t num_handles) {
  if (num_bytes || num_ports || num_handles != 1)
    return nullptr;

  return PlatformHandleDispatcher::Create(std::move(handles[0]));
}

PlatformHandleDispatcher::PlatformHandleDispatcher(
    ScopedInternalPlatformHandle platform_handle)
    : platform_handle_(std::move(platform_handle)) {}

PlatformHandleDispatcher::~PlatformHandleDispatcher() {
  DCHECK(is_closed_ && !in_transit_);
  DCHECK(!platform_handle_.is_valid());
}

}  // namespace edk
}  // namespace mojo
