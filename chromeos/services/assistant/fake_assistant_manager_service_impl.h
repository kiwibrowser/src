// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_ASSISTANT_FAKE_ASSISTANT_MANAGER_SERVICE_IMPL_H_
#define CHROMEOS_SERVICES_ASSISTANT_FAKE_ASSISTANT_MANAGER_SERVICE_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chromeos/services/assistant/assistant_manager_service.h"
#include "chromeos/services/assistant/public/mojom/assistant.mojom.h"

namespace chromeos {
namespace assistant {

// Stub implementation of AssistantManagerService.  Should return deterministic
// result for testing.
class FakeAssistantManagerServiceImpl : public AssistantManagerService {
 public:
  FakeAssistantManagerServiceImpl();
  ~FakeAssistantManagerServiceImpl() override;

  // assistant::AssistantManagerService overrides
  void Start(const std::string& access_token,
             base::OnceClosure callback) override;
  void SetAccessToken(const std::string& access_token) override;
  void EnableListening(bool enable) override;
  State GetState() const override;
  AssistantSettingsManager* GetAssistantSettingsManager() override;
  void SendGetSettingsUiRequest(
      const std::string& selector,
      GetSettingsUiResponseCallback callback) override;
  void SendUpdateSettingsUiRequest(
      const std::string& update,
      UpdateSettingsUiResponseCallback callback) override;

  // mojom::AssistantEvent overrides:
  void StartVoiceInteraction() override;
  void StopActiveInteraction() override;
  void SendTextQuery(const std::string& query) override;
  void AddAssistantEventSubscriber(
      mojom::AssistantEventSubscriberPtr subscriber) override;

 private:
  State state_ = State::STOPPED;
  DISALLOW_COPY_AND_ASSIGN(FakeAssistantManagerServiceImpl);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_ASSISTANT_FAKE_ASSISTANT_MANAGER_SERVICE_IMPL_H_
