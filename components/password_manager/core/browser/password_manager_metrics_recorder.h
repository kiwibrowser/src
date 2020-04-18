// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_METRICS_RECORDER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_METRICS_RECORDER_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "url/gurl.h"

class GURL;

namespace ukm {
namespace builders {
class PageWithPassword;
}
}  // namespace ukm

namespace password_manager {

class BrowserSavePasswordProgressLogger;

// The pupose of this class is to record various types of metrics about the
// behavior of the PasswordManager and its interaction with the user and the
// page.
// The PasswordManagerMetricsRecorder flushes metrics on destruction. As such
// any owner needs to destroy this instance when navigations are committed.
class PasswordManagerMetricsRecorder {
 public:
  // Reasons why the password manager failed to do a provisional saving and
  // therefore did not offer the user to save a password.
  enum ProvisionalSaveFailure {
    // Password manager is disabled or user is in incognito mode.
    SAVING_DISABLED,
    // Submitted form contains an empty password.
    EMPTY_PASSWORD,
    // No PasswordFormManager exists for this form.
    NO_MATCHING_FORM,
    // FormFetcher of PasswordFormManager is still loading.
    MATCHING_NOT_COMPLETE,
    // Form is blacklisted for saving. Obsolete since M47.
    FORM_BLACKLISTED,
    // <unknown purpose>. Obsolete since M48.
    INVALID_FORM,
    // A Google credential cannot be saved by policy because it is the Chrome
    // Sync credential and therefore acts as a master password that gives access
    // to all other credentials on https://passwords.google.com.
    SYNC_CREDENTIAL,
    // Credentials are not offered to be saved on HTTP pages if a credential is
    // stored for the corresponding HTTPS page.
    SAVING_ON_HTTP_AFTER_HTTPS,
    MAX_FAILURE_VALUE
  };

  // This enum represents user actions on a page with a password form that
  // cannot (reliably) be attributed to a specific form manager.
  enum class PageLevelUserAction {
    kUnknown = 0,

    // User chose to open the password viewer as part of a manual fallback.
    kShowAllPasswordsWhileSomeAreSuggested,
    kShowAllPasswordsWhileNoneAreSuggested,
  };

  // Records UKM metrics and reports them on destruction.
  PasswordManagerMetricsRecorder(ukm::SourceId source_id,
                                 const GURL& main_frame_url);

  PasswordManagerMetricsRecorder(
      PasswordManagerMetricsRecorder&& that) noexcept;
  ~PasswordManagerMetricsRecorder();

  PasswordManagerMetricsRecorder& operator=(
      PasswordManagerMetricsRecorder&& that);

  // Records that the user has modified a password field on a page. This may be
  // called multiple times but a single metric will be reported.
  void RecordUserModifiedPasswordField();

  // Log failure to provisionally save a password to in the PasswordManager to
  // UMA and the |logger|.
  void RecordProvisionalSaveFailure(ProvisionalSaveFailure failure,
                                    const GURL& main_frame_url,
                                    const GURL& form_origin,
                                    BrowserSavePasswordProgressLogger* logger);

  // Records a user action.
  void RecordPageLevelUserAction(PageLevelUserAction action);

 private:
  // URL for which UKMs are reported.
  GURL main_frame_url_;

  // Records URL keyed metrics (UKMs) and submits them on its destruction.
  std::unique_ptr<ukm::builders::PageWithPassword> ukm_entry_builder_;

  bool user_modified_password_field_ = false;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerMetricsRecorder);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_METRICS_RECORDER_H_
