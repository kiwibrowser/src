// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_PLATFORM_HANDLE_DISPATCHER_H_
#define MOJO_EDK_SYSTEM_PLATFORM_HANDLE_DISPATCHER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/dispatcher.h"
#include "mojo/edk/system/system_impl_export.h"

namespace mojo {
namespace edk {

class MOJO_SYSTEM_IMPL_EXPORT PlatformHandleDispatcher : public Dispatcher {
 public:
  static scoped_refptr<PlatformHandleDispatcher> Create(
      ScopedInternalPlatformHandle platform_handle);

  ScopedInternalPlatformHandle PassInternalPlatformHandle();

  // Dispatcher:
  Type GetType() const override;
  MojoResult Close() override;
  void StartSerialize(uint32_t* num_bytes,
                      uint32_t* num_ports,
                      uint32_t* num_handles) override;
  bool EndSerialize(void* destination,
                    ports::PortName* ports,
                    ScopedInternalPlatformHandle* handles) override;
  bool BeginTransit() override;
  void CompleteTransitAndClose() override;
  void CancelTransit() override;

  static scoped_refptr<PlatformHandleDispatcher> Deserialize(
      const void* bytes,
      size_t num_bytes,
      const ports::PortName* ports,
      size_t num_ports,
      ScopedInternalPlatformHandle* handles,
      size_t num_handles);

 private:
  PlatformHandleDispatcher(ScopedInternalPlatformHandle platform_handle);
  ~PlatformHandleDispatcher() override;

  base::Lock lock_;
  bool in_transit_ = false;
  bool is_closed_ = false;
  ScopedInternalPlatformHandle platform_handle_;

  DISALLOW_COPY_AND_ASSIGN(PlatformHandleDispatcher);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_PLATFORM_HANDLE_DISPATCHER_H_
