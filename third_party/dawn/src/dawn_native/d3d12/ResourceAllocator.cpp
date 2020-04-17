// Copyright 2017 The Dawn Authors
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

#include "dawn_native/d3d12/ResourceAllocator.h"

#include "dawn_native/d3d12/DeviceD3D12.h"

namespace dawn_native { namespace d3d12 {

    namespace {
        static constexpr D3D12_HEAP_PROPERTIES kDefaultHeapProperties = {
            D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0,
            0};

        static constexpr D3D12_HEAP_PROPERTIES kUploadHeapProperties = {
            D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0,
            0};

        static constexpr D3D12_HEAP_PROPERTIES kReadbackHeapProperties = {
            D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0,
            0};
    }  // namespace

    ResourceAllocator::ResourceAllocator(Device* device) : mDevice(device) {
    }

    ComPtr<ID3D12Resource> ResourceAllocator::Allocate(
        D3D12_HEAP_TYPE heapType,
        const D3D12_RESOURCE_DESC& resourceDescriptor,
        D3D12_RESOURCE_STATES initialUsage) {
        const D3D12_HEAP_PROPERTIES* heapProperties = nullptr;
        switch (heapType) {
            case D3D12_HEAP_TYPE_DEFAULT:
                heapProperties = &kDefaultHeapProperties;
                break;
            case D3D12_HEAP_TYPE_UPLOAD:
                heapProperties = &kUploadHeapProperties;
                break;
            case D3D12_HEAP_TYPE_READBACK:
                heapProperties = &kReadbackHeapProperties;
                break;
            default:
                UNREACHABLE();
        }

        ComPtr<ID3D12Resource> resource;

        // TODO(enga@google.com): Use CreatePlacedResource
        ASSERT_SUCCESS(mDevice->GetD3D12Device()->CreateCommittedResource(
            heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescriptor, initialUsage, nullptr,
            IID_PPV_ARGS(&resource)));

        return resource;
    }

    void ResourceAllocator::Release(ComPtr<ID3D12Resource> resource) {
        // Resources may still be in use on the GPU. Enqueue them so that we hold onto them until
        // GPU execution has completed
        mReleasedResources.Enqueue(resource, mDevice->GetPendingCommandSerial());
    }

    void ResourceAllocator::Tick(uint64_t lastCompletedSerial) {
        mReleasedResources.ClearUpTo(lastCompletedSerial);
    }

}}  // namespace dawn_native::d3d12
