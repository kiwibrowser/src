// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_AUTOFILL_FAKE_PASSWORD_MANAGER_CLIENT_H_
#define CHROME_RENDERER_AUTOFILL_FAKE_PASSWORD_MANAGER_CLIENT_H_

#include <string>
#include <vector>

#include "base/optional.h"
#include "base/strings/string16.h"
#include "components/autofill/content/common/autofill_driver.mojom.h"
#include "components/autofill/core/common/password_form.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

class FakePasswordManagerClient
    : public autofill::mojom::PasswordManagerClient {
 public:
  FakePasswordManagerClient();

  ~FakePasswordManagerClient() override;

  void BindRequest(
      autofill::mojom::PasswordManagerClientAssociatedRequest request);

  void Flush();

  bool called_show_pw_generation_popup() const {
    return called_show_pw_generation_popup_;
  }

  bool called_generation_available_for_form() const {
    return called_generation_available_for_form_;
  }

  bool called_hide_pw_generation_popup() const {
    return called_hide_pw_generation_popup_;
  }

  void reset_called_show_pw_generation_popup() {
    called_show_pw_generation_popup_ = false;
  }

  void reset_called_generation_available_for_form() {
    called_generation_available_for_form_ = false;
  }

  void reset_called_hide_pw_generation_popup() {
    called_hide_pw_generation_popup_ = false;
  }

 private:
  // autofill::mojom::PasswordManagerClient:
  void ShowPasswordGenerationPopup(const gfx::RectF& bounds,
                                   int max_length,
                                   const base::string16& generation_element,
                                   bool is_manually_triggered,
                                   const autofill::PasswordForm& form) override;

  void ShowPasswordEditingPopup(const gfx::RectF& bounds,
                                const autofill::PasswordForm& form) override;

  void GenerationAvailableForForm(const autofill::PasswordForm& form) override;

  void HidePasswordGenerationPopup() override;

  // Records whether ShowPasswordGenerationPopup() gets called.
  bool called_show_pw_generation_popup_ = false;

  // Records whether GenerationAvailableForForm() gets called.
  bool called_generation_available_for_form_ = false;

  // Records whether HidePasswordGenerationPopup() gets called.
  bool called_hide_pw_generation_popup_ = false;

  mojo::AssociatedBinding<autofill::mojom::PasswordManagerClient> binding_;
};

#endif  // CHROME_RENDERER_AUTOFILL_FAKE_PASSWORD_MANAGER_CLIENT_H_
