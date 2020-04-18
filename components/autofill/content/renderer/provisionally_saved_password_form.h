// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_PROVISIONALLY_SAVED_PASSWORD_FORM_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_PROVISIONALLY_SAVED_PASSWORD_FORM_H_

#include <memory>

#include "base/macros.h"
#include "components/autofill/core/common/password_form.h"
#include "third_party/blink/public/web/web_input_element.h"

namespace autofill {

struct PasswordForm;

// Represents a possibly submitted password form.
class ProvisionallySavedPasswordForm {
 public:
  ProvisionallySavedPasswordForm();
  ~ProvisionallySavedPasswordForm();

  // Sets the PasswordForm and web elements that were used in the PasswordForm
  // update.
  void Set(std::unique_ptr<PasswordForm> password_form,
           const blink::WebFormElement& form_element,
           const blink::WebInputElement& input_element);
  void Reset();

  // Returns true if the instance has |password_form_| set, but the actual
  // password data may be invalid (e.g. empty username or password).
  bool IsSet() const;
  // Returns true if |password_form_| has enough information that it is likely
  // filled out.
  bool IsPasswordValid() const;

  const PasswordForm& password_form() const {
    DCHECK(IsSet());
    return *password_form_;
  }
  blink::WebFormElement& form_element() { return form_element_; }
  blink::WebInputElement& input_element() { return input_element_; }

  void SetSubmissionIndicatorEvent(
      PasswordForm::SubmissionIndicatorEvent event);

 private:
  std::unique_ptr<PasswordForm> password_form_;
  // Last used WebFormElement for the PasswordForm submission. Can be null if
  // the form is unowned.
  blink::WebFormElement form_element_;
  // Last used WebInputElement which led to the PasswordForm update. Can be null
  // if the user has performed a form submission (via a button, for example).
  blink::WebInputElement input_element_;

  DISALLOW_COPY_AND_ASSIGN(ProvisionallySavedPasswordForm);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_PROVISIONALLY_SAVED_PASSWORD_FORM_H_
