// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_WAKE_LOCK_WAKE_LOCK_PROVIDER_H_
#define SERVICES_DEVICE_WAKE_LOCK_WAKE_LOCK_PROVIDER_H_

#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/device/public/mojom/wake_lock_context.mojom.h"
#include "services/device/public/mojom/wake_lock_provider.mojom.h"
#include "services/device/wake_lock/wake_lock_context.h"
#include "ui/gfx/native_widget_types.h"

namespace device {

// Serves requests for WakeLockContext connections.
class WakeLockProvider : public mojom::WakeLockProvider {
 public:
  WakeLockProvider(scoped_refptr<base::SingleThreadTaskRunner> file_task_runner,
                   const WakeLockContextCallback& native_view_getter);
  ~WakeLockProvider() override;

  static void Create(
      mojom::WakeLockProviderRequest request,
      scoped_refptr<base::SingleThreadTaskRunner> file_task_runner,
      const WakeLockContextCallback& native_view_getter);

  // mojom::WakeLockProvider:
  void GetWakeLockContextForID(
      int context_id,
      mojo::InterfaceRequest<mojom::WakeLockContext> request) override;

  void GetWakeLockWithoutContext(mojom::WakeLockType type,
                                 mojom::WakeLockReason reason,
                                 const std::string& description,
                                 mojom::WakeLockRequest request) override;

  static bool is_in_unittest_;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;
  WakeLockContextCallback native_view_getter_;

  DISALLOW_COPY_AND_ASSIGN(WakeLockProvider);
};

}  // namespace device

#endif  // SERVICES_DEVICE_WAKE_LOCK_WAKE_LOCK_PROVIDER_H_
