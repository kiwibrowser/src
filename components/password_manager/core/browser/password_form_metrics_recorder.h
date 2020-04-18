// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_METRICS_RECORDER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_METRICS_RECORDER_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/autofill/core/common/password_form.h"
#include "components/autofill/core/common/signatures_util.h"
#include "components/password_manager/core/browser/password_form_user_action.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "url/gurl.h"

namespace password_manager {

class FormFetcher;

// The pupose of this class is to record various types of metrics about the
// behavior of the PasswordFormManager and its interaction with the user and
// the page. The recorder tracks events tied to the logical life of a password
// form, from parsing to having been saved. These events happen on different
// places in the code and the logical password form can be captured by multiple
// instances of real objects. To allow sharing the single recorder object among
// those, this class is refcounted. Reporting happens on destruction of the
// metrics recorder. Note that UKM metrics are reported for intervals of length
// metrics::GetUploadInterval(). Only metrics that are reported from the time
// of creating the PasswordFormMetricsRecorder until the end of current upload
// interval are recorded. Everything after the end of the current upload
// interval is discarded. For this reason, it is essential that references are
// not just kept until browser shutdown.
class PasswordFormMetricsRecorder
    : public base::RefCounted<PasswordFormMetricsRecorder> {
 public:
  // Records UKM metrics and reports them on destruction. The |source_id| is
  // the ID of the WebContents document that the forms belong to.
  PasswordFormMetricsRecorder(bool is_main_frame_secure,
                              ukm::SourceId source_id);

  // ManagerAction - What does the PasswordFormManager do with this form? Either
  // it fills it, or it doesn't. If it doesn't fill it, that's either
  // because it has no match or it is disabled via the AUTOCOMPLETE=off
  // attribute. Note that if we don't have an exact match, we still provide
  // candidates that the user may end up choosing.
  enum ManagerAction {
    kManagerActionNone = 0,
    kManagerActionAutofilled,
    kManagerActionBlacklisted_Obsolete,
    kManagerActionMax
  };

  // Same as above without the obsoleted 'Blacklisted' action.
  enum ManagerActionNew {
    kManagerActionNewNone = 0,
    kManagerActionNewAutofilled,
    kManagerActionNewMax
  };

  // Result - What happens to the form?
  enum SubmitResult {
    kSubmitResultNotSubmitted = 0,
    kSubmitResultFailed,
    kSubmitResultPassed,
    kSubmitResultMax
  };

  // Whether the password manager filled a credential on a form.
  enum ManagerAutofillEvent {
    // No credential existed that could be filled into a password form.
    kManagerFillEventNoCredential = 0,
    // A credential could have been autofilled into a password form but was not
    // due to a policy. E.g. incognito mode requires a user interaction before
    // filling can happen. PSL matches are not autofilled and also on password
    // change forms we do not autofill.
    kManagerFillEventBlockedOnInteraction,
    // A credential was autofilled into a form.
    kManagerFillEventAutofilled
  };

  // Enumerates whether there were `suppressed` credentials. These are stored
  // credentials that were not filled, even though they might be related to the
  // observed form. See FormFetcher::GetSuppressed* for details.
  //
  // If suppressed credentials exist, it is also recorded whether their username
  // and/or password matched those submitted.
  enum SuppressedAccountExistence {
    kSuppressedAccountNone,
    // Recorded when there exists a suppressed account, but there was no
    // submitted form to compare its username and password to.
    kSuppressedAccountExists,
    // Recorded when there was a submitted form.
    kSuppressedAccountExistsDifferentUsername,
    kSuppressedAccountExistsSameUsername,
    kSuppressedAccountExistsSameUsernameAndPassword,
    kSuppressedAccountExistenceMax,
  };

  // What the form is used for. kSubmittedFormTypeUnspecified is only set before
  // the SetSubmittedFormType() is called, and should never be actually
  // uploaded.
  enum SubmittedFormType {
    kSubmittedFormTypeLogin,
    kSubmittedFormTypeLoginNoUsername,
    kSubmittedFormTypeChangePasswordEnabled,
    kSubmittedFormTypeChangePasswordDisabled,
    kSubmittedFormTypeChangePasswordNoUsername,
    kSubmittedFormTypeSignup,
    kSubmittedFormTypeSignupNoUsername,
    kSubmittedFormTypeLoginAndSignup,
    kSubmittedFormTypeUnspecified,
    kSubmittedFormTypeMax
  };

  // The reason why a password bubble was shown on the screen.
  enum class BubbleTrigger {
    kUnknown = 0,
    // The password manager suggests the user to save a password and asks for
    // confirmation.
    kPasswordManagerSuggestionAutomatic,
    kPasswordManagerSuggestionManual,
    // The site asked the user to save a password via the credential management
    // API.
    kCredentialManagementAPIAutomatic,
    kCredentialManagementAPIManual,
    kMax,
  };

  // The reason why a password bubble was dismissed.
  enum class BubbleDismissalReason {
    kUnknown = 0,
    kAccepted = 1,
    kDeclined = 2,
    kIgnored = 3
  };

  // This enum is a designed to be able to collect all kinds of potentially
  // interesting user interactions with sites and password manager UI in
  // relation to a given form. In contrast to UserAction, it is intended to be
  // extensible.
  enum class DetailedUserAction {
    // Interactions with password bubble.
    kEditedUsernameInBubble = 100,
    kSelectedDifferentPasswordInBubble = 101,
    kTriggeredManualFallbackForSaving = 102,
    kObsoleteTriggeredManualFallbackForUpdating = 103,  // unused

    // Interactions with form.
    kCorrectedUsernameInForm = 200,
  };

  // The maximum number of combinations of the ManagerAction, UserAction and
  // SubmitResult enums.
  // This is used when recording the actions taken by the form in UMA.
  static constexpr int kMaxNumActionsTaken =
      kManagerActionMax * static_cast<int>(UserAction::kMax) * kSubmitResultMax;

  // Same as above but for ManagerActionNew instead of ManagerAction.
  static constexpr int kMaxNumActionsTakenNew =
      kManagerActionNewMax * static_cast<int>(UserAction::kMax) *
      kSubmitResultMax;

  // The maximum number of combinations recorded into histograms in the
  // PasswordManager.SuppressedAccount.* family.
  static constexpr int kMaxSuppressedAccountStats =
      kSuppressedAccountExistenceMax *
      PasswordFormMetricsRecorder::kManagerActionNewMax *
      static_cast<int>(UserAction::kMax) *
      PasswordFormMetricsRecorder::kSubmitResultMax;

  // Called if the user could generate a password for this form.
  void MarkGenerationAvailable();

  // Stores whether the form has had its password auto generated by the browser.
  void SetHasGeneratedPassword(bool has_generated_password);

  // Stores the password manager and user actions and logs them.
  void SetManagerAction(ManagerAction manager_action);
  void SetUserAction(UserAction user_action);

  // Call these if/when we know the form submission worked or failed.
  // These routines are used to update internal statistics ("ActionsTaken").
  void LogSubmitPassed();
  void LogSubmitFailed();

  // Call this once the submitted form type has been determined.
  void SetSubmittedFormType(SubmittedFormType form_type);

  // Call this when a password is saved to indicate which path led to
  // submission.
  void SetSubmissionIndicatorEvent(
      autofill::PasswordForm::SubmissionIndicatorEvent event);

  // Records all histograms in the PasswordManager.SuppressedAccount.* family.
  // Takes the FormFetcher intance which owns the login data from PasswordStore.
  // |pending_credentials| stores credentials when the form was submitted but
  // success was still unknown. It contains credentials that are ready to be
  // written (saved or updated) to a password store.
  void RecordHistogramsOnSuppressedAccounts(
      bool observed_form_origin_has_cryptographic_scheme,
      const FormFetcher& form_fetcher,
      const autofill::PasswordForm& pending_credentials);

  // Records the event that a password bubble was shown.
  void RecordPasswordBubbleShown(
      metrics_util::CredentialSourceType credential_source_type,
      metrics_util::UIDisplayDisposition display_disposition);

  // Records the dismissal of a password bubble.
  void RecordUIDismissalReason(
      metrics_util::UIDismissalReason ui_dismissal_reason);

  // Records that the password manager managed or failed to fill a form.
  void RecordFillEvent(ManagerAutofillEvent event);

  // Converts the "ActionsTaken" fields (using ManagerActionNew) into an int so
  // they can be logged to UMA.
  // Public for testing.
  int GetActionsTakenNew() const;

  // Records a DetailedUserAction UKM metric.
  void RecordDetailedUserAction(DetailedUserAction action);

  // Hash algorithm for RecordFormSignature. Public for testing.
  static int64_t HashFormSignature(autofill::FormSignature form_signature);

  // Records a low entropy hash of the form signature in order to be able to
  // distinguish two forms on the same site.
  void RecordFormSignature(autofill::FormSignature form_signature);

 private:
  friend class base::RefCounted<PasswordFormMetricsRecorder>;

  // Enum to track which password bubble is currently being displayed.
  enum class CurrentBubbleOfInterest {
    // This covers the cases that no password bubble is currently being
    // displayed or the one displayed is none of the interesting cases.
    kNone,
    // The user is currently seeing a password save bubble.
    kSaveBubble,
    // The user is currently seeing a password update bubble.
    kUpdateBubble,
  };

  // Destructor reports a couple of UMA metrics as well as calls
  // RecordUkmMetric.
  ~PasswordFormMetricsRecorder();

  // Converts the "ActionsTaken" fields into an int so they can be logged to
  // UMA.
  int GetActionsTaken() const;

  // When supplied with the list of all |suppressed_forms| that belong to
  // certain suppressed credential type (see FormFetcher::GetSuppressed*),
  // filters that list down to forms whose type matches |manual_or_generated|,
  // and selects the suppressed account that matches |pending_credentials| most
  // closely. |pending_credentials| stores credentials when the form was
  // submitted but success was still unknown. It contains credentials that are
  // ready to be written (saved or updated) to a password store.
  SuppressedAccountExistence GetBestMatchingSuppressedAccount(
      const std::vector<const autofill::PasswordForm*>& suppressed_forms,
      autofill::PasswordForm::Type manual_or_generated,
      const autofill::PasswordForm& pending_credentials) const;

  // Encodes a UMA histogram sample for |best_matching_account| and
  // GetActionsTakenNew(). This is a mixed-based representation of a combination
  // of four attributes:
  //  -- whether there were suppressed credentials (and if so, their relation to
  //     the submitted username/password).
  //  -- whether the |observed_form_| got ultimately submitted
  //  -- what action the password manager performed (|manager_action_|),
  //  -- and what action the user performed (|user_action_|_).
  int GetHistogramSampleForSuppressedAccounts(
      SuppressedAccountExistence best_matching_account) const;

  // True if the main frame's visible URL, at the time this PasswordFormManager
  // was created, is secure.
  const bool is_main_frame_secure_;

  // Whether the user can choose to generate a password for this form.
  bool generation_available_ = false;

  // Whether this form has an auto generated password.
  bool has_generated_password_ = false;

  // Tracks which bubble is currently being displayed to the user.
  CurrentBubbleOfInterest current_bubble_ = CurrentBubbleOfInterest::kNone;

  // Whether the user was shown a prompt to update a password.
  bool update_prompt_shown_ = false;

  // Whether the user was shown a prompt to save a new credential.
  bool save_prompt_shown_ = false;

  // These three fields record the "ActionsTaken" by the browser and
  // the user with this form, and the result. They are combined and
  // recorded in UMA when the PasswordFormMetricsRecorder is destroyed.
  ManagerAction manager_action_ = kManagerActionNone;
  UserAction user_action_ = UserAction::kNone;
  SubmitResult submit_result_ = kSubmitResultNotSubmitted;

  // Form type of the form that the PasswordFormManager is managing. Set after
  // submission as the classification of the form can change depending on what
  // data the user has entered.
  SubmittedFormType submitted_form_type_ = kSubmittedFormTypeUnspecified;

  // The UKM SourceId of the document the form belongs to.
  ukm::SourceId source_id_;

  // Holds URL keyed metrics (UKMs) to be recorded on destruction.
  ukm::builders::PasswordForm ukm_entry_builder_;

  // Counter for DetailedUserActions observed during the lifetime of a
  // PasswordFormManager. Reported upon destruction.
  std::map<DetailedUserAction, int64_t> detailed_user_actions_counts_;

  DISALLOW_COPY_AND_ASSIGN(PasswordFormMetricsRecorder);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_METRICS_RECORDER_H_
