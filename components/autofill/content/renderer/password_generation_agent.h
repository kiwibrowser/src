// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_PASSWORD_GENERATION_AGENT_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_PASSWORD_GENERATION_AGENT_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "components/autofill/content/common/autofill_agent.mojom.h"
#include "components/autofill/content/common/autofill_driver.mojom.h"
#include "components/autofill/content/renderer/renderer_save_password_progress_logger.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/web/web_input_element.h"
#include "url/gurl.h"

namespace autofill {

struct PasswordForm;
struct PasswordFormGenerationData;
class PasswordAutofillAgent;

// This class is responsible for controlling communication for password
// generation between the browser (which shows the popup and generates
// passwords) and WebKit (shows the generation icon in the password field).
class PasswordGenerationAgent : public content::RenderFrameObserver,
                                public mojom::PasswordGenerationAgent {
 public:
  PasswordGenerationAgent(content::RenderFrame* render_frame,
                          PasswordAutofillAgent* password_agent,
                          service_manager::BinderRegistry* registry);
  ~PasswordGenerationAgent() override;

  void BindRequest(mojom::PasswordGenerationAgentRequest request);

  // mojom::PasswordGenerationAgent:
  void FormNotBlacklisted(const PasswordForm& form) override;
  void GeneratedPasswordAccepted(const base::string16& password) override;
  void FoundFormsEligibleForGeneration(
      const std::vector<PasswordFormGenerationData>& forms) override;
  // Sets |generation_element_| to the focused password field and shows a
  // generation popup at this field.
  void UserTriggeredGeneratePassword() override;
  void UserSelectedManualGenerationOption() override;

  // Enables the form classifier.
  void AllowToRunFormClassifier() override;

  // Returns true if the field being changed is one where a generated password
  // is being offered. Updates the state of the popup if necessary.
  bool TextDidChangeInTextField(const blink::WebInputElement& element);

  // Returns true if the newly focused node caused the generation UI to show.
  bool FocusedNodeHasChanged(const blink::WebNode& node);

  // Called when new form controls are inserted.
  void OnDynamicFormsSeen();

  // Called right before PasswordAutofillAgent filled |password_element|.
  void OnFieldAutofilled(const blink::WebInputElement& password_element);

  // The length that a password can be before the UI is hidden.
  static const size_t kMaximumOfferSize = 5;

 protected:
  // Returns true if the document for |render_frame()| is one that we should
  // consider analyzing. Virtual so that it can be overriden during testing.
  virtual bool ShouldAnalyzeDocument();

  // Use to force enable during testing.
  void set_enabled(bool enabled) { enabled_ = enabled; }

 private:
  struct AccountCreationFormData {
    linked_ptr<PasswordForm> form;
    std::vector<blink::WebInputElement> password_elements;

    AccountCreationFormData(
        linked_ptr<PasswordForm> form,
        std::vector<blink::WebInputElement> password_elements);
    AccountCreationFormData(const AccountCreationFormData& other);
    ~AccountCreationFormData();
  };

  typedef std::vector<AccountCreationFormData> AccountCreationFormDataList;

  // RenderFrameObserver:
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_document_navigation) override;
  void DidFinishDocumentLoad() override;
  void DidFinishLoad() override;
  void OnDestruct() override;

  const mojom::PasswordManagerDriverPtr& GetPasswordManagerDriver();

  const mojom::PasswordManagerClientAssociatedPtr& GetPasswordManagerClient();

  // Helper function that will try and populate |password_elements_| and
  // |possible_account_creation_form_|.
  void FindPossibleGenerationForm();

  // Helper function to decide if |passwords_| contains password fields for
  // an account creation form. Sets |generation_element_| to the field that
  // we want to trigger the generation UI on.
  void DetermineGenerationElement();

  // Helper function which takes care of the form processing and collecting the
  // information which is required to show the generation popup. Returns true if
  // all required information is collected.
  bool SetUpUserTriggeredGeneration();

  // Show password generation UI anchored at |generation_element_|.
  void ShowGenerationPopup();

  // Show UI for editing a generated password at |generation_element_|.
  void ShowEditingPopup();

  // Hides a password generation popup if one exists.
  void HidePopup();

  // Stops treating a password as generated.
  void PasswordNoLongerGenerated();

  // Runs HTML parsing based classifier and saves its outcome to proto.
  // TODO(crbug.com/621442): Remove client-side form classifier when server-side
  // classifier is ready.
  void RunFormClassifierAndSaveVote(const blink::WebFormElement& web_form,
                                    const PasswordForm& form);

  void LogMessage(autofill::SavePasswordProgressLogger::StringID message_id);
  void LogBoolean(autofill::SavePasswordProgressLogger::StringID message_id,
                  bool truth_value);
  void LogNumber(autofill::SavePasswordProgressLogger::StringID message_id,
                 int number);

  // Creates a password form to presave a generated password. It copies behavior
  // of CreatePasswordFormFromWebForm/FromUnownedInputElements, but takes
  // |password_value| from |generation_element_| and empties |username_value|.
  // If a form creating is failed, returns an empty unique_ptr.
  std::unique_ptr<PasswordForm> CreatePasswordFormToPresave();

  // Stores forms that are candidates for account creation.
  AccountCreationFormDataList possible_account_creation_forms_;

  // Stores the origins of the password forms confirmed not to be blacklisted
  // by the browser. A form can be blacklisted if a user chooses "never save
  // passwords for this site".
  std::vector<GURL> not_blacklisted_password_form_origins_;

  // Stores each password form for which the Autofill server classifies one of
  // the form's fields as an ACCOUNT_CREATION_PASSWORD or NEW_PASSWORD. These
  // forms will not be sent if the feature is disabled.
  std::vector<autofill::PasswordFormGenerationData> generation_enabled_forms_;

  // Data for form which generation is allowed on.
  std::unique_ptr<AccountCreationFormData> generation_form_data_;

  // Element where we want to trigger password generation UI.
  blink::WebInputElement generation_element_;

  // Password element that had focus last. Since Javascript could change focused
  // element after the user triggered a generation request, it is better to save
  // the last focused password element.
  blink::WebInputElement last_focused_password_element_;

  // If the password field at |generation_element_| contains a generated
  // password.
  bool password_is_generated_;

  // True if password generation was manually triggered.
  bool is_manually_triggered_;

  // True if a password was generated and the user edited it. Used for UMA
  // stats.
  bool password_edited_;

  // True if the generation popup was shown during this navigation. Used to
  // track UMA stats per page visit rather than per display, since the former
  // is more interesting.
  bool generation_popup_shown_;

  // True if the editing popup was shown during this navigation. Used to track
  // UMA stats per page rather than per display, since the former is more
  // interesting.
  bool editing_popup_shown_;

  // If this feature is enabled. Controlled by Finch.
  bool enabled_;

  // If the form classifier should run.
  bool form_classifier_enabled_;

  // True iff the generation element should be marked with special HTML
  // attribute (only for experimental purposes).
  bool mark_generation_element_;

  // Unowned pointer. Used to notify PassowrdAutofillAgent when values
  // in password fields are updated.
  PasswordAutofillAgent* password_agent_;

  mojom::PasswordManagerClientAssociatedPtr password_manager_client_;

  mojo::Binding<mojom::PasswordGenerationAgent> binding_;

  DISALLOW_COPY_AND_ASSIGN(PasswordGenerationAgent);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_PASSWORD_GENERATION_AGENT_H_
