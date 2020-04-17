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

#ifndef DAWNNATIVE_D3D12_TEXTURED3D12_H_
#define DAWNNATIVE_D3D12_TEXTURED3D12_H_

#include "dawn_native/Texture.h"

#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    class Device;

    DXGI_FORMAT D3D12TextureFormat(dawn::TextureFormat format);

    class Texture : public TextureBase {
      public:
        Texture(Device* device, const TextureDescriptor* descriptor);
        Texture(Device* device, const TextureDescriptor* descriptor, ID3D12Resource* nativeTexture);
        ~Texture();

        bool CreateD3D12ResourceBarrierIfNeeded(D3D12_RESOURCE_BARRIER* barrier,
                                                dawn::TextureUsageBit newUsage) const;
        bool CreateD3D12ResourceBarrierIfNeeded(D3D12_RESOURCE_BARRIER* barrier,
                                                D3D12_RESOURCE_STATES newState) const;
        DXGI_FORMAT GetD3D12Format() const;
        ID3D12Resource* GetD3D12Resource() const;
        void SetUsage(dawn::TextureUsageBit newUsage);
        void TransitionUsageNow(ComPtr<ID3D12GraphicsCommandList> commandList,
                                dawn::TextureUsageBit usage);
        void TransitionUsageNow(ComPtr<ID3D12GraphicsCommandList> commandList,
                                D3D12_RESOURCE_STATES newState);

        D3D12_RENDER_TARGET_VIEW_DESC GetRTVDescriptor(uint32_t mipSlice,
                                                       uint32_t arrayLayers,
                                                       uint32_t baseArrayLayer) const;

      private:
        // Dawn API
        void DestroyImpl() override;

        UINT16 GetDepthOrArraySize();

        ComPtr<ID3D12Resource> mResource = {};
        ID3D12Resource* mResourcePtr = nullptr;
        D3D12_RESOURCE_STATES mLastState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
    };

    class TextureView : public TextureViewBase {
      public:
        TextureView(TextureBase* texture, const TextureViewDescriptor* descriptor);

        DXGI_FORMAT GetD3D12Format() const;

        const D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRVDescriptor() const;
        D3D12_RENDER_TARGET_VIEW_DESC GetRTVDescriptor() const;
        D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDescriptor() const;

      private:
        D3D12_SHADER_RESOURCE_VIEW_DESC mSrvDesc;
    };
}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_TEXTURED3D12_H_
