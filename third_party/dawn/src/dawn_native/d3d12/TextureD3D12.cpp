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

#include "dawn_native/d3d12/TextureD3D12.h"

#include "dawn_native/d3d12/DescriptorHeapAllocator.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/ResourceAllocator.h"

namespace dawn_native { namespace d3d12 {

    namespace {
        D3D12_RESOURCE_STATES D3D12TextureUsage(dawn::TextureUsageBit usage,
                                                dawn::TextureFormat format) {
            D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

            // Present is an exclusive flag.
            if (usage & dawn::TextureUsageBit::Present) {
                return D3D12_RESOURCE_STATE_PRESENT;
            }

            if (usage & dawn::TextureUsageBit::TransferSrc) {
                resourceState |= D3D12_RESOURCE_STATE_COPY_SOURCE;
            }
            if (usage & dawn::TextureUsageBit::TransferDst) {
                resourceState |= D3D12_RESOURCE_STATE_COPY_DEST;
            }
            if (usage & dawn::TextureUsageBit::Sampled) {
                resourceState |= (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            }
            if (usage & dawn::TextureUsageBit::Storage) {
                resourceState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }
            if (usage & dawn::TextureUsageBit::OutputAttachment) {
                if (TextureFormatHasDepth(format) || TextureFormatHasStencil(format)) {
                    resourceState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
                } else {
                    resourceState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
                }
            }

            return resourceState;
        }

        D3D12_RESOURCE_FLAGS D3D12ResourceFlags(dawn::TextureUsageBit usage,
                                                dawn::TextureFormat format,
                                                bool isMultisampledTexture) {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

            if (usage & dawn::TextureUsageBit::Storage) {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            // A multisampled resource must have either D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET or
            // D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL set in D3D12_RESOURCE_DESC::Flags.
            // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_resource
            // _desc
            if ((usage & dawn::TextureUsageBit::OutputAttachment) || isMultisampledTexture) {
                if (TextureFormatHasDepth(format) || TextureFormatHasStencil(format)) {
                    flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                } else {
                    flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                }
            }

            ASSERT(!(flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ||
                   flags == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            return flags;
        }

        D3D12_RESOURCE_DIMENSION D3D12TextureDimension(dawn::TextureDimension dimension) {
            switch (dimension) {
                case dawn::TextureDimension::e2D:
                    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                default:
                    UNREACHABLE();
            }
        }

    }  // namespace

    DXGI_FORMAT D3D12TextureFormat(dawn::TextureFormat format) {
        switch (format) {
            case dawn::TextureFormat::R8G8B8A8Unorm:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case dawn::TextureFormat::R8G8Unorm:
                return DXGI_FORMAT_R8G8_UNORM;
            case dawn::TextureFormat::R8Unorm:
                return DXGI_FORMAT_R8_UNORM;
            case dawn::TextureFormat::R8G8B8A8Uint:
                return DXGI_FORMAT_R8G8B8A8_UINT;
            case dawn::TextureFormat::R8G8Uint:
                return DXGI_FORMAT_R8G8_UINT;
            case dawn::TextureFormat::R8Uint:
                return DXGI_FORMAT_R8_UINT;
            case dawn::TextureFormat::B8G8R8A8Unorm:
                return DXGI_FORMAT_B8G8R8A8_UNORM;
            case dawn::TextureFormat::D32FloatS8Uint:
                return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            default:
                UNREACHABLE();
        }
    }

    Texture::Texture(Device* device, const TextureDescriptor* descriptor)
        : TextureBase(device, descriptor, TextureState::OwnedInternal) {
        D3D12_RESOURCE_DESC resourceDescriptor;
        resourceDescriptor.Dimension = D3D12TextureDimension(GetDimension());
        resourceDescriptor.Alignment = 0;

        const Extent3D& size = GetSize();
        resourceDescriptor.Width = size.width;
        resourceDescriptor.Height = size.height;

        resourceDescriptor.DepthOrArraySize = GetDepthOrArraySize();
        resourceDescriptor.MipLevels = static_cast<UINT16>(GetNumMipLevels());
        resourceDescriptor.Format = D3D12TextureFormat(GetFormat());
        resourceDescriptor.SampleDesc.Count = descriptor->sampleCount;
        // TODO(bryan.bernhart@intel.com): investigate how to specify standard MSAA sample pattern.
        resourceDescriptor.SampleDesc.Quality = 0;
        resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDescriptor.Flags =
            D3D12ResourceFlags(GetUsage(), GetFormat(), IsMultisampledTexture());

        mResource = ToBackend(GetDevice())
                        ->GetResourceAllocator()
                        ->Allocate(D3D12_HEAP_TYPE_DEFAULT, resourceDescriptor,
                                   D3D12_RESOURCE_STATE_COMMON);
        mResourcePtr = mResource.Get();

        if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting)) {
            TransitionUsageNow(device->GetPendingCommandList(), D3D12_RESOURCE_STATE_RENDER_TARGET);
            uint32_t arrayLayerCount = GetArrayLayers();

            DescriptorHeapAllocator* descriptorHeapAllocator = device->GetDescriptorHeapAllocator();
            DescriptorHeapHandle rtvHeap =
                descriptorHeapAllocator->AllocateCPUHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap.GetCPUHandle(0);

            const float clearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            // TODO(natlee@microsoft.com): clear all array layers for 2D array textures
            for (int i = 0; i < resourceDescriptor.MipLevels; i++) {
                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = GetRTVDescriptor(i, arrayLayerCount, 0);
                device->GetD3D12Device()->CreateRenderTargetView(mResourcePtr, &rtvDesc, rtvHandle);
                device->GetPendingCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0,
                                                                       nullptr);
            }
        }
    }

    // With this constructor, the lifetime of the ID3D12Resource is externally managed.
    Texture::Texture(Device* device,
                     const TextureDescriptor* descriptor,
                     ID3D12Resource* nativeTexture)
        : TextureBase(device, descriptor, TextureState::OwnedExternal),
          mResourcePtr(nativeTexture) {
    }

    Texture::~Texture() {
        DestroyInternal();
    }

    bool Texture::CreateD3D12ResourceBarrierIfNeeded(D3D12_RESOURCE_BARRIER* barrier,
                                                     dawn::TextureUsageBit newUsage) const {
        return CreateD3D12ResourceBarrierIfNeeded(barrier,
                                                  D3D12TextureUsage(newUsage, GetFormat()));
    }

    bool Texture::CreateD3D12ResourceBarrierIfNeeded(D3D12_RESOURCE_BARRIER* barrier,
                                                     D3D12_RESOURCE_STATES newState) const {
        // Avoid transitioning the texture when it isn't needed.
        // TODO(cwallez@chromium.org): Need some form of UAV barriers at some point.
        if (mLastState == newState) {
            return false;
        }

        barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier->Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier->Transition.pResource = mResourcePtr;
        barrier->Transition.StateBefore = mLastState;
        barrier->Transition.StateAfter = newState;
        barrier->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        return true;
    }

    void Texture::DestroyImpl() {
        // If we own the resource, release it.
        ToBackend(GetDevice())->GetResourceAllocator()->Release(mResource);
        mResource = nullptr;
    }

    DXGI_FORMAT Texture::GetD3D12Format() const {
        return D3D12TextureFormat(GetFormat());
    }

    ID3D12Resource* Texture::GetD3D12Resource() const {
        return mResourcePtr;
    }

    UINT16 Texture::GetDepthOrArraySize() {
        switch (GetDimension()) {
            case dawn::TextureDimension::e2D:
                return static_cast<UINT16>(GetArrayLayers());
            default:
                UNREACHABLE();
        }
    }

    void Texture::SetUsage(dawn::TextureUsageBit newUsage) {
        mLastState = D3D12TextureUsage(newUsage, GetFormat());
    }

    void Texture::TransitionUsageNow(ComPtr<ID3D12GraphicsCommandList> commandList,
                                     dawn::TextureUsageBit usage) {
        TransitionUsageNow(commandList, D3D12TextureUsage(usage, GetFormat()));
    }

    void Texture::TransitionUsageNow(ComPtr<ID3D12GraphicsCommandList> commandList,
                                     D3D12_RESOURCE_STATES newState) {
        D3D12_RESOURCE_BARRIER barrier;
        if (CreateD3D12ResourceBarrierIfNeeded(&barrier, newState)) {
            commandList->ResourceBarrier(1, &barrier);
        }

        mLastState = newState;
    }

    D3D12_RENDER_TARGET_VIEW_DESC Texture::GetRTVDescriptor(uint32_t mipSlice,
                                                            uint32_t arrayLayers,
                                                            uint32_t baseArrayLayer) const {
        ASSERT(GetDimension() == dawn::TextureDimension::e2D);
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.Format = GetD3D12Format();
        if (IsMultisampledTexture()) {
            ASSERT(GetNumMipLevels() == 1);
            ASSERT(arrayLayers == 1);
            ASSERT(baseArrayLayer == 0);
            ASSERT(mipSlice == 0);
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        } else {
            // Currently we always use D3D12_TEX2D_ARRAY_RTV because we cannot specify base array
            // layer and layer count in D3D12_TEX2D_RTV. For 2D texture views, we treat them as
            // 1-layer 2D array textures. (Just like how we treat SRVs)
            // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_tex2d_rtv
            // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_tex2d_array
            // _rtv
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.FirstArraySlice = baseArrayLayer;
            rtvDesc.Texture2DArray.ArraySize = arrayLayers;
            rtvDesc.Texture2DArray.MipSlice = mipSlice;
            rtvDesc.Texture2DArray.PlaneSlice = 0;
        }
        return rtvDesc;
    }

    TextureView::TextureView(TextureBase* texture, const TextureViewDescriptor* descriptor)
        : TextureViewBase(texture, descriptor) {
        mSrvDesc.Format = D3D12TextureFormat(descriptor->format);
        mSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        // Currently we always use D3D12_TEX2D_ARRAY_SRV because we cannot specify base array layer
        // and layer count in D3D12_TEX2D_SRV. For 2D texture views, we treat them as 1-layer 2D
        // array textures.
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_tex2d_srv
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ns-d3d12-d3d12_tex2d_array_srv
        // TODO(jiawei.shao@intel.com): support more texture view dimensions.
        // TODO(jiawei.shao@intel.com): support creating SRV on multisampled textures.
        switch (descriptor->dimension) {
            case dawn::TextureViewDimension::e2D:
            case dawn::TextureViewDimension::e2DArray:
                ASSERT(texture->GetDimension() == dawn::TextureDimension::e2D);
                mSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                mSrvDesc.Texture2DArray.ArraySize = descriptor->arrayLayerCount;
                mSrvDesc.Texture2DArray.FirstArraySlice = descriptor->baseArrayLayer;
                mSrvDesc.Texture2DArray.MipLevels = descriptor->mipLevelCount;
                mSrvDesc.Texture2DArray.MostDetailedMip = descriptor->baseMipLevel;
                mSrvDesc.Texture2DArray.PlaneSlice = 0;
                mSrvDesc.Texture2DArray.ResourceMinLODClamp = 0;
                break;
            case dawn::TextureViewDimension::Cube:
            case dawn::TextureViewDimension::CubeArray:
                ASSERT(texture->GetDimension() == dawn::TextureDimension::e2D);
                ASSERT(descriptor->arrayLayerCount % 6 == 0);
                mSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                mSrvDesc.TextureCubeArray.First2DArrayFace = descriptor->baseArrayLayer;
                mSrvDesc.TextureCubeArray.NumCubes = descriptor->arrayLayerCount / 6;
                mSrvDesc.TextureCubeArray.MostDetailedMip = descriptor->baseMipLevel;
                mSrvDesc.TextureCubeArray.MipLevels = descriptor->mipLevelCount;
                mSrvDesc.TextureCubeArray.ResourceMinLODClamp = 0;
                break;
            default:
                UNREACHABLE();
        }
    }

    DXGI_FORMAT TextureView::GetD3D12Format() const {
        return D3D12TextureFormat(GetFormat());
    }

    const D3D12_SHADER_RESOURCE_VIEW_DESC& TextureView::GetSRVDescriptor() const {
        return mSrvDesc;
    }

    D3D12_RENDER_TARGET_VIEW_DESC TextureView::GetRTVDescriptor() const {
        return ToBackend(GetTexture())
            ->GetRTVDescriptor(GetBaseMipLevel(), GetLayerCount(), GetBaseArrayLayer());
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC TextureView::GetDSVDescriptor() const {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        dsvDesc.Format = ToBackend(GetTexture())->GetD3D12Format();
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        // TODO(jiawei.shao@intel.com): support rendering into a layer of a texture.
        ASSERT(GetTexture()->GetArrayLayers() == 1 && GetTexture()->GetNumMipLevels() == 1 &&
               GetBaseArrayLayer() == 0 && GetBaseMipLevel() == 0);

        if (GetTexture()->IsMultisampledTexture()) {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        } else {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;
        }

        return dsvDesc;
    }

}}  // namespace dawn_native::d3d12
