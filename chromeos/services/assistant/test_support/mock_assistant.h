// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_ASSISTANT_TEST_SUPPORT_MOCK_ASSISTANT_H_
#define CHROMEOS_SERVICES_ASSISTANT_TEST_SUPPORT_MOCK_ASSISTANT_H_

#include "base/macros.h"
#include "chromeos/services/assistant/public/mojom/assistant.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {
namespace assistant {

class MockAssistant : public mojom::Assistant {
 public:
  MockAssistant();
  ~MockAssistant() override;

  MOCK_METHOD0(StartVoiceInteraction, void());

  MOCK_METHOD0(StopActiveInteraction, void());

  MOCK_METHOD1(SendTextQuery, void(const std::string&));

  MOCK_METHOD1(AddAssistantEventSubscriber,
               void(chromeos::assistant::mojom::AssistantEventSubscriberPtr));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAssistant);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_ASSISTANT_TEST_SUPPORT_MOCK_ASSISTANT_H_
