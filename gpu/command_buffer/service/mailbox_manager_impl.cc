// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/mailbox_manager_impl.h"

#include <stddef.h>

#include <algorithm>

#include "gpu/command_buffer/service/texture_base.h"

namespace gpu {
namespace gles2 {

MailboxManagerImpl::MailboxManagerImpl() = default;

MailboxManagerImpl::~MailboxManagerImpl() {
  DCHECK(mailbox_to_textures_.empty());
  DCHECK(textures_to_mailboxes_.empty());
}

bool MailboxManagerImpl::UsesSync() {
  return false;
}

TextureBase* MailboxManagerImpl::ConsumeTexture(const Mailbox& mailbox) {
  MailboxToTextureMap::iterator it =
      mailbox_to_textures_.find(mailbox);
  if (it != mailbox_to_textures_.end())
    return it->second->first;

  return NULL;
}

void MailboxManagerImpl::ProduceTexture(const Mailbox& mailbox,
                                        TextureBase* texture) {
  MailboxToTextureMap::iterator it = mailbox_to_textures_.find(mailbox);
  if (it != mailbox_to_textures_.end()) {
    if (it->second->first == texture)
      return;
    TextureToMailboxMap::iterator texture_it = it->second;
    mailbox_to_textures_.erase(it);
    textures_to_mailboxes_.erase(texture_it);
  }
  if (texture)
    InsertTexture(mailbox, texture);
}

void MailboxManagerImpl::InsertTexture(const Mailbox& mailbox,
                                       TextureBase* texture) {
  texture->SetMailboxManager(this);
  TextureToMailboxMap::iterator texture_it =
      textures_to_mailboxes_.insert(std::make_pair(texture, mailbox));
  mailbox_to_textures_.insert(std::make_pair(mailbox, texture_it));
  DCHECK_EQ(mailbox_to_textures_.size(), textures_to_mailboxes_.size());
}

void MailboxManagerImpl::TextureDeleted(TextureBase* texture) {
  std::pair<TextureToMailboxMap::iterator,
            TextureToMailboxMap::iterator> range =
      textures_to_mailboxes_.equal_range(texture);
  for (TextureToMailboxMap::iterator it = range.first;
       it != range.second; ++it) {
    size_t count = mailbox_to_textures_.erase(it->second);
    DCHECK(count == 1);
  }
  textures_to_mailboxes_.erase(range.first, range.second);
  DCHECK_EQ(mailbox_to_textures_.size(), textures_to_mailboxes_.size());
}

}  // namespace gles2
}  // namespace gpu
