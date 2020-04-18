// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_SCREEN_EXIT_CODE_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_SCREEN_EXIT_CODE_H_

#include <string>

#include "base/callback.h"

namespace chromeos {

enum class AssistantOptInScreenExitCode {
  VALUE_PROP_SKIPPED = 0,
  VALUE_PROP_ACCEPTED = 1,
  EXIT_CODES_COUNT
};

using OnAssistantOptInScreenExitCallback =
    base::OnceCallback<void(AssistantOptInScreenExitCode exit_code)>;

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_SCREEN_EXIT_CODE_H_
