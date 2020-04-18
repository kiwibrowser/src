// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "components/arc/test/fake_wake_lock_instance.h"

namespace arc {

FakeWakeLockInstance::FakeWakeLockInstance() = default;

FakeWakeLockInstance::~FakeWakeLockInstance() = default;

void FakeWakeLockInstance::Init(mojom::WakeLockHostPtr host_ptr,
                                InitCallback callback) {
  host_ptr_ = std::move(host_ptr);
  std::move(callback).Run();
}

}  // namespace arc
