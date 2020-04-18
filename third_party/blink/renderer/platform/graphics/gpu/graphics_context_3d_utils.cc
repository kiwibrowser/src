// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/gpu/graphics_context_3d_utils.h"

#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/blink/renderer/platform/graphics/web_graphics_context_3d_provider_wrapper.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace {

struct GrTextureMailboxReleaseProcData {
  GrTexture* gr_texture_;
  base::WeakPtr<blink::WebGraphicsContext3DProviderWrapper>
      context_provider_wrapper_;
};

void GrTextureMailboxReleaseProc(void* data) {
  GrTextureMailboxReleaseProcData* release_proc_data =
      static_cast<GrTextureMailboxReleaseProcData*>(data);

  if (release_proc_data->context_provider_wrapper_) {
    release_proc_data->context_provider_wrapper_->Utils()->RemoveCachedMailbox(
        release_proc_data->gr_texture_);
  }

  delete release_proc_data;
}

}  // unnamed namespace

namespace blink {

void GraphicsContext3DUtils::GetMailboxForSkImage(gpu::Mailbox& out_mailbox,
                                                  const sk_sp<SkImage>& image,
                                                  GLenum filter) {
  // This object is owned by context_provider_wrapper_, so that weak ref
  // should never be null.
  DCHECK(context_provider_wrapper_);
  DCHECK(image->isTextureBacked());
  GrContext* gr = context_provider_wrapper_->ContextProvider()->GetGrContext();
  gpu::gles2::GLES2Interface* gl =
      context_provider_wrapper_->ContextProvider()->ContextGL();

  DCHECK(gr);
  DCHECK(gl);
  GrTexture* gr_texture = image->getTexture();
  DCHECK(gr == gr_texture->getContext());
  auto it = cached_mailboxes_.find(gr_texture);
  if (it != cached_mailboxes_.end()) {
    out_mailbox = it->value;
  } else {
    gl->GenMailboxCHROMIUM(out_mailbox.name);

    GrTextureMailboxReleaseProcData* release_proc_data =
        new GrTextureMailboxReleaseProcData();
    release_proc_data->gr_texture_ = gr_texture;
    release_proc_data->context_provider_wrapper_ = context_provider_wrapper_;
    gr_texture->setRelease(GrTextureMailboxReleaseProc, release_proc_data);
    cached_mailboxes_.insert(gr_texture, out_mailbox);
  }

  GrBackendTexture backend_texture = image->getBackendTexture(true);
  DCHECK(backend_texture.isValid());

  GrGLTextureInfo info;
  bool result = backend_texture.getGLTextureInfo(&info);
  DCHECK(result);

  GLuint texture_id = info.fID;
  gl->BindTexture(GL_TEXTURE_2D, texture_id);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl->BindTexture(GL_TEXTURE_2D, 0);
  gl->ProduceTextureDirectCHROMIUM(texture_id, out_mailbox.name);
  image->getTexture()->textureParamsModified();
}

void GraphicsContext3DUtils::RemoveCachedMailbox(GrTexture* gr_texture) {
  cached_mailboxes_.erase(gr_texture);
}

}  // namespace blink
