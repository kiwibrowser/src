// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/public/ozone_platform.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/ozone/platform_object.h"
#include "ui/ozone/platform_selection.h"

namespace ui {

namespace {

bool g_platform_initialized_ui = false;
bool g_platform_initialized_gpu = false;
base::LazyInstance<base::OnceCallback<void(OzonePlatform*)>>::Leaky
    instance_callback = LAZY_INSTANCE_INITIALIZER;

base::Lock& GetOzoneInstanceLock() {
  static base::Lock lock;
  return lock;
}

}  // namespace

OzonePlatform::OzonePlatform() {
  GetOzoneInstanceLock().AssertAcquired();
  DCHECK(!instance_) << "There should only be a single OzonePlatform.";
  instance_ = this;
  g_platform_initialized_ui = false;
  g_platform_initialized_gpu = false;
}

OzonePlatform::~OzonePlatform() = default;

// static
void OzonePlatform::InitializeForUI(const InitParams& args) {
  EnsureInstance();
  if (g_platform_initialized_ui)
    return;
  instance_->InitializeUI(args);
  {
    base::AutoLock lock(GetOzoneInstanceLock());
    g_platform_initialized_ui = true;
  }
  // This is deliberately created after initializing so that the platform can
  // create its own version of DDM.
  DeviceDataManager::CreateInstance();
  if (!instance_callback.Get().is_null())
    std::move(instance_callback.Get()).Run(instance_);
}

// static
void OzonePlatform::InitializeForGPU(const InitParams& args) {
  EnsureInstance();
  if (g_platform_initialized_gpu)
    return;
  instance_->InitializeGPU(args);
  {
    base::AutoLock lock(GetOzoneInstanceLock());
    g_platform_initialized_gpu = true;
  }
  if (!args.single_process && !instance_callback.Get().is_null())
    std::move(instance_callback.Get()).Run(instance_);
}

// static
void OzonePlatform::Shutdown() {
  base::AutoLock lock(GetOzoneInstanceLock());
  auto* tmp = instance_;
  instance_ = nullptr;
  delete tmp;
}

// static
OzonePlatform* OzonePlatform::GetInstance() {
  base::AutoLock lock(GetOzoneInstanceLock());
  DCHECK(instance_) << "OzonePlatform is not initialized";
  return instance_;
}

// static
OzonePlatform* OzonePlatform::EnsureInstance() {
  base::AutoLock lock(GetOzoneInstanceLock());
  if (!instance_) {
    TRACE_EVENT1("ozone",
                 "OzonePlatform::Initialize",
                 "platform",
                 GetOzonePlatformName());
    std::unique_ptr<OzonePlatform> platform =
        PlatformObject<OzonePlatform>::Create();

    // TODO(spang): Currently need to leak this object.
    OzonePlatform* pl = platform.release();
    DCHECK_EQ(instance_, pl);
  }
  return instance_;
}

// static
void OzonePlatform::RegisterStartupCallback(
    base::OnceCallback<void(OzonePlatform*)> callback) {
  OzonePlatform* inst = nullptr;
  {
    base::AutoLock lock(GetOzoneInstanceLock());
    if (!instance_ || !g_platform_initialized_ui) {
      instance_callback.Get() = std::move(callback);
      return;
    }
    inst = instance_;
  }
  std::move(callback).Run(inst);
}

// static
OzonePlatform* OzonePlatform::instance_ = nullptr;

IPC::MessageFilter* OzonePlatform::GetGpuMessageFilter() {
  return nullptr;
}

base::MessageLoop::Type OzonePlatform::GetMessageLoopTypeForGpu() {
  return base::MessageLoop::TYPE_DEFAULT;
}

void OzonePlatform::AddInterfaces(
    service_manager::BinderRegistryWithArgs<
        const service_manager::BindSourceInfo&>* registry) {}

void OzonePlatform::AfterSandboxEntry() {}

}  // namespace ui
