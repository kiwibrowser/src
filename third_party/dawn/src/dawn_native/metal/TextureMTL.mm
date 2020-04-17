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

#include "dawn_native/metal/TextureMTL.h"

#include "dawn_native/metal/DeviceMTL.h"

#include <IOSurface/IOSurface.h>

namespace dawn_native { namespace metal {

    namespace {
        bool UsageNeedsTextureView(dawn::TextureUsageBit usage) {
            constexpr dawn::TextureUsageBit kUsageNeedsTextureView =
                dawn::TextureUsageBit::Storage | dawn::TextureUsageBit::Sampled;
            return usage & kUsageNeedsTextureView;
        }

        MTLTextureUsage MetalTextureUsage(dawn::TextureUsageBit usage) {
            MTLTextureUsage result = MTLTextureUsageUnknown;  // This is 0

            if (usage & (dawn::TextureUsageBit::Storage)) {
                result |= MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead;
            }

            if (usage & (dawn::TextureUsageBit::Sampled)) {
                result |= MTLTextureUsageShaderRead;
            }

            if (usage & (dawn::TextureUsageBit::OutputAttachment)) {
                result |= MTLTextureUsageRenderTarget;
            }

            if (UsageNeedsTextureView(usage)) {
                result |= MTLTextureUsagePixelFormatView;
            }

            return result;
        }

        MTLTextureType MetalTextureType(dawn::TextureDimension dimension,
                                        unsigned int arrayLayers,
                                        unsigned int sampleCount) {
            switch (dimension) {
                case dawn::TextureDimension::e2D:
                    if (sampleCount > 1) {
                        ASSERT(arrayLayers == 1);
                        return MTLTextureType2DMultisample;
                    } else {
                        return (arrayLayers > 1) ? MTLTextureType2DArray : MTLTextureType2D;
                    }
            }
        }

        MTLTextureType MetalTextureViewType(dawn::TextureViewDimension dimension,
                                            unsigned int sampleCount) {
            switch (dimension) {
                case dawn::TextureViewDimension::e2D:
                    return (sampleCount > 1) ? MTLTextureType2DMultisample : MTLTextureType2D;
                case dawn::TextureViewDimension::e2DArray:
                    return MTLTextureType2DArray;
                case dawn::TextureViewDimension::Cube:
                    return MTLTextureTypeCube;
                case dawn::TextureViewDimension::CubeArray:
                    return MTLTextureTypeCubeArray;
                default:
                    UNREACHABLE();
                    return MTLTextureType2D;
            }
        }

        bool RequiresCreatingNewTextureView(const TextureBase* texture,
                                            const TextureViewDescriptor* textureViewDescriptor) {
            if (texture->GetFormat() != textureViewDescriptor->format) {
                return true;
            }

            if (texture->GetArrayLayers() != textureViewDescriptor->arrayLayerCount) {
                return true;
            }

            if (texture->GetNumMipLevels() != textureViewDescriptor->mipLevelCount) {
                return true;
            }

            switch (textureViewDescriptor->dimension) {
                case dawn::TextureViewDimension::Cube:
                case dawn::TextureViewDimension::CubeArray:
                    return true;
                default:
                    break;
            }

            return false;
        }

        ResultOrError<dawn::TextureFormat> GetFormatEquivalentToIOSurfaceFormat(uint32_t format) {
            switch (format) {
                case 'BGRA':
                    return dawn::TextureFormat::B8G8R8A8Unorm;
                case '2C08':
                    return dawn::TextureFormat::R8G8Unorm;
                case 'L008':
                    return dawn::TextureFormat::R8Unorm;
                default:
                    return DAWN_VALIDATION_ERROR("Unsupported IOSurface format");
            }
        }
    }

    MTLPixelFormat MetalPixelFormat(dawn::TextureFormat format) {
        switch (format) {
            case dawn::TextureFormat::R8G8B8A8Unorm:
                return MTLPixelFormatRGBA8Unorm;
            case dawn::TextureFormat::R8G8Unorm:
                return MTLPixelFormatRG8Unorm;
            case dawn::TextureFormat::R8Unorm:
                return MTLPixelFormatR8Unorm;
            case dawn::TextureFormat::R8G8B8A8Uint:
                return MTLPixelFormatRGBA8Uint;
            case dawn::TextureFormat::R8G8Uint:
                return MTLPixelFormatRG8Uint;
            case dawn::TextureFormat::R8Uint:
                return MTLPixelFormatR8Uint;
            case dawn::TextureFormat::B8G8R8A8Unorm:
                return MTLPixelFormatBGRA8Unorm;
            case dawn::TextureFormat::D32FloatS8Uint:
                return MTLPixelFormatDepth32Float_Stencil8;
            default:
                UNREACHABLE();
                return MTLPixelFormatRGBA8Unorm;
        }
    }

    MaybeError ValidateIOSurfaceCanBeWrapped(const DeviceBase*,
                                             const TextureDescriptor* descriptor,
                                             IOSurfaceRef ioSurface,
                                             uint32_t plane) {
        // IOSurfaceGetPlaneCount can return 0 for non-planar IOSurfaces but we will treat
        // non-planar like it is a single plane.
        size_t surfacePlaneCount = std::max(size_t(1), IOSurfaceGetPlaneCount(ioSurface));
        if (plane >= surfacePlaneCount) {
            return DAWN_VALIDATION_ERROR("IOSurface plane doesn't exist");
        }

        if (descriptor->dimension != dawn::TextureDimension::e2D) {
            return DAWN_VALIDATION_ERROR("IOSurface texture must be 2D");
        }

        if (descriptor->mipLevelCount != 1) {
            return DAWN_VALIDATION_ERROR("IOSurface mip level count must be 1");
        }

        if (descriptor->arrayLayerCount != 1) {
            return DAWN_VALIDATION_ERROR("IOSurface array layer count must be 1");
        }

        if (descriptor->sampleCount != 1) {
            return DAWN_VALIDATION_ERROR("IOSurface sample count must be 1");
        }

        if (descriptor->size.width != IOSurfaceGetWidthOfPlane(ioSurface, plane) ||
            descriptor->size.height != IOSurfaceGetHeightOfPlane(ioSurface, plane) ||
            descriptor->size.depth != 1) {
            return DAWN_VALIDATION_ERROR("IOSurface size doesn't match descriptor");
        }

        dawn::TextureFormat ioSurfaceFormat;
        DAWN_TRY_ASSIGN(ioSurfaceFormat,
                        GetFormatEquivalentToIOSurfaceFormat(IOSurfaceGetPixelFormat(ioSurface)));
        if (descriptor->format != ioSurfaceFormat) {
            return DAWN_VALIDATION_ERROR("IOSurface format doesn't match descriptor");
        }

        return {};
    }

    MTLTextureDescriptor* CreateMetalTextureDescriptor(const TextureDescriptor* descriptor) {
        MTLTextureDescriptor* mtlDesc = [MTLTextureDescriptor new];
        mtlDesc.textureType = MetalTextureType(descriptor->dimension, descriptor->arrayLayerCount,
                                               descriptor->sampleCount);
        mtlDesc.usage = MetalTextureUsage(descriptor->usage);
        mtlDesc.pixelFormat = MetalPixelFormat(descriptor->format);

        mtlDesc.width = descriptor->size.width;
        mtlDesc.height = descriptor->size.height;
        mtlDesc.depth = descriptor->size.depth;

        mtlDesc.mipmapLevelCount = descriptor->mipLevelCount;
        mtlDesc.arrayLength = descriptor->arrayLayerCount;
        mtlDesc.storageMode = MTLStorageModePrivate;

        mtlDesc.sampleCount = descriptor->sampleCount;

        return mtlDesc;
    }

    Texture::Texture(Device* device, const TextureDescriptor* descriptor)
        : TextureBase(device, descriptor, TextureState::OwnedInternal) {
        MTLTextureDescriptor* mtlDesc = CreateMetalTextureDescriptor(descriptor);
        mMtlTexture = [device->GetMTLDevice() newTextureWithDescriptor:mtlDesc];
        [mtlDesc release];
    }

    Texture::Texture(Device* device, const TextureDescriptor* descriptor, id<MTLTexture> mtlTexture)
        : TextureBase(device, descriptor, TextureState::OwnedInternal), mMtlTexture(mtlTexture) {
        [mMtlTexture retain];
    }

    Texture::Texture(Device* device,
                     const TextureDescriptor* descriptor,
                     IOSurfaceRef ioSurface,
                     uint32_t plane)
        : TextureBase(device, descriptor, TextureState::OwnedInternal) {
        MTLTextureDescriptor* mtlDesc = CreateMetalTextureDescriptor(descriptor);
        mtlDesc.storageMode = MTLStorageModeManaged;
        mMtlTexture = [device->GetMTLDevice() newTextureWithDescriptor:mtlDesc
                                                             iosurface:ioSurface
                                                                 plane:plane];
        [mtlDesc release];
    }

    Texture::~Texture() {
        DestroyInternal();
    }

    void Texture::DestroyImpl() {
        [mMtlTexture release];
        mMtlTexture = nil;
    }

    id<MTLTexture> Texture::GetMTLTexture() {
        return mMtlTexture;
    }

    TextureView::TextureView(TextureBase* texture, const TextureViewDescriptor* descriptor)
        : TextureViewBase(texture, descriptor) {
        id<MTLTexture> mtlTexture = ToBackend(texture)->GetMTLTexture();

        if (!UsageNeedsTextureView(texture->GetUsage())) {
            mMtlTextureView = nil;
        } else if (!RequiresCreatingNewTextureView(texture, descriptor)) {
            mMtlTextureView = [mtlTexture retain];
        } else {
            MTLPixelFormat format = MetalPixelFormat(descriptor->format);
            MTLTextureType textureViewType =
                MetalTextureViewType(descriptor->dimension, texture->GetSampleCount());
            auto mipLevelRange = NSMakeRange(descriptor->baseMipLevel, descriptor->mipLevelCount);
            auto arrayLayerRange =
                NSMakeRange(descriptor->baseArrayLayer, descriptor->arrayLayerCount);

            mMtlTextureView = [mtlTexture newTextureViewWithPixelFormat:format
                                                            textureType:textureViewType
                                                                 levels:mipLevelRange
                                                                 slices:arrayLayerRange];
        }
    }

    TextureView::~TextureView() {
        [mMtlTextureView release];
    }

    id<MTLTexture> TextureView::GetMTLTexture() {
        ASSERT(mMtlTextureView != nil);
        return mMtlTextureView;
    }
}}  // namespace dawn_native::metal
