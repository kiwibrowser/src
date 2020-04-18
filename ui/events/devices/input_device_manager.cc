// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/devices/input_device_manager.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace ui {
namespace {

// InputDeviceManager singleton is thread-local so that different instances can
// be used on different threads (eg. UI Service thread vs. browser UI thread).
base::LazyInstance<base::ThreadLocalPointer<InputDeviceManager>>::Leaky
    lazy_tls_ptr = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
InputDeviceManager* InputDeviceManager::GetInstance() {
  InputDeviceManager* instance = lazy_tls_ptr.Pointer()->Get();
  DCHECK(instance) << "InputDeviceManager::SetInstance must be called before "
                      "getting the instance of InputDeviceManager.";
  return instance;
}

// static
bool InputDeviceManager::HasInstance() {
  return lazy_tls_ptr.Pointer()->Get() != nullptr;
}

// static
void InputDeviceManager::SetInstance(InputDeviceManager* instance) {
  DCHECK(!lazy_tls_ptr.Pointer()->Get());
  lazy_tls_ptr.Pointer()->Set(instance);
}

// static
void InputDeviceManager::ClearInstance() {
  lazy_tls_ptr.Pointer()->Set(nullptr);
}

}  // namespace ui
