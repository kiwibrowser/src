// Copyright 2018 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn_native/DawnNative.h"
#include "dawn_native/Device.h"
#include "dawn_native/Instance.h"
#include "dawn_native/Texture.h"
#include "dawn_platform/DawnPlatform.h"

// Contains the entry-points into dawn_native

namespace dawn_native {

    DawnProcTable GetProcsAutogen();

    DawnProcTable GetProcs() {
        return GetProcsAutogen();
    }

    std::vector<const char*> GetTogglesUsed(WGPUDevice device) {
        const dawn_native::DeviceBase* deviceBase =
            reinterpret_cast<const dawn_native::DeviceBase*>(device);
        return deviceBase->GetTogglesUsed();
    }

    // Adapter

    Adapter::Adapter() = default;

    Adapter::Adapter(AdapterBase* impl) : mImpl(impl) {
    }

    Adapter::~Adapter() {
        mImpl = nullptr;
    }

    void Adapter::GetProperties(wgpu::AdapterProperties* properties) const {
        properties->backendType = mImpl->GetBackendType();
        properties->adapterType = mImpl->GetAdapterType();
        properties->deviceID = mImpl->GetPCIInfo().deviceId;
        properties->vendorID = mImpl->GetPCIInfo().vendorId;
        properties->name = mImpl->GetPCIInfo().name.c_str();
    }

    BackendType Adapter::GetBackendType() const {
        switch (mImpl->GetBackendType()) {
            case wgpu::BackendType::D3D12:
                return BackendType::D3D12;
            case wgpu::BackendType::Metal:
                return BackendType::Metal;
            case wgpu::BackendType::Null:
                return BackendType::Null;
            case wgpu::BackendType::OpenGL:
                return BackendType::OpenGL;
            case wgpu::BackendType::Vulkan:
                return BackendType::Vulkan;
            default:
                UNREACHABLE();
        }
    }

    DeviceType Adapter::GetDeviceType() const {
        switch (mImpl->GetAdapterType()) {
            case wgpu::AdapterType::DiscreteGPU:
                return DeviceType::DiscreteGPU;
            case wgpu::AdapterType::IntegratedGPU:
                return DeviceType::IntegratedGPU;
            case wgpu::AdapterType::CPU:
                return DeviceType::CPU;
            case wgpu::AdapterType::Unknown:
                return DeviceType::Unknown;
            default:
                UNREACHABLE();
        }
    }

    const PCIInfo& Adapter::GetPCIInfo() const {
        return mImpl->GetPCIInfo();
    }

    std::vector<const char*> Adapter::GetSupportedExtensions() const {
        ExtensionsSet supportedExtensionsSet = mImpl->GetSupportedExtensions();
        return supportedExtensionsSet.GetEnabledExtensionNames();
    }

    WGPUDeviceProperties Adapter::GetAdapterProperties() const {
        return mImpl->GetAdapterProperties();
    }

    Adapter::operator bool() const {
        return mImpl != nullptr;
    }

    WGPUDevice Adapter::CreateDevice(const DeviceDescriptor* deviceDescriptor) {
        return reinterpret_cast<WGPUDevice>(mImpl->CreateDevice(deviceDescriptor));
    }

    // AdapterDiscoverOptionsBase

    AdapterDiscoveryOptionsBase::AdapterDiscoveryOptionsBase(WGPUBackendType type)
        : backendType(type) {
    }

    // Instance

    Instance::Instance() : mImpl(InstanceBase::Create()) {
    }

    Instance::~Instance() {
        if (mImpl != nullptr) {
            mImpl->Release();
            mImpl = nullptr;
        }
    }

    void Instance::DiscoverDefaultAdapters() {
        mImpl->DiscoverDefaultAdapters();
    }

    bool Instance::DiscoverAdapters(const AdapterDiscoveryOptionsBase* options) {
        return mImpl->DiscoverAdapters(options);
    }

    std::vector<Adapter> Instance::GetAdapters() const {
        // Adapters are owned by mImpl so it is safe to return non RAII pointers to them
        std::vector<Adapter> adapters;
        for (const std::unique_ptr<AdapterBase>& adapter : mImpl->GetAdapters()) {
            adapters.push_back({adapter.get()});
        }
        return adapters;
    }

    const ToggleInfo* Instance::GetToggleInfo(const char* toggleName) {
        return mImpl->GetToggleInfo(toggleName);
    }

    void Instance::EnableBackendValidation(bool enableBackendValidation) {
        mImpl->EnableBackendValidation(enableBackendValidation);
    }

    void Instance::EnableBeginCaptureOnStartup(bool beginCaptureOnStartup) {
        mImpl->EnableBeginCaptureOnStartup(beginCaptureOnStartup);
    }

    void Instance::SetPlatform(dawn_platform::Platform* platform) {
        mImpl->SetPlatform(platform);
    }

    WGPUInstance Instance::Get() const {
        return reinterpret_cast<WGPUInstance>(mImpl);
    }

    size_t GetLazyClearCountForTesting(WGPUDevice device) {
        dawn_native::DeviceBase* deviceBase = reinterpret_cast<dawn_native::DeviceBase*>(device);
        return deviceBase->GetLazyClearCountForTesting();
    }

    size_t GetDeprecationWarningCountForTesting(WGPUDevice device) {
        dawn_native::DeviceBase* deviceBase = reinterpret_cast<dawn_native::DeviceBase*>(device);
        return deviceBase->GetDeprecationWarningCountForTesting();
    }

    bool IsTextureSubresourceInitialized(WGPUTexture texture,
                                         uint32_t baseMipLevel,
                                         uint32_t levelCount,
                                         uint32_t baseArrayLayer,
                                         uint32_t layerCount) {
        dawn_native::TextureBase* textureBase =
            reinterpret_cast<dawn_native::TextureBase*>(texture);
        SubresourceRange range = {baseMipLevel, levelCount, baseArrayLayer, layerCount};
        return textureBase->IsSubresourceContentInitialized(range);
    }

    std::vector<const char*> GetProcMapNamesForTestingInternal();

    std::vector<const char*> GetProcMapNamesForTesting() {
        return GetProcMapNamesForTestingInternal();
    }

    ExternalImageDescriptor::ExternalImageDescriptor(ExternalImageDescriptorType type)
        : type(type) {
    }

}  // namespace dawn_native
