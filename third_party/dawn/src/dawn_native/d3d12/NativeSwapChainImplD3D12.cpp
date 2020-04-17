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

#include "dawn_native/d3d12/NativeSwapChainImplD3D12.h"

#include "common/Assert.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/TextureD3D12.h"

namespace dawn_native { namespace d3d12 {

    namespace {
        DXGI_USAGE D3D12SwapChainBufferUsage(DawnTextureUsageBit allowedUsages) {
            DXGI_USAGE usage = DXGI_CPU_ACCESS_NONE;
            if (allowedUsages & DAWN_TEXTURE_USAGE_BIT_SAMPLED) {
                usage |= DXGI_USAGE_SHADER_INPUT;
            }
            if (allowedUsages & DAWN_TEXTURE_USAGE_BIT_STORAGE) {
                usage |= DXGI_USAGE_UNORDERED_ACCESS;
            }
            if (allowedUsages & DAWN_TEXTURE_USAGE_BIT_OUTPUT_ATTACHMENT) {
                usage |= DXGI_USAGE_RENDER_TARGET_OUTPUT;
            }
            return usage;
        }

        static constexpr unsigned int kFrameCount = 3;
    }  // anonymous namespace

    NativeSwapChainImpl::NativeSwapChainImpl(Device* device, HWND window)
        : mWindow(window), mDevice(device) {
    }

    NativeSwapChainImpl::~NativeSwapChainImpl() {
    }

    void NativeSwapChainImpl::Init(DawnWSIContextD3D12* /*context*/) {
    }

    DawnSwapChainError NativeSwapChainImpl::Configure(DawnTextureFormat format,
                                                      DawnTextureUsageBit usage,
                                                      uint32_t width,
                                                      uint32_t height) {
        ASSERT(width > 0);
        ASSERT(height > 0);
        ASSERT(format == static_cast<DawnTextureFormat>(GetPreferredFormat()));

        ComPtr<IDXGIFactory4> factory = mDevice->GetFactory();
        ComPtr<ID3D12CommandQueue> queue = mDevice->GetCommandQueue();

        // Create the D3D12 swapchain, assuming only two buffers for now
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = D3D12TextureFormat(GetPreferredFormat());
        swapChainDesc.BufferUsage = D3D12SwapChainBufferUsage(usage);
        swapChainDesc.BufferCount = kFrameCount;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;

        ComPtr<IDXGISwapChain1> swapChain1;
        ASSERT_SUCCESS(factory->CreateSwapChainForHwnd(queue.Get(), mWindow, &swapChainDesc,
                                                       nullptr, nullptr, &swapChain1));

        ASSERT_SUCCESS(swapChain1.As(&mSwapChain));

        // Gather the resources that will be used to present to the swapchain
        mBuffers.resize(kFrameCount);
        for (uint32_t i = 0; i < kFrameCount; ++i) {
            ASSERT_SUCCESS(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBuffers[i])));
        }

        // Set the initial serial of buffers to 0 so that we don't wait on them when they are first
        // used
        mBufferSerials.resize(kFrameCount, 0);

        return DAWN_SWAP_CHAIN_NO_ERROR;
    }

    DawnSwapChainError NativeSwapChainImpl::GetNextTexture(DawnSwapChainNextTexture* nextTexture) {
        mCurrentBuffer = mSwapChain->GetCurrentBackBufferIndex();
        nextTexture->texture.ptr = mBuffers[mCurrentBuffer].Get();

        // TODO(cwallez@chromium.org) Currently we force the CPU to wait for the GPU to be finished
        // with the buffer. Ideally the synchronization should be all done on the GPU.
        mDevice->WaitForSerial(mBufferSerials[mCurrentBuffer]);

        return DAWN_SWAP_CHAIN_NO_ERROR;
    }

    DawnSwapChainError NativeSwapChainImpl::Present() {
        // This assumes the texture has already been transition to the PRESENT state.

        ASSERT_SUCCESS(mSwapChain->Present(1, 0));
        // TODO(cwallez@chromium.org): Make the serial ticking implicit.
        mDevice->NextSerial();

        mBufferSerials[mCurrentBuffer] = mDevice->GetPendingCommandSerial();
        return DAWN_SWAP_CHAIN_NO_ERROR;
    }

    dawn::TextureFormat NativeSwapChainImpl::GetPreferredFormat() const {
        return dawn::TextureFormat::R8G8B8A8Unorm;
    }

}}  // namespace dawn_native::d3d12
