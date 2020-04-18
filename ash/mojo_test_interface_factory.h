// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MOJO_TEST_INTERFACE_FACTORY_H_
#define ASH_MOJO_TEST_INTERFACE_FACTORY_H_

#include "ash/ash_export.h"
#include "base/memory/ref_counted.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace ash {
namespace mojo_test_interface_factory {

// Registers all mojo test interfaces provided by ash. May be called on IO
// thread (when running ash in-process in chrome) or on the main thread (when
// running in mash).
ASH_EXPORT void RegisterInterfaces(
    service_manager::BinderRegistry* registry,
    scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner);

}  // namespace mojo_test_interface_factory
}  // namespace ash

#endif  // ASH_MOJO_TEST_INTERFACE_FACTORY_H_
