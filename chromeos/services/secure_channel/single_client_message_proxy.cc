// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/single_client_message_proxy.h"

namespace chromeos {

namespace secure_channel {

SingleClientMessageProxy::SingleClientMessageProxy(Delegate* delegate)
    : delegate_(delegate) {}

SingleClientMessageProxy::~SingleClientMessageProxy() = default;

void SingleClientMessageProxy::NotifySendMessageRequested(
    const std::string& message_feature,
    const std::string& message_payload,
    base::OnceClosure on_sent_callback) {
  delegate_->OnSendMessageRequested(message_feature, message_payload,
                                    std::move(on_sent_callback));
}

void SingleClientMessageProxy::NotifyClientDisconnected() {
  delegate_->OnClientDisconnected(GetProxyId());
}

const mojom::ConnectionMetadata&
SingleClientMessageProxy::GetConnectionMetadataFromDelegate() {
  return delegate_->GetConnectionMetadata();
}

}  // namespace secure_channel

}  // namespace chromeos
