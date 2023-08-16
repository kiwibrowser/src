// Copyright 2019 The Dawn Authors
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

#include "dawn_native/d3d12/ResourceAllocatorManagerD3D12.h"

#include "dawn_native/d3d12/D3D12Error.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/HeapAllocatorD3D12.h"
#include "dawn_native/d3d12/HeapD3D12.h"
#include "dawn_native/d3d12/ResidencyManagerD3D12.h"
#include "dawn_native/d3d12/UtilsD3D12.h"

namespace dawn_native { namespace d3d12 {
    namespace {
        MemorySegment GetMemorySegment(Device* device, D3D12_HEAP_TYPE heapType) {
            if (device->GetDeviceInfo().isUMA) {
                return MemorySegment::Local;
            }

            D3D12_HEAP_PROPERTIES heapProperties =
                device->GetD3D12Device()->GetCustomHeapProperties(0, heapType);

            if (heapProperties.MemoryPoolPreference == D3D12_MEMORY_POOL_L1) {
                return MemorySegment::Local;
            }

            return MemorySegment::NonLocal;
        }

        D3D12_HEAP_TYPE GetD3D12HeapType(ResourceHeapKind resourceHeapKind) {
            switch (resourceHeapKind) {
                case Readback_OnlyBuffers:
                case Readback_AllBuffersAndTextures:
                    return D3D12_HEAP_TYPE_READBACK;
                case Default_AllBuffersAndTextures:
                case Default_OnlyBuffers:
                case Default_OnlyNonRenderableOrDepthTextures:
                case Default_OnlyRenderableOrDepthTextures:
                    return D3D12_HEAP_TYPE_DEFAULT;
                case Upload_OnlyBuffers:
                case Upload_AllBuffersAndTextures:
                    return D3D12_HEAP_TYPE_UPLOAD;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_HEAP_FLAGS GetD3D12HeapFlags(ResourceHeapKind resourceHeapKind) {
            switch (resourceHeapKind) {
                case Default_AllBuffersAndTextures:
                case Readback_AllBuffersAndTextures:
                case Upload_AllBuffersAndTextures:
                    return D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
                case Default_OnlyBuffers:
                case Readback_OnlyBuffers:
                case Upload_OnlyBuffers:
                    return D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
                case Default_OnlyNonRenderableOrDepthTextures:
                    return D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
                case Default_OnlyRenderableOrDepthTextures:
                    return D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
                default:
                    UNREACHABLE();
            }
        }

        ResourceHeapKind GetResourceHeapKind(D3D12_RESOURCE_DIMENSION dimension,
                                             D3D12_HEAP_TYPE heapType,
                                             D3D12_RESOURCE_FLAGS flags,
                                             uint32_t resourceHeapTier) {
            if (resourceHeapTier >= 2) {
                switch (heapType) {
                    case D3D12_HEAP_TYPE_UPLOAD:
                        return Upload_AllBuffersAndTextures;
                    case D3D12_HEAP_TYPE_DEFAULT:
                        return Default_AllBuffersAndTextures;
                    case D3D12_HEAP_TYPE_READBACK:
                        return Readback_AllBuffersAndTextures;
                    default:
                        UNREACHABLE();
                }
            }

            switch (dimension) {
                case D3D12_RESOURCE_DIMENSION_BUFFER: {
                    switch (heapType) {
                        case D3D12_HEAP_TYPE_UPLOAD:
                            return Upload_OnlyBuffers;
                        case D3D12_HEAP_TYPE_DEFAULT:
                            return Default_OnlyBuffers;
                        case D3D12_HEAP_TYPE_READBACK:
                            return Readback_OnlyBuffers;
                        default:
                            UNREACHABLE();
                    }
                    break;
                }
                case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                case D3D12_RESOURCE_DIMENSION_TEXTURE3D: {
                    switch (heapType) {
                        case D3D12_HEAP_TYPE_DEFAULT: {
                            if ((flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ||
                                (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)) {
                                return Default_OnlyRenderableOrDepthTextures;
                            }
                            return Default_OnlyNonRenderableOrDepthTextures;
                        }

                        default:
                            UNREACHABLE();
                    }
                    break;
                }
                default:
                    UNREACHABLE();
            }
        }

        uint64_t GetResourcePlacementAlignment(ResourceHeapKind resourceHeapKind,
                                               uint32_t sampleCount,
                                               uint64_t requestedAlignment) {
            switch (resourceHeapKind) {
                // Small resources can take advantage of smaller alignments. For example,
                // if the most detailed mip can fit under 64KB, 4KB alignments can be used.
                // Must be non-depth or without render-target to use small resource alignment.
                // This also applies to MSAA textures (4MB => 64KB).
                //
                // Note: Only known to be used for small textures; however, MSDN suggests
                // it could be extended for more cases. If so, this could default to always
                // attempt small resource placement.
                // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_desc
                case Default_OnlyNonRenderableOrDepthTextures:
                    return (sampleCount > 1) ? D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
                                             : D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
                default:
                    return requestedAlignment;
            }
        }

        bool IsClearValueOptimizable(const D3D12_RESOURCE_DESC& resourceDescriptor) {
            // Optimized clear color cannot be set on buffers, non-render-target/depth-stencil
            // textures, or typeless resources
            // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createcommittedresource
            // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createplacedresource
            return !IsTypeless(resourceDescriptor.Format) &&
                   resourceDescriptor.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER &&
                   (resourceDescriptor.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
                                                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0;
        }

    }  // namespace

    ResourceAllocatorManager::ResourceAllocatorManager(Device* device) : mDevice(device) {
        mResourceHeapTier = (mDevice->IsToggleEnabled(Toggle::UseD3D12ResourceHeapTier2))
                                ? mDevice->GetDeviceInfo().resourceHeapTier
                                : 1;

        for (uint32_t i = 0; i < ResourceHeapKind::EnumCount; i++) {
            const ResourceHeapKind resourceHeapKind = static_cast<ResourceHeapKind>(i);
            mHeapAllocators[i] = std::make_unique<HeapAllocator>(
                mDevice, GetD3D12HeapType(resourceHeapKind), GetD3D12HeapFlags(resourceHeapKind),
                GetMemorySegment(device, GetD3D12HeapType(resourceHeapKind)));
            mSubAllocatedResourceAllocators[i] = std::make_unique<BuddyMemoryAllocator>(
                kMaxHeapSize, kMinHeapSize, mHeapAllocators[i].get());
        }
    }

    ResultOrError<ResourceHeapAllocation> ResourceAllocatorManager::AllocateMemory(
        D3D12_HEAP_TYPE heapType,
        const D3D12_RESOURCE_DESC& resourceDescriptor,
        D3D12_RESOURCE_STATES initialUsage) {
        // In order to suppress a warning in the D3D12 debug layer, we need to specify an
        // optimized clear value. As there are no negative consequences when picking a mismatched
        // clear value, we use zero as the optimized clear value. This also enables fast clears on
        // some architectures.
        D3D12_CLEAR_VALUE zero{};
        D3D12_CLEAR_VALUE* optimizedClearValue = nullptr;
        if (IsClearValueOptimizable(resourceDescriptor)) {
            zero.Format = resourceDescriptor.Format;
            optimizedClearValue = &zero;
        }

        // TODO(bryan.bernhart@intel.com): Conditionally disable sub-allocation.
        // For very large resources, there is no benefit to suballocate.
        // For very small resources, it is inefficent to suballocate given the min. heap
        // size could be much larger then the resource allocation.
        // Attempt to satisfy the request using sub-allocation (placed resource in a heap).
        ResourceHeapAllocation subAllocation;
        DAWN_TRY_ASSIGN(subAllocation, CreatePlacedResource(heapType, resourceDescriptor,
                                                            optimizedClearValue, initialUsage));
        if (subAllocation.GetInfo().mMethod != AllocationMethod::kInvalid) {
            return std::move(subAllocation);
        }

        // If sub-allocation fails, fall-back to direct allocation (committed resource).
        ResourceHeapAllocation directAllocation;
        DAWN_TRY_ASSIGN(directAllocation,
                        CreateCommittedResource(heapType, resourceDescriptor, optimizedClearValue,
                                                initialUsage));
        if (directAllocation.GetInfo().mMethod != AllocationMethod::kInvalid) {
            return std::move(directAllocation);
        }

        // If direct allocation fails, the system is probably out of memory.
        return DAWN_OUT_OF_MEMORY_ERROR("Allocation failed");
    }

    void ResourceAllocatorManager::Tick(Serial completedSerial) {
        for (ResourceHeapAllocation& allocation :
             mAllocationsToDelete.IterateUpTo(completedSerial)) {
            if (allocation.GetInfo().mMethod == AllocationMethod::kSubAllocated) {
                FreeMemory(allocation);
            }
        }
        mAllocationsToDelete.ClearUpTo(completedSerial);
    }

    void ResourceAllocatorManager::DeallocateMemory(ResourceHeapAllocation& allocation) {
        if (allocation.GetInfo().mMethod == AllocationMethod::kInvalid) {
            return;
        }

        mAllocationsToDelete.Enqueue(allocation, mDevice->GetPendingCommandSerial());

        // Directly allocated ResourceHeapAllocations are created with a heap object that must be
        // manually deleted upon deallocation. See ResourceAllocatorManager::CreateCommittedResource
        // for more information.
        if (allocation.GetInfo().mMethod == AllocationMethod::kDirect) {
            delete allocation.GetResourceHeap();
        }

        // Invalidate the allocation immediately in case one accidentally
        // calls DeallocateMemory again using the same allocation.
        allocation.Invalidate();

        ASSERT(allocation.GetD3D12Resource() == nullptr);
    }

    void ResourceAllocatorManager::FreeMemory(ResourceHeapAllocation& allocation) {
        ASSERT(allocation.GetInfo().mMethod == AllocationMethod::kSubAllocated);

        D3D12_HEAP_PROPERTIES heapProp;
        allocation.GetD3D12Resource()->GetHeapProperties(&heapProp, nullptr);

        const D3D12_RESOURCE_DESC resourceDescriptor = allocation.GetD3D12Resource()->GetDesc();

        const size_t resourceHeapKindIndex =
            GetResourceHeapKind(resourceDescriptor.Dimension, heapProp.Type,
                                resourceDescriptor.Flags, mResourceHeapTier);

        mSubAllocatedResourceAllocators[resourceHeapKindIndex]->Deallocate(allocation);
    }

    ResultOrError<ResourceHeapAllocation> ResourceAllocatorManager::CreatePlacedResource(
        D3D12_HEAP_TYPE heapType,
        const D3D12_RESOURCE_DESC& requestedResourceDescriptor,
        const D3D12_CLEAR_VALUE* optimizedClearValue,
        D3D12_RESOURCE_STATES initialUsage) {
        const ResourceHeapKind resourceHeapKind =
            GetResourceHeapKind(requestedResourceDescriptor.Dimension, heapType,
                                requestedResourceDescriptor.Flags, mResourceHeapTier);

        D3D12_RESOURCE_DESC resourceDescriptor = requestedResourceDescriptor;
        resourceDescriptor.Alignment = GetResourcePlacementAlignment(
            resourceHeapKind, requestedResourceDescriptor.SampleDesc.Count,
            requestedResourceDescriptor.Alignment);

        D3D12_RESOURCE_ALLOCATION_INFO resourceInfo =
            mDevice->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &resourceDescriptor);

        // If the requested resource alignment was rejected, let D3D tell us what the
        // required alignment is for this resource.
        if (resourceDescriptor.Alignment != resourceInfo.Alignment) {
            resourceDescriptor.Alignment = 0;
            resourceInfo =
                mDevice->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &resourceDescriptor);
        }

        // If d3d tells us the resource size is invalid, treat the error as OOM.
        // Otherwise, creating the resource could cause a device loss (too large).
        // This is because NextPowerOfTwo(UINT64_MAX) overflows and proceeds to
        // incorrectly allocate a mismatched size.
        if (resourceInfo.SizeInBytes == 0 ||
            resourceInfo.SizeInBytes == std::numeric_limits<uint64_t>::max()) {
            return DAWN_OUT_OF_MEMORY_ERROR("Resource allocation size was invalid.");
        }

        BuddyMemoryAllocator* allocator =
            mSubAllocatedResourceAllocators[static_cast<size_t>(resourceHeapKind)].get();

        ResourceMemoryAllocation allocation;
        DAWN_TRY_ASSIGN(allocation,
                        allocator->Allocate(resourceInfo.SizeInBytes, resourceInfo.Alignment));
        if (allocation.GetInfo().mMethod == AllocationMethod::kInvalid) {
            return ResourceHeapAllocation{};  // invalid
        }

        Heap* heap = ToBackend(allocation.GetResourceHeap());

        // Before calling CreatePlacedResource, we must ensure the target heap is resident.
        // CreatePlacedResource will fail if it is not.
        DAWN_TRY(mDevice->GetResidencyManager()->LockAllocation(heap));

        // With placed resources, a single heap can be reused.
        // The resource placed at an offset is only reclaimed
        // upon Tick or after the last command list using the resource has completed
        // on the GPU. This means the same physical memory is not reused
        // within the same command-list and does not require additional synchronization (aliasing
        // barrier).
        // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createplacedresource
        ComPtr<ID3D12Resource> placedResource;
        DAWN_TRY(CheckOutOfMemoryHRESULT(
            mDevice->GetD3D12Device()->CreatePlacedResource(
                heap->GetD3D12Heap(), allocation.GetOffset(), &resourceDescriptor, initialUsage,
                optimizedClearValue, IID_PPV_ARGS(&placedResource)),
            "ID3D12Device::CreatePlacedResource"));

        // After CreatePlacedResource has finished, the heap can be unlocked from residency. This
        // will insert it into the residency LRU.
        mDevice->GetResidencyManager()->UnlockAllocation(heap);

        return ResourceHeapAllocation{allocation.GetInfo(), allocation.GetOffset(),
                                      std::move(placedResource), heap};
    }

    ResultOrError<ResourceHeapAllocation> ResourceAllocatorManager::CreateCommittedResource(
        D3D12_HEAP_TYPE heapType,
        const D3D12_RESOURCE_DESC& resourceDescriptor,
        const D3D12_CLEAR_VALUE* optimizedClearValue,
        D3D12_RESOURCE_STATES initialUsage) {
        D3D12_HEAP_PROPERTIES heapProperties;
        heapProperties.Type = heapType;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 0;
        heapProperties.VisibleNodeMask = 0;

        // If d3d tells us the resource size is invalid, treat the error as OOM.
        // Otherwise, creating the resource could cause a device loss (too large).
        // This is because NextPowerOfTwo(UINT64_MAX) overflows and proceeds to
        // incorrectly allocate a mismatched size.
        D3D12_RESOURCE_ALLOCATION_INFO resourceInfo =
            mDevice->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &resourceDescriptor);
        if (resourceInfo.SizeInBytes == 0 ||
            resourceInfo.SizeInBytes == std::numeric_limits<uint64_t>::max()) {
            return DAWN_OUT_OF_MEMORY_ERROR("Resource allocation size was invalid.");
        }

        if (resourceInfo.SizeInBytes > kMaxHeapSize) {
            return ResourceHeapAllocation{};  // Invalid
        }

        // CreateCommittedResource will implicitly make the created resource resident. We must
        // ensure enough free memory exists before allocating to avoid an out-of-memory error when
        // overcommitted.
        DAWN_TRY(mDevice->GetResidencyManager()->EnsureCanAllocate(
            resourceInfo.SizeInBytes, GetMemorySegment(mDevice, heapType)));

        // Note: Heap flags are inferred by the resource descriptor and do not need to be explicitly
        // provided to CreateCommittedResource.
        ComPtr<ID3D12Resource> committedResource;
        DAWN_TRY(CheckOutOfMemoryHRESULT(
            mDevice->GetD3D12Device()->CreateCommittedResource(
                &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescriptor, initialUsage,
                optimizedClearValue, IID_PPV_ARGS(&committedResource)),
            "ID3D12Device::CreateCommittedResource"));

        // When using CreateCommittedResource, D3D12 creates an implicit heap that contains the
        // resource allocation. Because Dawn's memory residency management occurs at the resource
        // heap granularity, every directly allocated ResourceHeapAllocation also stores a Heap
        // object. This object is created manually, and must be deleted manually upon deallocation
        // of the committed resource.
        Heap* heap = new Heap(committedResource, GetMemorySegment(mDevice, heapType),
                              resourceInfo.SizeInBytes);

        // Calling CreateCommittedResource implicitly calls MakeResident on the resource. We must
        // track this to avoid calling MakeResident a second time.
        mDevice->GetResidencyManager()->TrackResidentAllocation(heap);

        AllocationInfo info;
        info.mMethod = AllocationMethod::kDirect;

        return ResourceHeapAllocation{info,
                                      /*offset*/ 0, std::move(committedResource), heap};
    }

}}  // namespace dawn_native::d3d12
