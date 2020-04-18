// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/renderer_save_password_progress_logger.h"

#include "base/strings/string16.h"
#include "base/values.h"
#include "third_party/blink/public/web/web_form_control_element.h"

namespace autofill {

RendererSavePasswordProgressLogger::RendererSavePasswordProgressLogger(
    mojom::PasswordManagerDriver* password_manager_driver)
    : password_manager_driver_(password_manager_driver) {
  DCHECK(password_manager_driver);
}

RendererSavePasswordProgressLogger::~RendererSavePasswordProgressLogger() {}

void RendererSavePasswordProgressLogger::SendLog(const std::string& log) {
  password_manager_driver_->RecordSavePasswordProgress(log);
}

void RendererSavePasswordProgressLogger::LogElementName(
    StringID label,
    const blink::WebFormControlElement& element) {
  LogValue(label,
           base::Value(ScrubElementID((element.NameForAutofill().Utf16()))));
}

}  // namespace autofill
