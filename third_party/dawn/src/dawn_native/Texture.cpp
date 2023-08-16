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

#include "dawn_native/Texture.h"

#include <algorithm>

#include "common/Assert.h"
#include "common/Constants.h"
#include "common/Math.h"
#include "dawn_native/Device.h"
#include "dawn_native/PassResourceUsage.h"
#include "dawn_native/ValidationUtils_autogen.h"

namespace dawn_native {
    namespace {
        // TODO(jiawei.shao@intel.com): implement texture view format compatibility rule
        MaybeError ValidateTextureViewFormatCompatibility(const TextureBase* texture,
                                                          const TextureViewDescriptor* descriptor) {
            if (texture->GetFormat().format != descriptor->format) {
                return DAWN_VALIDATION_ERROR(
                    "The format of texture view is not compatible to the original texture");
            }

            return {};
        }

        // TODO(jiawei.shao@intel.com): support validation on all texture view dimensions
        bool IsTextureViewDimensionCompatibleWithTextureDimension(
            wgpu::TextureViewDimension textureViewDimension,
            wgpu::TextureDimension textureDimension) {
            switch (textureViewDimension) {
                case wgpu::TextureViewDimension::e2D:
                case wgpu::TextureViewDimension::e2DArray:
                case wgpu::TextureViewDimension::Cube:
                case wgpu::TextureViewDimension::CubeArray:
                    return textureDimension == wgpu::TextureDimension::e2D;
                default:
                    UNREACHABLE();
                    return false;
            }
        }

        // TODO(jiawei.shao@intel.com): support validation on all texture view dimensions
        bool IsArrayLayerValidForTextureViewDimension(
            wgpu::TextureViewDimension textureViewDimension,
            uint32_t textureViewArrayLayer) {
            switch (textureViewDimension) {
                case wgpu::TextureViewDimension::e2D:
                    return textureViewArrayLayer == 1u;
                case wgpu::TextureViewDimension::e2DArray:
                    return true;
                case wgpu::TextureViewDimension::Cube:
                    return textureViewArrayLayer == 6u;
                case wgpu::TextureViewDimension::CubeArray:
                    return textureViewArrayLayer % 6 == 0;
                default:
                    UNREACHABLE();
                    return false;
            }
        }

        bool IsTextureSizeValidForTextureViewDimension(
            wgpu::TextureViewDimension textureViewDimension,
            const Extent3D& textureSize) {
            switch (textureViewDimension) {
                case wgpu::TextureViewDimension::Cube:
                case wgpu::TextureViewDimension::CubeArray:
                    return textureSize.width == textureSize.height;
                case wgpu::TextureViewDimension::e2D:
                case wgpu::TextureViewDimension::e2DArray:
                    return true;
                default:
                    UNREACHABLE();
                    return false;
            }
        }

        // TODO(jiawei.shao@intel.com): support more sample count.
        MaybeError ValidateSampleCount(const TextureDescriptor* descriptor, const Format* format) {
            if (!IsValidSampleCount(descriptor->sampleCount)) {
                return DAWN_VALIDATION_ERROR("The sample count of the texture is not supported.");
            }

            if (descriptor->sampleCount > 1) {
                if (descriptor->mipLevelCount > 1) {
                    return DAWN_VALIDATION_ERROR(
                        "The mipmap level count of a multisampled texture must be 1.");
                }

                // Multisampled 2D array texture is not supported because on Metal it requires the
                // version of macOS be greater than 10.14.
                if (descriptor->size.depth > 1) {
                    return DAWN_VALIDATION_ERROR(
                        "Multisampled textures with depth > 1 are not supported.");
                }

                if (format->isCompressed) {
                    return DAWN_VALIDATION_ERROR(
                        "The sample counts of the textures in BC formats must be 1.");
                }

                if (descriptor->usage & wgpu::TextureUsage::Storage) {
                    return DAWN_VALIDATION_ERROR(
                        "The sample counts of the storage textures must be 1.");
                }
            }

            return {};
        }

        MaybeError ValidateTextureViewDimensionCompatibility(
            const TextureBase* texture,
            const TextureViewDescriptor* descriptor) {
            if (!IsArrayLayerValidForTextureViewDimension(descriptor->dimension,
                                                          descriptor->arrayLayerCount)) {
                return DAWN_VALIDATION_ERROR(
                    "The dimension of the texture view is not compatible with the layer count");
            }

            if (!IsTextureViewDimensionCompatibleWithTextureDimension(descriptor->dimension,
                                                                      texture->GetDimension())) {
                return DAWN_VALIDATION_ERROR(
                    "The dimension of the texture view is not compatible with the dimension of the"
                    "original texture");
            }

            if (!IsTextureSizeValidForTextureViewDimension(descriptor->dimension,
                                                           texture->GetSize())) {
                return DAWN_VALIDATION_ERROR(
                    "The dimension of the texture view is not compatible with the size of the"
                    "original texture");
            }

            return {};
        }

        MaybeError ValidateTextureSize(const TextureDescriptor* descriptor, const Format* format) {
            ASSERT(descriptor->size.width != 0 && descriptor->size.height != 0);
            if (descriptor->size.width > kMaxTextureSize ||
                descriptor->size.height > kMaxTextureSize) {
                return DAWN_VALIDATION_ERROR("Texture max size exceeded");
            }

            if (Log2(std::max(descriptor->size.width, descriptor->size.height)) + 1 <
                descriptor->mipLevelCount) {
                return DAWN_VALIDATION_ERROR("Texture has too many mip levels");
            }

            if (format->isCompressed && (descriptor->size.width % format->blockWidth != 0 ||
                                         descriptor->size.height % format->blockHeight != 0)) {
                return DAWN_VALIDATION_ERROR(
                    "The size of the texture is incompatible with the texture format");
            }

            if (descriptor->dimension == wgpu::TextureDimension::e2D &&
                descriptor->size.depth > kMaxTexture2DArrayLayers) {
                return DAWN_VALIDATION_ERROR("Texture 2D array layer count exceeded");
            }
            if (descriptor->mipLevelCount > kMaxTexture2DMipLevels) {
                return DAWN_VALIDATION_ERROR("Max texture 2D mip level exceeded");
            }

            return {};
        }

        MaybeError ValidateTextureUsage(const TextureDescriptor* descriptor, const Format* format) {
            DAWN_TRY(dawn_native::ValidateTextureUsage(descriptor->usage));

            constexpr wgpu::TextureUsage kValidCompressedUsages = wgpu::TextureUsage::Sampled |
                                                                  wgpu::TextureUsage::CopySrc |
                                                                  wgpu::TextureUsage::CopyDst;
            if (format->isCompressed && (descriptor->usage & (~kValidCompressedUsages))) {
                return DAWN_VALIDATION_ERROR(
                    "Compressed texture format is incompatible with the texture usage");
            }

            if (!format->isRenderable &&
                (descriptor->usage & wgpu::TextureUsage::OutputAttachment)) {
                return DAWN_VALIDATION_ERROR(
                    "Non-renderable format used with OutputAttachment usage");
            }

            if (!format->supportsStorageUsage &&
                (descriptor->usage & wgpu::TextureUsage::Storage)) {
                return DAWN_VALIDATION_ERROR("Format cannot be used in storage textures");
            }

            return {};
        }

    }  // anonymous namespace

    MaybeError ValidateTextureDescriptor(const DeviceBase* device,
                                         const TextureDescriptor* descriptor) {
        if (descriptor == nullptr) {
            return DAWN_VALIDATION_ERROR("Texture descriptor is nullptr");
        }
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        const Format* format;
        DAWN_TRY_ASSIGN(format, device->GetInternalFormat(descriptor->format));

        DAWN_TRY(ValidateTextureUsage(descriptor, format));
        DAWN_TRY(ValidateTextureDimension(descriptor->dimension));
        DAWN_TRY(ValidateSampleCount(descriptor, format));

        // TODO(jiawei.shao@intel.com): check stuff based on the dimension
        if (descriptor->size.width == 0 || descriptor->size.height == 0 ||
            descriptor->size.depth == 0 || descriptor->mipLevelCount == 0) {
            return DAWN_VALIDATION_ERROR("Cannot create an empty texture");
        }

        if (descriptor->dimension != wgpu::TextureDimension::e2D) {
            return DAWN_VALIDATION_ERROR("Texture dimension must be 2D (for now)");
        }

        DAWN_TRY(ValidateTextureSize(descriptor, format));

        return {};
    }

    MaybeError ValidateTextureViewDescriptor(const TextureBase* texture,
                                             const TextureViewDescriptor* descriptor) {
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        // Parent texture should have been already validated.
        ASSERT(texture);
        ASSERT(!texture->IsError());
        if (texture->GetTextureState() == TextureBase::TextureState::Destroyed) {
            return DAWN_VALIDATION_ERROR("Destroyed texture used to create texture view");
        }

        DAWN_TRY(ValidateTextureViewDimension(descriptor->dimension));
        if (descriptor->dimension == wgpu::TextureViewDimension::e1D ||
            descriptor->dimension == wgpu::TextureViewDimension::e3D) {
            return DAWN_VALIDATION_ERROR("Texture view dimension must be 2D compatible.");
        }

        DAWN_TRY(ValidateTextureFormat(descriptor->format));

        DAWN_TRY(ValidateTextureAspect(descriptor->aspect));
        if (descriptor->aspect != wgpu::TextureAspect::All) {
            return DAWN_VALIDATION_ERROR("Texture aspect must be 'all'");
        }

        // TODO(jiawei.shao@intel.com): check stuff based on resource limits
        if (descriptor->arrayLayerCount == 0 || descriptor->mipLevelCount == 0) {
            return DAWN_VALIDATION_ERROR("Cannot create an empty texture view");
        }

        if (uint64_t(descriptor->baseArrayLayer) + uint64_t(descriptor->arrayLayerCount) >
            uint64_t(texture->GetArrayLayers())) {
            return DAWN_VALIDATION_ERROR("Texture view array-layer out of range");
        }

        if (uint64_t(descriptor->baseMipLevel) + uint64_t(descriptor->mipLevelCount) >
            uint64_t(texture->GetNumMipLevels())) {
            return DAWN_VALIDATION_ERROR("Texture view mip-level out of range");
        }

        DAWN_TRY(ValidateTextureViewFormatCompatibility(texture, descriptor));
        DAWN_TRY(ValidateTextureViewDimensionCompatibility(texture, descriptor));

        return {};
    }

    TextureViewDescriptor GetTextureViewDescriptorWithDefaults(
        const TextureBase* texture,
        const TextureViewDescriptor* descriptor) {
        ASSERT(texture);

        TextureViewDescriptor desc = {};
        if (descriptor) {
            desc = *descriptor;
        }

        // The default value for the view dimension depends on the texture's dimension with a
        // special case for 2DArray being chosen automatically if arrayLayerCount is unspecified.
        if (desc.dimension == wgpu::TextureViewDimension::Undefined) {
            switch (texture->GetDimension()) {
                case wgpu::TextureDimension::e1D:
                    desc.dimension = wgpu::TextureViewDimension::e1D;
                    break;

                case wgpu::TextureDimension::e2D:
                    if (texture->GetArrayLayers() > 1u && desc.arrayLayerCount == 0) {
                        desc.dimension = wgpu::TextureViewDimension::e2DArray;
                    } else {
                        desc.dimension = wgpu::TextureViewDimension::e2D;
                    }
                    break;

                case wgpu::TextureDimension::e3D:
                    desc.dimension = wgpu::TextureViewDimension::e3D;
                    break;

                default:
                    UNREACHABLE();
            }
        }

        if (desc.format == wgpu::TextureFormat::Undefined) {
            desc.format = texture->GetFormat().format;
        }
        if (desc.arrayLayerCount == 0) {
            desc.arrayLayerCount = texture->GetArrayLayers() - desc.baseArrayLayer;
        }
        if (desc.mipLevelCount == 0) {
            desc.mipLevelCount = texture->GetNumMipLevels() - desc.baseMipLevel;
        }
        return desc;
    }

    ResultOrError<TextureDescriptor> FixTextureDescriptor(DeviceBase* device,
                                                          const TextureDescriptor* desc) {
        TextureDescriptor fixedDesc = *desc;

        if (desc->arrayLayerCount != 1) {
            if (desc->size.depth != 1) {
                return DAWN_VALIDATION_ERROR("arrayLayerCount and size.depth cannot both be != 1");
            } else {
                fixedDesc.size.depth = fixedDesc.arrayLayerCount;
                fixedDesc.arrayLayerCount = 1;
                device->EmitDeprecationWarning(
                    "wgpu::TextureDescriptor::arrayLayerCount is deprecated in favor of "
                    "::size::depth");
            }
        }

        return {std::move(fixedDesc)};
    }

    bool IsValidSampleCount(uint32_t sampleCount) {
        switch (sampleCount) {
            case 1:
            case 4:
                return true;

            default:
                return false;
        }
    }

    // static
    SubresourceRange SubresourceRange::SingleSubresource(uint32_t baseMipLevel,
                                                         uint32_t baseArrayLayer) {
        return {baseMipLevel, 1, baseArrayLayer, 1};
    }

    // TextureBase

    TextureBase::TextureBase(DeviceBase* device,
                             const TextureDescriptor* descriptor,
                             TextureState state)
        : ObjectBase(device),
          mDimension(descriptor->dimension),
          mFormat(device->GetValidInternalFormat(descriptor->format)),
          mSize(descriptor->size),
          mMipLevelCount(descriptor->mipLevelCount),
          mSampleCount(descriptor->sampleCount),
          mUsage(descriptor->usage),
          mState(state) {
        uint32_t subresourceCount = GetSubresourceCount();
        mIsSubresourceContentInitializedAtIndex = std::vector<bool>(subresourceCount, false);

        // Add readonly storage usage if the texture has a storage usage. The validation rules in
        // ValidatePassResourceUsage will make sure we don't use both at the same time.
        if (mUsage & wgpu::TextureUsage::Storage) {
            mUsage |= kReadonlyStorageTexture;
        }
    }

    static Format kUnusedFormat;

    TextureBase::TextureBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : ObjectBase(device, tag), mFormat(kUnusedFormat) {
    }

    // static
    TextureBase* TextureBase::MakeError(DeviceBase* device) {
        return new TextureBase(device, ObjectBase::kError);
    }

    wgpu::TextureDimension TextureBase::GetDimension() const {
        ASSERT(!IsError());
        return mDimension;
    }

    // TODO(jiawei.shao@intel.com): return more information about texture format
    const Format& TextureBase::GetFormat() const {
        ASSERT(!IsError());
        return mFormat;
    }
    const Extent3D& TextureBase::GetSize() const {
        ASSERT(!IsError());
        return mSize;
    }
    uint32_t TextureBase::GetWidth() const {
        ASSERT(!IsError());
        return mSize.width;
    }
    uint32_t TextureBase::GetHeight() const {
        ASSERT(!IsError());
        ASSERT(mDimension == wgpu::TextureDimension::e2D ||
               mDimension == wgpu::TextureDimension::e3D);
        return mSize.height;
    }
    uint32_t TextureBase::GetDepth() const {
        ASSERT(!IsError());
        ASSERT(mDimension == wgpu::TextureDimension::e3D);
        return mSize.depth;
    }
    uint32_t TextureBase::GetArrayLayers() const {
        ASSERT(!IsError());
        // TODO(cwallez@chromium.org): Update for 1D / 3D textures when they are supported.
        ASSERT(mDimension == wgpu::TextureDimension::e2D);
        return mSize.depth;
    }
    uint32_t TextureBase::GetNumMipLevels() const {
        ASSERT(!IsError());
        return mMipLevelCount;
    }
    SubresourceRange TextureBase::GetAllSubresources() const {
        ASSERT(!IsError());
        return {0, mMipLevelCount, 0, GetArrayLayers()};
    }
    uint32_t TextureBase::GetSampleCount() const {
        ASSERT(!IsError());
        return mSampleCount;
    }
    uint32_t TextureBase::GetSubresourceCount() const {
        ASSERT(!IsError());
        return mMipLevelCount * mSize.depth;
    }
    wgpu::TextureUsage TextureBase::GetUsage() const {
        ASSERT(!IsError());
        return mUsage;
    }

    TextureBase::TextureState TextureBase::GetTextureState() const {
        ASSERT(!IsError());
        return mState;
    }

    uint32_t TextureBase::GetSubresourceIndex(uint32_t mipLevel, uint32_t arraySlice) const {
        ASSERT(arraySlice <= kMaxTexture2DArrayLayers);
        ASSERT(mipLevel <= kMaxTexture2DMipLevels);
        static_assert(kMaxTexture2DMipLevels <=
                          std::numeric_limits<uint32_t>::max() / kMaxTexture2DArrayLayers,
                      "texture size overflows uint32_t");
        return GetNumMipLevels() * arraySlice + mipLevel;
    }

    bool TextureBase::IsSubresourceContentInitialized(const SubresourceRange& range) const {
        ASSERT(!IsError());
        for (uint32_t arrayLayer = range.baseArrayLayer;
             arrayLayer < range.baseArrayLayer + range.layerCount; ++arrayLayer) {
            for (uint32_t mipLevel = range.baseMipLevel;
                 mipLevel < range.baseMipLevel + range.levelCount; ++mipLevel) {
                uint32_t subresourceIndex = GetSubresourceIndex(mipLevel, arrayLayer);
                ASSERT(subresourceIndex < mIsSubresourceContentInitializedAtIndex.size());
                if (!mIsSubresourceContentInitializedAtIndex[subresourceIndex]) {
                    return false;
                }
            }
        }
        return true;
    }

    void TextureBase::SetIsSubresourceContentInitialized(bool isInitialized,
                                                         const SubresourceRange& range) {
        ASSERT(!IsError());
        for (uint32_t arrayLayer = range.baseArrayLayer;
             arrayLayer < range.baseArrayLayer + range.layerCount; ++arrayLayer) {
            for (uint32_t mipLevel = range.baseMipLevel;
                 mipLevel < range.baseMipLevel + range.levelCount; ++mipLevel) {
                uint32_t subresourceIndex = GetSubresourceIndex(mipLevel, arrayLayer);
                ASSERT(subresourceIndex < mIsSubresourceContentInitializedAtIndex.size());
                mIsSubresourceContentInitializedAtIndex[subresourceIndex] = isInitialized;
            }
        }
    }

    MaybeError TextureBase::ValidateCanUseInSubmitNow() const {
        ASSERT(!IsError());
        if (mState == TextureState::Destroyed) {
            return DAWN_VALIDATION_ERROR("Destroyed texture used in a submit");
        }
        return {};
    }

    bool TextureBase::IsMultisampledTexture() const {
        ASSERT(!IsError());
        return mSampleCount > 1;
    }

    Extent3D TextureBase::GetMipLevelVirtualSize(uint32_t level) const {
        Extent3D extent = {std::max(mSize.width >> level, 1u), 1u, 1u};
        if (mDimension == wgpu::TextureDimension::e1D) {
            return extent;
        }

        extent.height = std::max(mSize.height >> level, 1u);
        if (mDimension == wgpu::TextureDimension::e2D) {
            return extent;
        }

        extent.depth = std::max(mSize.depth >> level, 1u);
        return extent;
    }

    Extent3D TextureBase::GetMipLevelPhysicalSize(uint32_t level) const {
        Extent3D extent = GetMipLevelVirtualSize(level);

        // Compressed Textures will have paddings if their width or height is not a multiple of
        // 4 at non-zero mipmap levels.
        if (mFormat.isCompressed) {
            // TODO(jiawei.shao@intel.com): check if there are any overflows.
            uint32_t blockWidth = mFormat.blockWidth;
            uint32_t blockHeight = mFormat.blockHeight;
            extent.width = (extent.width + blockWidth - 1) / blockWidth * blockWidth;
            extent.height = (extent.height + blockHeight - 1) / blockHeight * blockHeight;
        }

        return extent;
    }

    Extent3D TextureBase::ClampToMipLevelVirtualSize(uint32_t level,
                                                     const Origin3D& origin,
                                                     const Extent3D& extent) const {
        const Extent3D virtualSizeAtLevel = GetMipLevelVirtualSize(level);
        uint32_t clampedCopyExtentWidth = (origin.x + extent.width > virtualSizeAtLevel.width)
                                              ? (virtualSizeAtLevel.width - origin.x)
                                              : extent.width;
        uint32_t clampedCopyExtentHeight = (origin.y + extent.height > virtualSizeAtLevel.height)
                                               ? (virtualSizeAtLevel.height - origin.y)
                                               : extent.height;
        return {clampedCopyExtentWidth, clampedCopyExtentHeight, extent.depth};
    }

    TextureViewBase* TextureBase::CreateView(const TextureViewDescriptor* descriptor) {
        return GetDevice()->CreateTextureView(this, descriptor);
    }

    void TextureBase::Destroy() {
        if (GetDevice()->ConsumedError(ValidateDestroy())) {
            return;
        }
        ASSERT(!IsError());
        DestroyInternal();
    }

    void TextureBase::DestroyImpl() {
    }

    void TextureBase::DestroyInternal() {
        DestroyImpl();
        mState = TextureState::Destroyed;
    }

    MaybeError TextureBase::ValidateDestroy() const {
        DAWN_TRY(GetDevice()->ValidateObject(this));
        return {};
    }

    // TextureViewBase

    TextureViewBase::TextureViewBase(TextureBase* texture, const TextureViewDescriptor* descriptor)
        : ObjectBase(texture->GetDevice()),
          mTexture(texture),
          mAspect(descriptor->aspect),
          mFormat(GetDevice()->GetValidInternalFormat(descriptor->format)),
          mDimension(descriptor->dimension),
          mRange({descriptor->baseMipLevel, descriptor->mipLevelCount, descriptor->baseArrayLayer,
                  descriptor->arrayLayerCount}) {
    }

    TextureViewBase::TextureViewBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : ObjectBase(device, tag), mFormat(kUnusedFormat) {
    }

    // static
    TextureViewBase* TextureViewBase::MakeError(DeviceBase* device) {
        return new TextureViewBase(device, ObjectBase::kError);
    }

    const TextureBase* TextureViewBase::GetTexture() const {
        ASSERT(!IsError());
        return mTexture.Get();
    }

    TextureBase* TextureViewBase::GetTexture() {
        ASSERT(!IsError());
        return mTexture.Get();
    }

    wgpu::TextureAspect TextureViewBase::GetAspect() const {
        ASSERT(!IsError());
        return mAspect;
    }

    const Format& TextureViewBase::GetFormat() const {
        ASSERT(!IsError());
        return mFormat;
    }

    wgpu::TextureViewDimension TextureViewBase::GetDimension() const {
        ASSERT(!IsError());
        return mDimension;
    }

    uint32_t TextureViewBase::GetBaseMipLevel() const {
        ASSERT(!IsError());
        return mRange.baseMipLevel;
    }

    uint32_t TextureViewBase::GetLevelCount() const {
        ASSERT(!IsError());
        return mRange.levelCount;
    }

    uint32_t TextureViewBase::GetBaseArrayLayer() const {
        ASSERT(!IsError());
        return mRange.baseArrayLayer;
    }

    uint32_t TextureViewBase::GetLayerCount() const {
        ASSERT(!IsError());
        return mRange.layerCount;
    }

    const SubresourceRange& TextureViewBase::GetSubresourceRange() const {
        ASSERT(!IsError());
        return mRange;
    }
}  // namespace dawn_native
