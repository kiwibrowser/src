// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/reflector_texture.h"

#include "components/viz/common/gl_helper.h"
#include "content/browser/compositor/owned_mailbox.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"

namespace content {

ReflectorTexture::ReflectorTexture(viz::ContextProvider* context_provider)
    : texture_id_(0) {
  viz::GLHelper* shared_helper =
      ImageTransportFactory::GetInstance()->GetGLHelper();
  mailbox_ = new OwnedMailbox(shared_helper);
  gpu::gles2::GLES2Interface* gl = context_provider->ContextGL();

  gl_helper_.reset(new viz::GLHelper(gl, context_provider->ContextSupport()));

  texture_id_ = gl_helper_->ConsumeMailboxToTexture(mailbox_->mailbox(),
                                                    mailbox_->sync_token());
}

ReflectorTexture::~ReflectorTexture() {
  gl_helper_->DeleteTexture(texture_id_);
}

void ReflectorTexture::CopyTextureFullImage(const gfx::Size& size) {
  gl_helper_->CopyTextureFullImage(texture_id_, size);
  // Insert a barrier to make the copy show up in the mirroring compositor's
  // mailbox. Since the the compositor contexts and the
  // ImageTransportFactory's
  // GLHelper are all on the same GPU channel, this is sufficient instead of
  // plumbing through a sync point.
  gl_helper_->InsertOrderingBarrier();
}

void ReflectorTexture::CopyTextureSubImage(const gfx::Rect& rect) {
  gl_helper_->CopyTextureSubImage(texture_id_, rect);
  // Insert a barrier for the same reason above.
  gl_helper_->InsertOrderingBarrier();
}

}  // namespace content
