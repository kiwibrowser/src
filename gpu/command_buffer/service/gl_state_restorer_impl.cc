// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gl_state_restorer_impl.h"

#include "gpu/command_buffer/service/decoder_context.h"
#include "gpu/command_buffer/service/query_manager.h"

namespace gpu {

GLStateRestorerImpl::GLStateRestorerImpl(base::WeakPtr<DecoderContext> decoder)
    : decoder_(decoder) {}

GLStateRestorerImpl::~GLStateRestorerImpl() = default;

bool GLStateRestorerImpl::IsInitialized() {
  DCHECK(decoder_.get());
  return decoder_->initialized();
}

void GLStateRestorerImpl::RestoreState(const gl::GLStateRestorer* prev_state) {
  DCHECK(decoder_.get());
  const GLStateRestorerImpl* restorer_impl =
      static_cast<const GLStateRestorerImpl*>(prev_state);

  decoder_->RestoreState(
      restorer_impl ? restorer_impl->GetContextState() : NULL);
}

void GLStateRestorerImpl::RestoreAllTextureUnitAndSamplerBindings() {
  DCHECK(decoder_.get());
  decoder_->RestoreAllTextureUnitAndSamplerBindings(NULL);
}

void GLStateRestorerImpl::RestoreActiveTexture() {
  DCHECK(decoder_.get());
  decoder_->RestoreActiveTexture();
}

void GLStateRestorerImpl::RestoreActiveTextureUnitBinding(unsigned int target) {
  DCHECK(decoder_.get());
  decoder_->RestoreActiveTextureUnitBinding(target);
}

void GLStateRestorerImpl::RestoreAllExternalTextureBindingsIfNeeded() {
  DCHECK(decoder_.get());
  decoder_->RestoreAllExternalTextureBindingsIfNeeded();
}

void GLStateRestorerImpl::RestoreFramebufferBindings() {
  DCHECK(decoder_.get());
  decoder_->RestoreFramebufferBindings();
}

void GLStateRestorerImpl::RestoreProgramBindings() {
  DCHECK(decoder_.get());
  decoder_->RestoreProgramBindings();
}

void GLStateRestorerImpl::RestoreBufferBinding(unsigned int target) {
  DCHECK(decoder_.get());
  decoder_->RestoreBufferBinding(target);
}

void GLStateRestorerImpl::RestoreVertexAttribArray(unsigned int index) {
  DCHECK(decoder_.get());
  decoder_->RestoreVertexAttribArray(index);
}

void GLStateRestorerImpl::PauseQueries() {
  DCHECK(decoder_.get());
  decoder_->GetQueryManager()->PauseQueries();
}

void GLStateRestorerImpl::ResumeQueries() {
  DCHECK(decoder_.get());
  decoder_->GetQueryManager()->ResumeQueries();
}

const gles2::ContextState* GLStateRestorerImpl::GetContextState() const {
  DCHECK(decoder_.get());
  return decoder_->GetContextState();
}

}  // namespace gpu
