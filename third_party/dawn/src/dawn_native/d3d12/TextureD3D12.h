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

#include "common/Serial.h"
#include "dawn_native/Texture.h"

#include "dawn_native/DawnNative.h"
#include "dawn_native/PassResourceUsage.h"
#include "dawn_native/d3d12/ResourceHeapAllocationD3D12.h"
#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    class CommandRecordingContext;
    class Device;

    DXGI_FORMAT D3D12TextureFormat(wgpu::TextureFormat format);
    MaybeError ValidateD3D12TextureCanBeWrapped(ID3D12Resource* d3d12Resource,
                                                const TextureDescriptor* descriptor);
    MaybeError ValidateTextureDescriptorCanBeWrapped(const TextureDescriptor* descriptor);

    class Texture final : public TextureBase {
      public:
        static ResultOrError<Ref<TextureBase>> Create(Device* device,
                                                      const TextureDescriptor* descriptor);
        static ResultOrError<Ref<TextureBase>> Create(Device* device,
                                                      const ExternalImageDescriptor* descriptor,
                                                      HANDLE sharedHandle,
                                                      uint64_t acquireMutexKey,
                                                      bool isSwapChainTexture);
        Texture(Device* device,
                const TextureDescriptor* descriptor,
                ComPtr<ID3D12Resource> d3d12Texture);

        DXGI_FORMAT GetD3D12Format() const;
        ID3D12Resource* GetD3D12Resource() const;

        D3D12_RENDER_TARGET_VIEW_DESC GetRTVDescriptor(uint32_t mipLevel,
                                                       uint32_t baseArrayLayer,
                                                       uint32_t layerCount) const;
        D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDescriptor(uint32_t mipLevel,
                                                       uint32_t baseArrayLayer,
                                                       uint32_t layerCount) const;
        void EnsureSubresourceContentInitialized(CommandRecordingContext* commandContext,
                                                 const SubresourceRange& range);

        void TrackUsageAndGetResourceBarrierForPass(CommandRecordingContext* commandContext,
                                                    std::vector<D3D12_RESOURCE_BARRIER>* barrier,
                                                    const PassTextureUsage& textureUsages);
        void TrackUsageAndTransitionNow(CommandRecordingContext* commandContext,
                                        wgpu::TextureUsage usage,
                                        const SubresourceRange& range);
        void TrackUsageAndTransitionNow(CommandRecordingContext* commandContext,
                                        D3D12_RESOURCE_STATES newState,
                                        const SubresourceRange& range);
        void TrackAllUsageAndTransitionNow(CommandRecordingContext* commandContext,
                                           wgpu::TextureUsage usage);
        void TrackAllUsageAndTransitionNow(CommandRecordingContext* commandContext,
                                           D3D12_RESOURCE_STATES newState);

      private:
        Texture(Device* device, const TextureDescriptor* descriptor, TextureState state);
        ~Texture() override;
        using TextureBase::TextureBase;

        MaybeError InitializeAsInternalTexture();
        MaybeError InitializeAsExternalTexture(const TextureDescriptor* descriptor,
                                               HANDLE sharedHandle,
                                               uint64_t acquireMutexKey,
                                               bool isSwapChainTexture);

        // Dawn API
        void DestroyImpl() override;
        MaybeError ClearTexture(CommandRecordingContext* commandContext,
                                const SubresourceRange& range,
                                TextureBase::ClearValue clearValue);

        void TransitionUsageAndGetResourceBarrier(CommandRecordingContext* commandContext,
                                                  std::vector<D3D12_RESOURCE_BARRIER>* barrier,
                                                  D3D12_RESOURCE_STATES newState,
                                                  const SubresourceRange& range);

        void TransitionSingleOrAllSubresources(std::vector<D3D12_RESOURCE_BARRIER>* barriers,
                                               uint32_t index,
                                               D3D12_RESOURCE_STATES subresourceNewState,
                                               const Serial pendingCommandSerial,
                                               bool allSubresources);
        void HandleTransitionSpecialCases(CommandRecordingContext* commandContext);

        bool mSameLastUsagesAcrossSubresources = true;

        struct StateAndDecay {
            D3D12_RESOURCE_STATES lastState;
            Serial lastDecaySerial;
            bool isValidToDecay;
        };
        std::vector<StateAndDecay> mSubresourceStateAndDecay;

        ResourceHeapAllocation mResourceAllocation;
        bool mSwapChainTexture = false;

        Serial mAcquireMutexKey = 0;
        ComPtr<IDXGIKeyedMutex> mDxgiKeyedMutex;
    };

    class TextureView final : public TextureViewBase {
      public:
        TextureView(TextureBase* texture, const TextureViewDescriptor* descriptor);

        DXGI_FORMAT GetD3D12Format() const;

        const D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRVDescriptor() const;
        D3D12_RENDER_TARGET_VIEW_DESC GetRTVDescriptor() const;
        D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDescriptor() const;
        D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDescriptor() const;

      private:
        D3D12_SHADER_RESOURCE_VIEW_DESC mSrvDesc;
    };
}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_TEXTURED3D12_H_
