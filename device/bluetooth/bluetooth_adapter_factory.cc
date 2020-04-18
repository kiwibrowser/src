// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter_factory.h"

#include <vector>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "device/bluetooth/bluetooth_adapter.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#endif
#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif
#if defined(ANDROID)
#include "base/android/build_info.h"
#endif

namespace device {

namespace {

static base::LazyInstance<BluetoothAdapterFactory>::Leaky g_singleton =
    LAZY_INSTANCE_INITIALIZER;

// Shared default adapter instance.  We don't want to keep this class around
// if nobody is using it, so use a WeakPtr and create the object when needed.
// Since Google C++ Style (and clang's static analyzer) forbids us having
// exit-time destructors, we use a leaky lazy instance for it.
base::LazyInstance<base::WeakPtr<BluetoothAdapter>>::Leaky default_adapter =
    LAZY_INSTANCE_INITIALIZER;

#if defined(OS_WIN) || defined(OS_LINUX)
typedef std::vector<BluetoothAdapterFactory::AdapterCallback>
    AdapterCallbackList;

// List of adapter callbacks to be called once the adapter is initialized.
// Since Google C++ Style (and clang's static analyzer) forbids us having
// exit-time destructors we use a lazy instance for it.
base::LazyInstance<AdapterCallbackList>::DestructorAtExit adapter_callbacks =
    LAZY_INSTANCE_INITIALIZER;

void RunAdapterCallbacks() {
  DCHECK(default_adapter.Get());
  scoped_refptr<BluetoothAdapter> adapter(default_adapter.Get().get());
  for (std::vector<BluetoothAdapterFactory::AdapterCallback>::const_iterator
           iter = adapter_callbacks.Get().begin();
       iter != adapter_callbacks.Get().end();
       ++iter) {
    iter->Run(adapter);
  }
  adapter_callbacks.Get().clear();
}
#endif  // defined(OS_WIN) || defined(OS_LINUX)

}  // namespace

BluetoothAdapterFactory::~BluetoothAdapterFactory() = default;

// static
BluetoothAdapterFactory& BluetoothAdapterFactory::Get() {
  return g_singleton.Get();
}

// static
bool BluetoothAdapterFactory::IsBluetoothSupported() {
  // SetAdapterForTesting() may be used to provide a test or mock adapter
  // instance even on platforms that would otherwise not support it.
  if (default_adapter.Get())
    return true;
#if defined(OS_ANDROID) || defined(OS_WIN) || defined(OS_LINUX) || \
    defined(OS_MACOSX)
  return true;
#else
  return false;
#endif
}

bool BluetoothAdapterFactory::IsLowEnergySupported() {
  if (values_for_testing_) {
    return values_for_testing_->GetLESupported();
  }

#if defined(OS_ANDROID)
  return base::android::BuildInfo::GetInstance()->sdk_int() >=
         base::android::SDK_VERSION_MARSHMALLOW;
#elif defined(OS_WIN)
  // Windows 8 supports Low Energy GATT operations but it does not support
  // scanning, initiating connections and GATT Server. To keep the API
  // consistent we consider Windows 8 as lacking Low Energy support.
  return base::win::GetVersion() >= base::win::VERSION_WIN10;
#elif defined(OS_MACOSX)
  return base::mac::IsAtLeastOS10_10();
#elif defined(OS_LINUX)
  return true;
#else
  return false;
#endif
}

// static
void BluetoothAdapterFactory::GetAdapter(const AdapterCallback& callback) {
  DCHECK(IsBluetoothSupported());

#if defined(OS_WIN) || defined(OS_LINUX)
  if (!default_adapter.Get()) {
    default_adapter.Get() =
        BluetoothAdapter::CreateAdapter(base::Bind(&RunAdapterCallbacks));
    DCHECK(!default_adapter.Get()->IsInitialized());
  }

  if (!default_adapter.Get()->IsInitialized())
    adapter_callbacks.Get().push_back(callback);
#else   // !defined(OS_WIN) && !defined(OS_LINUX)
  if (!default_adapter.Get()) {
    default_adapter.Get() =
        BluetoothAdapter::CreateAdapter(BluetoothAdapter::InitCallback());
  }

  DCHECK(default_adapter.Get()->IsInitialized());
#endif  // defined(OS_WIN) || defined(OS_LINUX)

  if (default_adapter.Get()->IsInitialized())
    callback.Run(scoped_refptr<BluetoothAdapter>(default_adapter.Get().get()));
}

#if defined(OS_LINUX)
// static
void BluetoothAdapterFactory::Shutdown() {
  if (default_adapter.Get())
    default_adapter.Get().get()->Shutdown();
}
#endif

// static
void BluetoothAdapterFactory::SetAdapterForTesting(
    scoped_refptr<BluetoothAdapter> adapter) {
  default_adapter.Get() = adapter->GetWeakPtrForTesting();
}

// static
bool BluetoothAdapterFactory::HasSharedInstanceForTesting() {
  return default_adapter.Get() != nullptr;
}

BluetoothAdapterFactory::GlobalValuesForTesting::GlobalValuesForTesting()
    : weak_ptr_factory_(this) {}

BluetoothAdapterFactory::GlobalValuesForTesting::~GlobalValuesForTesting() =
    default;

base::WeakPtr<BluetoothAdapterFactory::GlobalValuesForTesting>
BluetoothAdapterFactory::GlobalValuesForTesting::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

std::unique_ptr<BluetoothAdapterFactory::GlobalValuesForTesting>
BluetoothAdapterFactory::InitGlobalValuesForTesting() {
  auto v = std::make_unique<BluetoothAdapterFactory::GlobalValuesForTesting>();
  values_for_testing_ = v->GetWeakPtr();
  return v;
}

BluetoothAdapterFactory::BluetoothAdapterFactory() = default;

}  // namespace device
