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

#ifndef DAWNNATIVE_D3D12_RENDERPASSBUILDERD3D12_H_
#define DAWNNATIVE_D3D12_RENDERPASSBUILDERD3D12_H_

#include "common/Constants.h"
#include "dawn_native/d3d12/d3d12_platform.h"
#include "dawn_native/dawn_platform.h"

#include <array>

namespace dawn_native { namespace d3d12 {

    class TextureView;

    // RenderPassBuilder stores parameters related to render pass load and store operations.
    // When the D3D12 render pass API is available, the needed descriptors can be fetched
    // directly from the RenderPassBuilder. When the D3D12 render pass API is not available, the
    // descriptors are still fetched and any information necessary to emulate the load and store
    // operations is extracted from the descriptors.
    class RenderPassBuilder {
      public:
        RenderPassBuilder(bool hasUAV);

        uint32_t GetColorAttachmentCount() const;

        // Returns descriptors that are fed directly to BeginRenderPass, or are used as parameter
        // storage if D3D12 render pass API is unavailable.
        const D3D12_RENDER_PASS_RENDER_TARGET_DESC* GetRenderPassRenderTargetDescriptors() const;
        const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* GetRenderPassDepthStencilDescriptor() const;

        D3D12_RENDER_PASS_FLAGS GetRenderPassFlags() const;

        // Returns attachment RTVs to use with OMSetRenderTargets.
        const D3D12_CPU_DESCRIPTOR_HANDLE* GetRenderTargetViews() const;

        bool HasDepth() const;

        // Functions that set the appropriate values in the render pass descriptors.
        void SetDepthAccess(wgpu::LoadOp loadOp,
                            wgpu::StoreOp storeOp,
                            float clearDepth,
                            DXGI_FORMAT format);
        void SetDepthNoAccess();
        void SetDepthStencilNoAccess();
        void SetRenderTargetBeginningAccess(uint32_t attachment,
                                            wgpu::LoadOp loadOp,
                                            dawn_native::Color clearColor,
                                            DXGI_FORMAT format);
        void SetRenderTargetEndingAccess(uint32_t attachment, wgpu::StoreOp storeOp);
        void SetRenderTargetEndingAccessResolve(uint32_t attachment,
                                                wgpu::StoreOp storeOp,
                                                TextureView* resolveSource,
                                                TextureView* resolveDestination);
        void SetStencilAccess(wgpu::LoadOp loadOp,
                              wgpu::StoreOp storeOp,
                              uint8_t clearStencil,
                              DXGI_FORMAT format);
        void SetStencilNoAccess();

        void SetRenderTargetView(uint32_t attachmentIndex,
                                 D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptor);
        void SetDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptor);

      private:
        uint32_t mColorAttachmentCount = 0;
        bool mHasDepth = false;
        D3D12_RENDER_PASS_FLAGS mRenderPassFlags = D3D12_RENDER_PASS_FLAG_NONE;
        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC mRenderPassDepthStencilDesc;
        std::array<D3D12_RENDER_PASS_RENDER_TARGET_DESC, kMaxColorAttachments>
            mRenderPassRenderTargetDescriptors;
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxColorAttachments> mRenderTargetViews;
        std::array<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS,
                   kMaxColorAttachments>
            mSubresourceParams;
    };
}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_RENDERPASSBUILDERD3D12_H_
