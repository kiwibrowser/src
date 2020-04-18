// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/test_password_generation_agent.h"

namespace autofill {

TestPasswordGenerationAgent::TestPasswordGenerationAgent(
    content::RenderFrame* render_frame,
    PasswordAutofillAgent* password_agent,
    service_manager::BinderRegistry* registry)
    : PasswordGenerationAgent(render_frame, password_agent, registry) {
  // Always enable when testing.
  set_enabled(true);
}

TestPasswordGenerationAgent::~TestPasswordGenerationAgent() {}

bool TestPasswordGenerationAgent::ShouldAnalyzeDocument() {
  return true;
}

}  // namespace autofill
