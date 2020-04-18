// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/fake_assistant_settings_manager_impl.h"

namespace chromeos {
namespace assistant {

FakeAssistantSettingsManagerImpl::FakeAssistantSettingsManagerImpl() = default;

FakeAssistantSettingsManagerImpl::~FakeAssistantSettingsManagerImpl() = default;

void FakeAssistantSettingsManagerImpl::GetSettings(
    const std::string& selector,
    GetSettingsCallback callback) {}

void FakeAssistantSettingsManagerImpl::BindRequest(
    mojom::AssistantSettingsManagerRequest request) {}

}  // namespace assistant
}  // namespace chromeos
