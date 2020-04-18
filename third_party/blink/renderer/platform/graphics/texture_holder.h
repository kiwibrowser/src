// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_TEXTURE_HOLDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_TEXTURE_HOLDER_H_

#include "base/memory/weak_ptr.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/graphics/graphics_types.h"
#include "third_party/blink/renderer/platform/graphics/image.h"
#include "third_party/blink/renderer/platform/graphics/web_graphics_context_3d_provider_wrapper.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace blink {

class PLATFORM_EXPORT TextureHolder {
 public:
  virtual ~TextureHolder() = default;

  // Methods overridden by all sub-classes
  virtual bool IsSkiaTextureHolder() = 0;
  virtual bool IsMailboxTextureHolder() = 0;
  virtual IntSize Size() const = 0;
  virtual bool CurrentFrameKnownToBeOpaque() = 0;
  virtual bool IsValid() const = 0;

  // Methods overrided by MailboxTextureHolder
  virtual const gpu::Mailbox& GetMailbox() const {
    NOTREACHED();
    static const gpu::Mailbox mailbox;
    return mailbox;
  }
  virtual const gpu::SyncToken& GetSyncToken() const {
    static const gpu::SyncToken sync_token;
    return sync_token;
  }
  virtual void UpdateSyncToken(gpu::SyncToken) { NOTREACHED(); }
  virtual void Sync(MailboxSyncMode) { NOTREACHED(); }
  virtual bool IsCrossThread() const { return false; }

  // Methods overridden by SkiaTextureHolder
  virtual sk_sp<SkImage> GetSkImage() {
    NOTREACHED();
    return nullptr;
  }
  virtual void Abandon() { is_abandoned_ = true; }  // Overrides must call base.

  // Methods that have exactly the same impelmentation for all sub-classes
  base::WeakPtr<WebGraphicsContext3DProviderWrapper> ContextProviderWrapper()
      const {
    return context_provider_wrapper_;
  }

  WebGraphicsContext3DProvider* ContextProvider() const {
    return context_provider_wrapper_
               ? context_provider_wrapper_->ContextProvider()
               : nullptr;
  }
  bool IsAbandoned() const { return is_abandoned_; }

 protected:
  TextureHolder(base::WeakPtr<WebGraphicsContext3DProviderWrapper>&&
                    context_provider_wrapper)
      : context_provider_wrapper_(std::move(context_provider_wrapper)) {}

 private:
  // Keep a clone of the SingleThreadTaskRunner. This is to handle the case
  // where the AcceleratedStaticBitmapImage was created on one thread and
  // transferred to another thread, and the original thread gone out of scope,
  // and that we need to clear the resouces associated with that
  // AcceleratedStaticBitmapImage on the original thread.
  base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
  bool is_abandoned_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_TEXTURE_HOLDER_H_
