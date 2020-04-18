// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/owned_mailbox.h"

#include "base/logging.h"
#include "components/viz/common/gl_helper.h"
#include "content/browser/compositor/image_transport_factory.h"

namespace content {

OwnedMailbox::OwnedMailbox(viz::GLHelper* gl_helper)
    : texture_id_(0), gl_helper_(gl_helper) {
  texture_id_ = gl_helper_->CreateTexture();
  mailbox_holder_ = gl_helper_->ProduceMailboxHolderFromTexture(texture_id_);
  // The texture target is not exposed on this class, as GLHelper assumes
  // GL_TEXTURE_2D.
  DCHECK_EQ(mailbox_holder_.texture_target,
            static_cast<uint32_t>(GL_TEXTURE_2D));
  ImageTransportFactory::GetInstance()->GetContextFactory()->AddObserver(this);
}

OwnedMailbox::~OwnedMailbox() {
  if (gl_helper_)
    Destroy();
}

void OwnedMailbox::UpdateSyncToken(const gpu::SyncToken& sync_token) {
  if (sync_token.HasData())
    mailbox_holder_.sync_token = sync_token;
}

void OwnedMailbox::Destroy() {
  ImageTransportFactory::GetInstance()->GetContextFactory()->RemoveObserver(
      this);
  gl_helper_->WaitSyncToken(mailbox_holder_.sync_token);
  gl_helper_->DeleteTexture(texture_id_);
  texture_id_ = 0;
  mailbox_holder_ = gpu::MailboxHolder();
  gl_helper_ = nullptr;
}

void OwnedMailbox::OnLostResources() {
  if (gl_helper_)
    Destroy();
}

}  // namespace content
