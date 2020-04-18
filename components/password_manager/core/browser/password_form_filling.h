// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_FILLING_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_FILLING_H_

#include <map>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"

namespace autofill {
struct PasswordForm;
}  // namespace autofill

namespace password_manager {
class PasswordManagerClient;
class PasswordManagerDriver;
class PasswordFormMetricsRecorder;

void SendFillInformationToRenderer(
    const PasswordManagerClient& client,
    PasswordManagerDriver* driver,
    bool is_blacklisted,
    const autofill::PasswordForm& observed_form,
    const std::map<base::string16, const autofill::PasswordForm*>& best_matches,
    const std::vector<const autofill::PasswordForm*>& federated_matches,
    const autofill::PasswordForm* preferred_match,
    PasswordFormMetricsRecorder* metrics_recorder);

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_FILLING_H_
