// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/autofill/fake_password_manager_client.h"

FakePasswordManagerClient::FakePasswordManagerClient() : binding_(this) {}

FakePasswordManagerClient::~FakePasswordManagerClient() {}

void FakePasswordManagerClient::BindRequest(
    autofill::mojom::PasswordManagerClientAssociatedRequest request) {
  binding_.Bind(std::move(request));
}

void FakePasswordManagerClient::Flush() {
  if (binding_.is_bound())
    binding_.FlushForTesting();
}

// autofill::mojom::PasswordManagerClient:
void FakePasswordManagerClient::ShowPasswordGenerationPopup(
    const gfx::RectF& bounds,
    int max_length,
    const base::string16& generation_element,
    bool is_manually_triggered,
    const autofill::PasswordForm& form) {
  called_show_pw_generation_popup_ = true;
}

void FakePasswordManagerClient::ShowPasswordEditingPopup(
    const gfx::RectF& bounds,
    const autofill::PasswordForm& form) {}

void FakePasswordManagerClient::GenerationAvailableForForm(
    const autofill::PasswordForm& form) {
  called_generation_available_for_form_ = true;
}

void FakePasswordManagerClient::HidePasswordGenerationPopup() {
  called_hide_pw_generation_popup_ = true;
}
