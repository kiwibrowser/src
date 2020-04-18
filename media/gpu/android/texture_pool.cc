// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/texture_pool.h"

#include "gpu/command_buffer/service/texture_manager.h"
#include "media/gpu/android/texture_wrapper.h"
#include "media/gpu/command_buffer_helper.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/scoped_make_current.h"

namespace media {

TexturePool::TexturePool(scoped_refptr<CommandBufferHelper> helper)
    : helper_(std::move(helper)), weak_factory_(this) {
  if (helper_) {
    helper_->SetWillDestroyStubCB(base::BindOnce(
        &TexturePool::OnWillDestroyStub, weak_factory_.GetWeakPtr()));
  }
}

TexturePool::~TexturePool() {
  // Note that the size of |pool_| doesn't, in general, tell us if there are any
  // textures.  If the stub has been destroyed, then we will drop the
  // TextureRefs but leave null entries in the map.  So, we check |stub_| too.
  if (pool_.size() && helper_) {
    // TODO(liberato): consider using ScopedMakeCurrent here, though if we are
    // ever called as part of decoder teardown, then using ScopedMakeCurrent
    // isn't safe.  For now, we preserve the old behavior (MakeCurrent).
    //
    // We check IsContextCurrent, even though that only checks for the
    // underlying shared context if |context| is a virtual context.  Assuming
    // that all TextureRef does is to delete a texture, this is enough.  Of
    // course, we shouldn't assume that this is all it does.
    bool have_context =
        helper_->IsContextCurrent() || helper_->MakeContextCurrent();
    DestroyAllPlatformTextures(have_context);
  }
}

void TexturePool::AddTexture(std::unique_ptr<TextureWrapper> texture) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(pool_.find(texture.get()) == pool_.end());
  // Don't permit additions after we've lost the stub.
  // TODO(liberato): consider making this fail gracefully.  However, nobody
  // should be doing this, so for now it's a DCHECK.
  DCHECK(helper_);
  TextureWrapper* texture_raw = texture.get();
  pool_[texture_raw] = std::move(texture);
}

void TexturePool::ReleaseTexture(TextureWrapper* texture,
                                 const gpu::SyncToken& sync_token) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // If we don't have a sync token, or if we have no stub, then just finish.
  if (!sync_token.HasData() || !helper_) {
    OnSyncTokenReleased(texture);
    return;
  }

  // We keep a strong ref to |this| in the callback, so that we are guaranteed
  // to receive it.  It's common for the last ref to us to be our caller, as
  // a callback.  We need to stick around a bit longer than that if there's a
  // sync token.  Plus, we're required to keep |helper_| around while a wait is
  // still pending.
  helper_->WaitForSyncToken(
      sync_token, base::BindOnce(&TexturePool::OnSyncTokenReleased,
                                 scoped_refptr<TexturePool>(this), texture));
}

void TexturePool::OnSyncTokenReleased(TextureWrapper* texture) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto iter = pool_.find(texture);
  DCHECK(iter != pool_.end());

  // If we can't make the context current, then notify the texture.  Note that
  // the wrapper might already have been destroyed, which is fine.  We elide
  // the MakeContextCurrent if our underlying physical context is current, which
  // only works if we don't do much besides delete the texture.
  bool have_context =
      helper_ && (helper_->IsContextCurrent() || helper_->MakeContextCurrent());
  if (iter->second && !have_context)
    texture->ForceContextLost();

  pool_.erase(iter);
}

void TexturePool::DestroyAllPlatformTextures(bool have_context) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Destroy the wrapper, but keep the entry around in the map.  We do this so
  // that ReleaseTexture can still check that at least the texture was, at some
  // point, in the map.  Hopefully, since nobody should be adding textures to
  // the pool after we've lost the stub, there's no issue with aliasing if the
  // ptr is re-used; it won't be given to us, so it's okay.
  for (auto& it : pool_) {
    std::unique_ptr<TextureWrapper> texture = std::move(it.second);
    if (!texture)
      continue;

    // If we can't make the context current, then notify all the textures that
    // they can't delete the underlying platform textures.
    if (!have_context)
      texture->ForceContextLost();

    // |texture| will be destroyed.
  }
}

void TexturePool::OnWillDestroyStub(bool have_context) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(helper_);
  DestroyAllPlatformTextures(have_context);
  helper_ = nullptr;
}

}  // namespace media
