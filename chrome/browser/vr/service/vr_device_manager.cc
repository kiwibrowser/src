// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/service/vr_device_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "chrome/browser/vr/service/browser_xr_device.h"
#include "chrome/common/chrome_features.h"
#include "content/public/common/content_features.h"
#include "content/public/common/service_manager_connection.h"
#include "device/vr/buildflags/buildflags.h"
#include "device/vr/orientation/orientation_device_provider.h"
#include "device/vr/vr_device_provider.h"

#if defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_ARCORE)
#include "device/vr/android/arcore/arcore_device_provider_factory.h"
#endif

#include "device/vr/android/gvr/gvr_device_provider.h"
#endif

#if BUILDFLAG(ENABLE_OPENVR)
#include "device/vr/openvr/openvr_device_provider.h"
#endif

#if BUILDFLAG(ENABLE_OCULUS_VR)
#include "device/vr/oculus/oculus_device_provider.h"
#endif

namespace vr {

namespace {
VRDeviceManager* g_vr_device_manager = nullptr;
}  // namespace

VRDeviceManager* VRDeviceManager::GetInstance() {
  if (!g_vr_device_manager) {
    // Register VRDeviceProviders for the current platform
    ProviderList providers;

#if defined(OS_ANDROID)
    // TODO(https://crbug.com/828321): when we support multiple devices and
    // choosing based on session parameters, add both.
    if (base::FeatureList::IsEnabled(features::kWebXrHitTest)) {
#if BUILDFLAG(ENABLE_ARCORE)
      providers.emplace_back(device::ARCoreDeviceProviderFactory::Create());
#endif
    } else {
      providers.emplace_back(std::make_unique<device::GvrDeviceProvider>());
    }
#endif

#if BUILDFLAG(ENABLE_OPENVR)
    if (base::FeatureList::IsEnabled(features::kOpenVR))
      providers.emplace_back(std::make_unique<device::OpenVRDeviceProvider>());
#endif

#if BUILDFLAG(ENABLE_OCULUS_VR)
    // For now, only use the Oculus when OpenVR is not enabled.
    // TODO(billorr): Add more complicated logic to avoid routing Oculus devices
    // through OpenVR.
    if (base::FeatureList::IsEnabled(features::kOculusVR) &&
        providers.size() == 0)
      providers.emplace_back(
          std::make_unique<device::OculusVRDeviceProvider>());
#endif

    if (base::FeatureList::IsEnabled(features::kWebXrOrientationSensorDevice)) {
      content::ServiceManagerConnection* connection =
          content::ServiceManagerConnection::GetForProcess();
      if (connection) {
        providers.emplace_back(
            std::make_unique<device::VROrientationDeviceProvider>(
                connection->GetConnector()));
      }
    }

    new VRDeviceManager(std::move(providers));
  }
  return g_vr_device_manager;
}

bool VRDeviceManager::HasInstance() {
  return g_vr_device_manager != nullptr;
}

VRDeviceManager::VRDeviceManager(ProviderList providers)
    : providers_(std::move(providers)) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  CHECK(!g_vr_device_manager);
  g_vr_device_manager = this;
}

VRDeviceManager::~VRDeviceManager() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  g_vr_device_manager = nullptr;
}

void VRDeviceManager::AddService(VRServiceImpl* service) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Loop through any currently active devices and send Connected messages to
  // the service. Future devices that come online will send a Connected message
  // when they are created.
  InitializeProviders();

  for (const DeviceMap::value_type& map_entry : devices_) {
    if (!map_entry.second->GetDevice()->IsFallbackDevice() ||
        devices_.size() == 1)
      service->ConnectDevice(map_entry.second.get());
  }

  if (AreAllProvidersInitialized())
    service->InitializationComplete();

  services_.insert(service);
}

void VRDeviceManager::RemoveService(VRServiceImpl* service) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  services_.erase(service);

  if (services_.empty()) {
    // Delete the device manager when it has no active connections.
    delete g_vr_device_manager;
  }
}

void VRDeviceManager::AddDevice(device::VRDevice* device) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(devices_.find(device->GetId()) == devices_.end());
  // Ignore any devices with VR_DEVICE_LAST_ID, which is used to prevent
  // wraparound of device ids.
  if (device->GetId() == device::VR_DEVICE_LAST_ID)
    return;

  // If we were previously using a fallback device, remove it.
  // TODO(offenwanger): This has the potential to cause device change events to
  // fire in rapid succession. This should be discussed and resolved when we
  // start to actually add and remove devices.
  if (devices_.size() == 1 &&
      devices_.begin()->second->GetDevice()->IsFallbackDevice()) {
    BrowserXrDevice* device = devices_.begin()->second.get();
    for (VRServiceImpl* service : services_)
      service->RemoveDevice(device);
  }

  devices_[device->GetId()] = std::make_unique<BrowserXrDevice>(device);
  if (!device->IsFallbackDevice() || devices_.size() == 1) {
    BrowserXrDevice* device_to_add = devices_[device->GetId()].get();
    for (VRServiceImpl* service : services_)
      service->ConnectDevice(device_to_add);
  }
}

void VRDeviceManager::RemoveDevice(device::VRDevice* device) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (device->GetId() == device::VR_DEVICE_LAST_ID)
    return;
  auto it = devices_.find(device->GetId());
  DCHECK(it != devices_.end());

  for (VRServiceImpl* service : services_)
    service->RemoveDevice(it->second.get());

  devices_.erase(it);

  if (devices_.size() == 1 &&
      devices_.begin()->second->GetDevice()->IsFallbackDevice()) {
    BrowserXrDevice* device = devices_.begin()->second.get();
    for (VRServiceImpl* service : services_)
      service->ConnectDevice(device);
  }
}

void VRDeviceManager::RecordVrStartupHistograms() {
#if BUILDFLAG(ENABLE_OPENVR)
  device::OpenVRDeviceProvider::RecordRuntimeAvailability();
#endif
}

device::VRDevice* VRDeviceManager::GetDevice(unsigned int index) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (index == 0)
    return nullptr;

  DeviceMap::iterator iter = devices_.find(index);
  if (iter == devices_.end())
    return nullptr;

  return iter->second->GetDevice();
}

void VRDeviceManager::InitializeProviders() {
  if (providers_initialized_)
    return;

  for (const auto& provider : providers_) {
    provider->Initialize(base::BindRepeating(&VRDeviceManager::AddDevice,
                                             base::Unretained(this)),
                         base::BindRepeating(&VRDeviceManager::RemoveDevice,
                                             base::Unretained(this)),
                         base::BindOnce(&VRDeviceManager::OnProviderInitialized,
                                        base::Unretained(this)));
  }

  providers_initialized_ = true;
}

void VRDeviceManager::OnProviderInitialized() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  ++num_initialized_providers_;
  if (AreAllProvidersInitialized()) {
    for (VRServiceImpl* service : services_)
      service->InitializationComplete();
  }
}

bool VRDeviceManager::AreAllProvidersInitialized() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return num_initialized_providers_ == providers_.size();
}

size_t VRDeviceManager::NumberOfConnectedServices() {
  return services_.size();
}

}  // namespace vr
