// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/common/password_manager_features.h"

namespace password_manager {

namespace features {

// Enable affiliation based matching, so that credentials stored for an Android
// application will also be considered matches for, and be filled into
// corresponding Web applications.
const base::Feature kAffiliationBasedMatching = {
    "AffiliationBasedMatching", base::FEATURE_ENABLED_BY_DEFAULT};

// Use HTML based username detector.
const base::Feature kHtmlBasedUsernameDetector = {
    "HtmlBaseUsernameDetector", base::FEATURE_ENABLED_BY_DEFAULT};

// Enable additional elements in the form popup UI, which will allow the user to
// view all saved passwords.
const base::Feature kEnableManualFallbacksGeneration = {
    "EnableManualFallbacksGeneration", base::FEATURE_DISABLED_BY_DEFAULT};

// Enable additional elements in the form popup UI, which will allow the user to
// trigger generation or view all saved passwords.
const base::Feature kManualFallbacksFilling = {
    "ManualFallbacksFilling", base::FEATURE_DISABLED_BY_DEFAULT};

// Enable a standalone popup UI, which will allow the user to view all saved
// passwords.
const base::Feature kEnableManualFallbacksFillingStandalone = {
    "EnableManualFallbacksFillingStandalone",
    base::FEATURE_DISABLED_BY_DEFAULT};

// Enable a context menu item in the password field that allows the user
// to manually enforce saving of their password.
const base::Feature kPasswordForceSaving = {
    "PasswordForceSaving", base::FEATURE_DISABLED_BY_DEFAULT};

// Enable the user to trigger password generation manually.
const base::Feature kEnableManualPasswordGeneration = {
    "enable-manual-password-generation", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables the "Show all saved passwords" option in Context Menu.
const base::Feature kEnableShowAllSavedPasswordsContextMenu{
    "kEnableShowAllSavedPasswordsContextMenu",
    base::FEATURE_ENABLED_BY_DEFAULT};

// Disallow autofilling of the sync credential.
const base::Feature kProtectSyncCredential = {
    "protect-sync-credential", base::FEATURE_DISABLED_BY_DEFAULT};

// Disallow autofilling of the sync credential only for transactional reauth
// pages.
const base::Feature kProtectSyncCredentialOnReauth = {
    "ProtectSyncCredentialOnReauth", base::FEATURE_DISABLED_BY_DEFAULT};

// Controls the ability to export passwords from Chrome's settings page.
const base::Feature kPasswordExport = {"PasswordExport",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

// Controls the ability to import passwords from Chrome's settings page.
const base::Feature kPasswordImport = {"PasswordImport",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

// Allows searching for saved passwords in the settings page on mobile devices.
const base::Feature kPasswordSearchMobile = {"PasswordSearchMobile",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

// Adds password-related features to the keyboard accessory on mobile devices.
const base::Feature kPasswordsKeyboardAccessory = {
    "PasswordsKeyboardAccessory", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables the experiment for the password manager to only fill on account
// selection, rather than autofilling on page load, with highlighting of fields.
const base::Feature kFillOnAccountSelect = {"fill-on-account-select",
                                            base::FEATURE_DISABLED_BY_DEFAULT};
// Enables new password form parsing mechanism, details in
// go/new-cpm-design-refactoring.
const base::Feature kNewPasswordFormParsing = {
    "new-password-form-parsing", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features

}  // namespace password_manager
