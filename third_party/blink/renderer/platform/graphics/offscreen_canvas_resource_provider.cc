// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/offscreen_canvas_resource_provider.h"

#include "base/memory/shared_memory.h"
#include "base/numerics/checked_math.h"
#include "components/viz/common/quads/shared_bitmap.h"
#include "components/viz/common/resources/bitmap_allocation.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_graphics_context_3d_provider.h"
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"
#include "third_party/blink/renderer/platform/graphics/offscreen_canvas_frame_dispatcher.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/typed_arrays/array_buffer.h"
#include "third_party/blink/renderer/platform/wtf/typed_arrays/uint8_array.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSwizzle.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace {

// TODO(danakj): One day the gpu::mojom::Mailbox type should be shared with
// blink directly and we won't need to use gpu::mojom::blink::Mailbox, nor the
// conversion through WTF::Vector.
gpu::mojom::blink::MailboxPtr SharedBitmapIdToGpuMailboxPtr(
    const viz::SharedBitmapId& id) {
  WTF::Vector<int8_t> name(GL_MAILBOX_SIZE_CHROMIUM);
  for (int i = 0; i < GL_MAILBOX_SIZE_CHROMIUM; ++i)
    name[i] = id.name[i];
  return {base::in_place, name};
}

}  // namespace

namespace blink {

OffscreenCanvasResourceProvider::OffscreenCanvasResourceProvider(
    int width,
    int height,
    OffscreenCanvasFrameDispatcher* frame_dispatcher)
    : frame_dispatcher_(frame_dispatcher), width_(width), height_(height) {}

OffscreenCanvasResourceProvider::~OffscreenCanvasResourceProvider() = default;

std::unique_ptr<OffscreenCanvasResourceProvider::FrameResource>
OffscreenCanvasResourceProvider::CreateOrRecycleFrameResource() {
  if (recyclable_resource_) {
    recyclable_resource_->spare_lock = true;
    return std::move(recyclable_resource_);
  }
  return std::make_unique<FrameResource>();
}

void OffscreenCanvasResourceProvider::TransferResource(
    viz::TransferableResource* resource) {
  resource->id = next_resource_id_;
  resource->format = viz::ResourceFormat::RGBA_8888;
  resource->size = gfx::Size(width_, height_);
  // This indicates the filtering on the resource inherently, not the desired
  // filtering effect on the quad.
  resource->filter = GL_NEAREST;
  // TODO(crbug.com/646022): making this overlay-able.
  resource->is_overlay_candidate = false;
}

void OffscreenCanvasResourceProvider::SetTransferableResourceToSharedBitmap(
    viz::TransferableResource& resource,
    scoped_refptr<StaticBitmapImage> image) {
  std::unique_ptr<FrameResource> frame_resource =
      CreateOrRecycleFrameResource();
  if (!frame_resource->shared_memory) {
    frame_resource->provider = this;
    frame_resource->shared_bitmap_id = viz::SharedBitmap::GenerateId();
    frame_resource->shared_memory =
        viz::bitmap_allocation::AllocateMappedBitmap(gfx::Size(width_, height_),
                                                     resource.format);
    frame_dispatcher_->DidAllocateSharedBitmap(
        viz::bitmap_allocation::DuplicateAndCloseMappedBitmap(
            frame_resource->shared_memory.get(), gfx::Size(width_, height_),
            resource.format),
        SharedBitmapIdToGpuMailboxPtr(frame_resource->shared_bitmap_id));
  }
  void* pixels = frame_resource->shared_memory->memory();
  DCHECK(pixels);
  // When |image| is texture backed, this function does a GPU readback which is
  // required.
  sk_sp<SkImage> sk_image = image->PaintImageForCurrentFrame().GetSkImage();
  if (sk_image->bounds().isEmpty())
    return;
  SkImageInfo image_info = SkImageInfo::Make(
      width_, height_, kN32_SkColorType,
      image->IsPremultiplied() ? kPremul_SkAlphaType : kUnpremul_SkAlphaType,
      sk_image->refColorSpace());
  if (image_info.isEmpty())
    return;

  if (RuntimeEnabledFeatures::CanvasColorManagementEnabled()) {
    image_info = image_info.makeColorType(sk_image->colorType());
  }

  // TODO(junov): Optimize to avoid copying pixels for non-texture-backed
  // sk_image. See crbug.com/651456.
  bool read_pixels_successful =
      sk_image->readPixels(image_info, pixels, image_info.minRowBytes(), 0, 0);
  DCHECK(read_pixels_successful);
  if (!read_pixels_successful)
    return;
  resource.mailbox_holder.mailbox = frame_resource->shared_bitmap_id;
  resource.mailbox_holder.texture_target = 0;
  resource.is_software = true;

  resources_.insert(next_resource_id_, std::move(frame_resource));
}

void OffscreenCanvasResourceProvider::
    SetTransferableResourceToStaticBitmapImage(
        viz::TransferableResource& resource,
        scoped_refptr<StaticBitmapImage> image) {
  DCHECK(image->IsTextureBacked());
  DCHECK(image->IsValid());
  image->EnsureMailbox(kVerifiedSyncToken, GL_LINEAR);
  resource.mailbox_holder = gpu::MailboxHolder(
      image->GetMailbox(), image->GetSyncToken(), GL_TEXTURE_2D);
  resource.read_lock_fences_enabled = false;
  resource.is_software = false;

  std::unique_ptr<FrameResource> frame_resource =
      CreateOrRecycleFrameResource();

  frame_resource->provider = this;
  frame_resource->image = std::move(image);
  resources_.insert(next_resource_id_, std::move(frame_resource));
}

void OffscreenCanvasResourceProvider::ReclaimResources(
    const WTF::Vector<viz::ReturnedResource>& resources) {
  for (const auto& resource : resources) {
    auto it = resources_.find(resource.id);

    DCHECK(it != resources_.end());
    if (it == resources_.end())
      continue;

    if (it->value->image && it->value->image->ContextProviderWrapper() &&
        resource.sync_token.HasData()) {
      it->value->image->ContextProviderWrapper()
          ->ContextProvider()
          ->ContextGL()
          ->WaitSyncTokenCHROMIUM(resource.sync_token.GetConstData());
    }
    ReclaimResourceInternal(it);
  }
}

void OffscreenCanvasResourceProvider::ReclaimResource(unsigned resource_id) {
  auto it = resources_.find(resource_id);
  if (it != resources_.end()) {
    ReclaimResourceInternal(it);
  }
}

void OffscreenCanvasResourceProvider::ReclaimResourceInternal(
    const ResourceMap::iterator& it) {
  if (it->value->spare_lock) {
    it->value->spare_lock = false;
  } else {
    // Really reclaim the resources.
    recyclable_resource_ = std::move(it->value);
    // Release SkImage immediately since it is not recyclable.
    recyclable_resource_->image = nullptr;
    resources_.erase(it);
  }
}

OffscreenCanvasResourceProvider::FrameResource::~FrameResource() {
  provider->frame_dispatcher_->DidDeleteSharedBitmap(
      SharedBitmapIdToGpuMailboxPtr(shared_bitmap_id));
}

}  // namespace blink
