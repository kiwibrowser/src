// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICE_DEVICE_WAKE_LOCK_WAKE_LOCK_CONTEXT_H_
#define SERVICE_DEVICE_WAKE_LOCK_WAKE_LOCK_CONTEXT_H_

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "services/device/public/mojom/wake_lock_context.mojom.h"
#include "ui/gfx/native_widget_types.h"

namespace device {

// Callback that maps a context ID to the NativeView associated with
// that context. This callback is provided to the Device Service by its
// embedder.
using WakeLockContextCallback = base::Callback<gfx::NativeView(int)>;

// Serves requests for WakeLock connections within a given context.
class WakeLockContext : public mojom::WakeLockContext {
 public:
  WakeLockContext(int context_id,
                  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner,
                  const WakeLockContextCallback& native_view_getter);
  ~WakeLockContext() override;

  // mojom::WakeLockContext:
  void GetWakeLock(mojom::WakeLockType type,
                   mojom::WakeLockReason reason,
                   const std::string& description,
                   mojom::WakeLockRequest request) override;

  static const int WakeLockInvalidContextId;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;
  int context_id_;
  WakeLockContextCallback native_view_getter_;

  DISALLOW_COPY_AND_ASSIGN(WakeLockContext);
};

}  // namespace device

#endif  // SERVICE_DEVICE_WAKE_LOCK_WAKE_LOCK_CONTEXT_H_
