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

#include "dawn_native/d3d12/RenderPassBuilderD3D12.h"

#include "dawn_native/Format.h"
#include "dawn_native/d3d12/CommandBufferD3D12.h"
#include "dawn_native/d3d12/Forward.h"
#include "dawn_native/d3d12/TextureD3D12.h"

#include "dawn_native/dawn_platform.h"

namespace dawn_native { namespace d3d12 {

    namespace {
        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE D3D12BeginningAccessType(wgpu::LoadOp loadOp) {
            switch (loadOp) {
                case wgpu::LoadOp::Clear:
                    return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                case wgpu::LoadOp::Load:
                    return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE D3D12EndingAccessType(wgpu::StoreOp storeOp) {
            switch (storeOp) {
                case wgpu::StoreOp::Clear:
                    return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                case wgpu::StoreOp::Store:
                    return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                default:
                    UNREACHABLE();
            }
        }

        D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_PARAMETERS D3D12EndingAccessResolveParameters(
            wgpu::StoreOp storeOp,
            TextureView* resolveSource,
            TextureView* resolveDestination) {
            D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_PARAMETERS resolveParameters;

            resolveParameters.Format = resolveDestination->GetD3D12Format();
            resolveParameters.pSrcResource =
                ToBackend(resolveSource->GetTexture())->GetD3D12Resource();
            resolveParameters.pDstResource =
                ToBackend(resolveDestination->GetTexture())->GetD3D12Resource();

            // Clear or preserve the resolve source.
            if (storeOp == wgpu::StoreOp::Clear) {
                resolveParameters.PreserveResolveSource = false;
            } else if (storeOp == wgpu::StoreOp::Store) {
                resolveParameters.PreserveResolveSource = true;
            }

            // RESOLVE_MODE_AVERAGE is only valid for non-integer formats.
            // TODO: Investigate and determine how integer format resolves should work in WebGPU.
            switch (resolveDestination->GetFormat().type) {
                case Format::Type::Sint:
                case Format::Type::Uint:
                    resolveParameters.ResolveMode = D3D12_RESOLVE_MODE_MAX;
                    break;
                default:
                    resolveParameters.ResolveMode = D3D12_RESOLVE_MODE_AVERAGE;
                    break;
            }

            resolveParameters.SubresourceCount = 1;

            return resolveParameters;
        }

        D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS
        D3D12EndingAccessResolveSubresourceParameters(TextureView* resolveDestination) {
            D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS subresourceParameters;
            Texture* resolveDestinationTexture = ToBackend(resolveDestination->GetTexture());

            subresourceParameters.DstX = 0;
            subresourceParameters.DstY = 0;
            subresourceParameters.SrcSubresource = 0;
            subresourceParameters.DstSubresource = resolveDestinationTexture->GetSubresourceIndex(
                resolveDestination->GetBaseMipLevel(), resolveDestination->GetBaseArrayLayer());
            // Resolving a specified sub-rect is only valid on hardware that supports sample
            // positions. This means even {0, 0, width, height} would be invalid if unsupported. To
            // avoid this, we assume sub-rect resolves never work by setting them to all zeros or
            // "empty" to resolve the entire region.
            subresourceParameters.SrcRect = {0, 0, 0, 0};

            return subresourceParameters;
        }
    }  // anonymous namespace

    RenderPassBuilder::RenderPassBuilder(bool hasUAV) {
        if (hasUAV) {
            mRenderPassFlags = D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
        }
    }

    void RenderPassBuilder::SetRenderTargetView(uint32_t attachmentIndex,
                                                D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptor) {
        ASSERT(mColorAttachmentCount < kMaxColorAttachments);
        mRenderTargetViews[attachmentIndex] = baseDescriptor;
        mRenderPassRenderTargetDescriptors[attachmentIndex].cpuDescriptor = baseDescriptor;
        mColorAttachmentCount++;
    }

    void RenderPassBuilder::SetDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptor) {
        mRenderPassDepthStencilDesc.cpuDescriptor = baseDescriptor;
    }

    uint32_t RenderPassBuilder::GetColorAttachmentCount() const {
        return mColorAttachmentCount;
    }

    bool RenderPassBuilder::HasDepth() const {
        return mHasDepth;
    }

    const D3D12_RENDER_PASS_RENDER_TARGET_DESC*
    RenderPassBuilder::GetRenderPassRenderTargetDescriptors() const {
        return mRenderPassRenderTargetDescriptors.data();
    }

    const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC*
    RenderPassBuilder::GetRenderPassDepthStencilDescriptor() const {
        return &mRenderPassDepthStencilDesc;
    }

    D3D12_RENDER_PASS_FLAGS RenderPassBuilder::GetRenderPassFlags() const {
        return mRenderPassFlags;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE* RenderPassBuilder::GetRenderTargetViews() const {
        return mRenderTargetViews.data();
    }

    void RenderPassBuilder::SetRenderTargetBeginningAccess(uint32_t attachment,
                                                           wgpu::LoadOp loadOp,
                                                           dawn_native::Color clearColor,
                                                           DXGI_FORMAT format) {
        mRenderPassRenderTargetDescriptors[attachment].BeginningAccess.Type =
            D3D12BeginningAccessType(loadOp);
        if (loadOp == wgpu::LoadOp::Clear) {
            mRenderPassRenderTargetDescriptors[attachment]
                .BeginningAccess.Clear.ClearValue.Color[0] = clearColor.r;
            mRenderPassRenderTargetDescriptors[attachment]
                .BeginningAccess.Clear.ClearValue.Color[1] = clearColor.g;
            mRenderPassRenderTargetDescriptors[attachment]
                .BeginningAccess.Clear.ClearValue.Color[2] = clearColor.b;
            mRenderPassRenderTargetDescriptors[attachment]
                .BeginningAccess.Clear.ClearValue.Color[3] = clearColor.a;
            mRenderPassRenderTargetDescriptors[attachment].BeginningAccess.Clear.ClearValue.Format =
                format;
        }
    }

    void RenderPassBuilder::SetRenderTargetEndingAccess(uint32_t attachment,
                                                        wgpu::StoreOp storeOp) {
        mRenderPassRenderTargetDescriptors[attachment].EndingAccess.Type =
            D3D12EndingAccessType(storeOp);
    }

    void RenderPassBuilder::SetRenderTargetEndingAccessResolve(uint32_t attachment,
                                                               wgpu::StoreOp storeOp,
                                                               TextureView* resolveSource,
                                                               TextureView* resolveDestination) {
        mRenderPassRenderTargetDescriptors[attachment].EndingAccess.Type =
            D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
        mRenderPassRenderTargetDescriptors[attachment].EndingAccess.Resolve =
            D3D12EndingAccessResolveParameters(storeOp, resolveSource, resolveDestination);

        mSubresourceParams[attachment] =
            D3D12EndingAccessResolveSubresourceParameters(resolveDestination);

        mRenderPassRenderTargetDescriptors[attachment].EndingAccess.Resolve.pSubresourceParameters =
            &mSubresourceParams[attachment];
    }

    void RenderPassBuilder::SetDepthAccess(wgpu::LoadOp loadOp,
                                           wgpu::StoreOp storeOp,
                                           float clearDepth,
                                           DXGI_FORMAT format) {
        mHasDepth = true;
        mRenderPassDepthStencilDesc.DepthBeginningAccess.Type = D3D12BeginningAccessType(loadOp);
        if (loadOp == wgpu::LoadOp::Clear) {
            mRenderPassDepthStencilDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth =
                clearDepth;
            mRenderPassDepthStencilDesc.DepthBeginningAccess.Clear.ClearValue.Format = format;
        }
        mRenderPassDepthStencilDesc.DepthEndingAccess.Type = D3D12EndingAccessType(storeOp);
    }

    void RenderPassBuilder::SetStencilAccess(wgpu::LoadOp loadOp,
                                             wgpu::StoreOp storeOp,
                                             uint8_t clearStencil,
                                             DXGI_FORMAT format) {
        mRenderPassDepthStencilDesc.StencilBeginningAccess.Type = D3D12BeginningAccessType(loadOp);
        if (loadOp == wgpu::LoadOp::Clear) {
            mRenderPassDepthStencilDesc.StencilBeginningAccess.Clear.ClearValue.DepthStencil
                .Stencil = clearStencil;
            mRenderPassDepthStencilDesc.StencilBeginningAccess.Clear.ClearValue.Format = format;
        }
        mRenderPassDepthStencilDesc.StencilEndingAccess.Type = D3D12EndingAccessType(storeOp);
    }

    void RenderPassBuilder::SetDepthNoAccess() {
        mRenderPassDepthStencilDesc.DepthBeginningAccess.Type =
            D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
        mRenderPassDepthStencilDesc.DepthEndingAccess.Type =
            D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
    }

    void RenderPassBuilder::SetDepthStencilNoAccess() {
        SetDepthNoAccess();
        SetStencilNoAccess();
    }

    void RenderPassBuilder::SetStencilNoAccess() {
        mRenderPassDepthStencilDesc.StencilBeginningAccess.Type =
            D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
        mRenderPassDepthStencilDesc.StencilEndingAccess.Type =
            D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
    }

}}  // namespace dawn_native::d3d12
