// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MOJO_INTERFACE_FACTORY_H_
#define ASH_MOJO_INTERFACE_FACTORY_H_

#include "ash/ash_export.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace ash {

namespace mojo_interface_factory {

// Registers all mojo services provided by ash. May be called on IO thread
// (when running ash in-process in chrome) or on the main thread (when running
// in mash).
ASH_EXPORT void RegisterInterfaces(
    service_manager::BinderRegistry* registry,
    scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner);

// Registers a callback that runs during |RegisterInterfaces|. Can be used to
// inject interfaces from test-only build targets.
using RegisterInterfacesCallback =
    base::OnceCallback<void(service_manager::BinderRegistry*,
                            scoped_refptr<base::SingleThreadTaskRunner>)>;
ASH_EXPORT void SetRegisterInterfacesCallback(RegisterInterfacesCallback cb);

}  // namespace mojo_interface_factory

}  // namespace ash

#endif  // ASH_MOJO_INTERFACE_FACTORY_H_
