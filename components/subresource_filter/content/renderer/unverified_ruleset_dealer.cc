// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/renderer/unverified_ruleset_dealer.h"

#include "components/subresource_filter/content/common/subresource_filter_messages.h"
#include "ipc/ipc_message_macros.h"

namespace subresource_filter {

UnverifiedRulesetDealer::UnverifiedRulesetDealer() = default;
UnverifiedRulesetDealer::~UnverifiedRulesetDealer() = default;

bool UnverifiedRulesetDealer::OnControlMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(UnverifiedRulesetDealer, message)
    IPC_MESSAGE_HANDLER(SubresourceFilterMsg_SetRulesetForProcess,
                        OnSetRulesetForProcess)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void UnverifiedRulesetDealer::OnSetRulesetForProcess(
    const IPC::PlatformFileForTransit& platform_file) {
  SetRulesetFile(IPC::PlatformFileForTransitToFile(platform_file));
}

}  // namespace subresource_filter
