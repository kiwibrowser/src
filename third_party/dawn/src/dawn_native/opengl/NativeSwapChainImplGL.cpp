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

#include "dawn_native/opengl/NativeSwapChainImplGL.h"

#include "dawn_native/opengl/DeviceGL.h"

namespace dawn_native { namespace opengl {

    NativeSwapChainImpl::NativeSwapChainImpl(Device* device,
                                             PresentCallback present,
                                             void* presentUserdata)
        : mPresentCallback(present), mPresentUserdata(presentUserdata), mDevice(device) {
    }

    NativeSwapChainImpl::~NativeSwapChainImpl() {
        const OpenGLFunctions& gl = mDevice->gl;
        gl.DeleteTextures(1, &mBackTexture);
        gl.DeleteFramebuffers(1, &mBackFBO);
    }

    void NativeSwapChainImpl::Init(DawnWSIContextGL* /*context*/) {
        const OpenGLFunctions& gl = mDevice->gl;
        gl.GenTextures(1, &mBackTexture);
        gl.BindTexture(GL_TEXTURE_2D, mBackTexture);
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        gl.GenFramebuffers(1, &mBackFBO);
        gl.BindFramebuffer(GL_READ_FRAMEBUFFER, mBackFBO);
        gl.FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                mBackTexture, 0);
    }

    DawnSwapChainError NativeSwapChainImpl::Configure(WGPUTextureFormat format,
                                                      WGPUTextureUsage usage,
                                                      uint32_t width,
                                                      uint32_t height) {
        if (format != WGPUTextureFormat_RGBA8Unorm) {
            return "unsupported format";
        }
        ASSERT(width > 0);
        ASSERT(height > 0);
        mWidth = width;
        mHeight = height;

        const OpenGLFunctions& gl = mDevice->gl;
        gl.BindTexture(GL_TEXTURE_2D, mBackTexture);
        // Reallocate the texture
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                      nullptr);

        return DAWN_SWAP_CHAIN_NO_ERROR;
    }

    DawnSwapChainError NativeSwapChainImpl::GetNextTexture(DawnSwapChainNextTexture* nextTexture) {
        nextTexture->texture.u32 = mBackTexture;
        return DAWN_SWAP_CHAIN_NO_ERROR;
    }

    DawnSwapChainError NativeSwapChainImpl::Present() {
        const OpenGLFunctions& gl = mDevice->gl;
        gl.BindFramebuffer(GL_READ_FRAMEBUFFER, mBackFBO);
        gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        gl.BlitFramebuffer(0, 0, mWidth, mHeight, 0, mHeight, mWidth, 0, GL_COLOR_BUFFER_BIT,
                           GL_NEAREST);

        mPresentCallback(mPresentUserdata);

        return DAWN_SWAP_CHAIN_NO_ERROR;
    }

    wgpu::TextureFormat NativeSwapChainImpl::GetPreferredFormat() const {
        return wgpu::TextureFormat::RGBA8Unorm;
    }

}}  // namespace dawn_native::opengl
