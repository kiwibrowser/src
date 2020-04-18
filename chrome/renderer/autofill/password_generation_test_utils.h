// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_AUTOFILL_PASSWORD_GENERATION_TEST_UTILS_H_
#define CHROME_RENDERER_AUTOFILL_PASSWORD_GENERATION_TEST_UTILS_H_

#include <string>
#include <vector>

#include "base/strings/string16.h"

namespace blink {
class WebDocument;
}

namespace autofill {

class TestPasswordGenerationAgent;

void SetNotBlacklistedMessage(TestPasswordGenerationAgent* generation_agent,
                              const char* form_str);
void SetAccountCreationFormsDetectedMessage(TestPasswordGenerationAgent* agent,
                                            blink::WebDocument document,
                                            int form_index,
                                            int field_index);

std::string CreateScriptToRegisterListeners(
    const char* const element_name,
    std::vector<base::string16>* variables_to_check);

}  // namespace autofill

#endif  // CHROME_RENDERER_AUTOFILL_PASSWORD_GENERATION_TEST_UTILS_H_
