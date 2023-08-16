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

#include "dawn_native/d3d12/SwapChainD3D12.h"

#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/TextureD3D12.h"

#include <dawn/dawn_wsi.h>

namespace dawn_native { namespace d3d12 {

    SwapChain::SwapChain(Device* device, const SwapChainDescriptor* descriptor)
        : OldSwapChainBase(device, descriptor) {
        const auto& im = GetImplementation();
        DawnWSIContextD3D12 wsiContext = {};
        wsiContext.device = reinterpret_cast<WGPUDevice>(GetDevice());
        im.Init(im.userData, &wsiContext);

        ASSERT(im.textureUsage != WGPUTextureUsage_None);
        mTextureUsage = static_cast<wgpu::TextureUsage>(im.textureUsage);
    }

    SwapChain::~SwapChain() {
    }

    TextureBase* SwapChain::GetNextTextureImpl(const TextureDescriptor* descriptor) {
        const auto& im = GetImplementation();
        DawnSwapChainNextTexture next = {};
        DawnSwapChainError error = im.GetNextTexture(im.userData, &next);
        if (error) {
            GetDevice()->HandleError(InternalErrorType::Internal, error);
            return nullptr;
        }

        ComPtr<ID3D12Resource> d3d12Texture = static_cast<ID3D12Resource*>(next.texture.ptr);
        return new Texture(ToBackend(GetDevice()), descriptor, std::move(d3d12Texture));
    }

    MaybeError SwapChain::OnBeforePresent(TextureViewBase* view) {
        Device* device = ToBackend(GetDevice());

        CommandRecordingContext* commandContext;
        DAWN_TRY_ASSIGN(commandContext, device->GetPendingCommandContext());

        // Perform the necessary transition for the texture to be presented.
        ToBackend(view->GetTexture())
            ->TrackUsageAndTransitionNow(commandContext, mTextureUsage,
                                         view->GetSubresourceRange());

        DAWN_TRY(device->ExecutePendingCommandContext());

        return {};
    }

}}  // namespace dawn_native::d3d12
