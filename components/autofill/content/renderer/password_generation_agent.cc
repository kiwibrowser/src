// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/password_generation_agent.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/form_classifier.h"
#include "components/autofill/content/renderer/password_autofill_agent.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_form.h"
#include "components/autofill/core/common/password_form_generation_data.h"
#include "components/autofill/core/common/password_generation_util.h"
#include "components/autofill/core/common/signatures_util.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "google_apis/gaia/gaia_urls.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_form_element.h"
#include "third_party/blink/public/web/web_input_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/gfx/geometry/rect.h"

namespace autofill {

namespace {

using Logger = autofill::SavePasswordProgressLogger;

// Returns pairs of |PasswordForm| and corresponding |WebFormElement| for all
// <form>s in the frame and for unowned <input>s. The method doesn't filter out
// invalid |PasswordForm|s.
std::vector<std::pair<std::unique_ptr<PasswordForm>, blink::WebFormElement>>
GetAllPasswordFormsInFrame(PasswordAutofillAgent* password_agent,
                           blink::WebLocalFrame* web_frame) {
  blink::WebVector<blink::WebFormElement> web_forms;
  web_frame->GetDocument().Forms(web_forms);
  std::vector<std::pair<std::unique_ptr<PasswordForm>, blink::WebFormElement>>
      all_forms;
  for (const blink::WebFormElement& web_form : web_forms) {
    all_forms.emplace_back(std::make_pair(
        password_agent->GetPasswordFormFromWebForm(web_form), web_form));
  }
  all_forms.emplace_back(
      std::make_pair(password_agent->GetPasswordFormFromUnownedInputElements(),
                     blink::WebFormElement()));
  return all_forms;
}

// Returns true if we think that this form is for account creation. Password
// field(s) of the form are pushed back to |passwords|.
bool GetAccountCreationPasswordFields(
    const std::vector<blink::WebFormControlElement>& control_elements,
    std::vector<blink::WebInputElement>* passwords) {
  for (const auto& control_element : control_elements) {
    const blink::WebInputElement* input_element =
        ToWebInputElement(&control_element);
    if (input_element && input_element->IsTextField() &&
        input_element->IsPasswordFieldForAutofill()) {
      passwords->push_back(*input_element);
    }
  }
  return !passwords->empty();
}

bool ContainsURL(const std::vector<GURL>& urls, const GURL& url) {
  return base::ContainsValue(urls, url);
}

// Calculates the signature of |form| and searches it in |forms|.
const PasswordFormGenerationData* FindFormGenerationData(
    const std::vector<PasswordFormGenerationData>& forms,
    const PasswordForm& form) {
  FormSignature form_signature = CalculateFormSignature(form.form_data);
  for (const auto& form_it : forms) {
    if (form_it.form_signature == form_signature)
      return &form_it;
  }
  return nullptr;
}

// Returns a vector of up to 2 password fields with autocomplete attribute set
// to "new-password". These will be filled with the generated password.
std::vector<blink::WebInputElement> FindNewPasswordElementsMarkedBySite(
    const std::vector<blink::WebInputElement>& all_password_elements) {
  std::vector<blink::WebInputElement> passwords;

  auto is_new_password_field = [](const blink::WebInputElement& element) {
    return AutocompleteFlagForElement(element) ==
           AutocompleteFlag::NEW_PASSWORD;
  };

  auto field_iter =
      std::find_if(all_password_elements.begin(), all_password_elements.end(),
                   is_new_password_field);
  if (field_iter != all_password_elements.end()) {
    passwords.push_back(*field_iter++);
    field_iter = std::find_if(field_iter, all_password_elements.end(),
                              is_new_password_field);
    if (field_iter != all_password_elements.end())
      passwords.push_back(*field_iter);
  }

  return passwords;
}

// Returns a vector of up to 2 password fields into which Chrome should fill the
// generated password. It assumes that |field_signature| describes the field
// where Chrome shows the password generation prompt.
std::vector<blink::WebInputElement> FindPasswordElementsForGeneration(
    const std::vector<blink::WebInputElement>& all_password_elements,
    const PasswordFormGenerationData& generation_data) {
  auto generation_field_iter = all_password_elements.end();
  auto confirmation_field_iter = all_password_elements.end();
  for (auto iter = all_password_elements.begin();
       iter != all_password_elements.end(); ++iter) {
    const blink::WebInputElement& input = *iter;
    FieldSignature signature = CalculateFieldSignatureByNameAndType(
        input.NameForAutofill().Utf16(),
        input.FormControlTypeForAutofill().Utf8());
    if (signature == generation_data.field_signature) {
      generation_field_iter = iter;
    } else if (generation_data.confirmation_field_signature &&
               signature == *generation_data.confirmation_field_signature) {
      confirmation_field_iter = iter;
    }
  }

  std::vector<blink::WebInputElement> passwords;
  if (generation_field_iter != all_password_elements.end()) {
    passwords.push_back(*generation_field_iter);

    if (confirmation_field_iter == all_password_elements.end())
      confirmation_field_iter = generation_field_iter + 1;
    if (confirmation_field_iter != all_password_elements.end())
      passwords.push_back(*confirmation_field_iter);
  }
  return passwords;
}

void CopyElementValueToOtherInputElements(
    const blink::WebInputElement* element,
    std::vector<blink::WebInputElement>* elements) {
  for (blink::WebInputElement& it : *elements) {
    if (*element != it) {
      it.SetAutofillValue(element->Value());
    }
  }
}

}  // namespace

PasswordGenerationAgent::AccountCreationFormData::AccountCreationFormData(
    linked_ptr<PasswordForm> password_form,
    std::vector<blink::WebInputElement> passwords)
    : form(password_form), password_elements(std::move(passwords)) {}

PasswordGenerationAgent::AccountCreationFormData::AccountCreationFormData(
    const AccountCreationFormData& other) = default;

PasswordGenerationAgent::AccountCreationFormData::~AccountCreationFormData() {}

PasswordGenerationAgent::PasswordGenerationAgent(
    content::RenderFrame* render_frame,
    PasswordAutofillAgent* password_agent,
    service_manager::BinderRegistry* registry)
    : content::RenderFrameObserver(render_frame),
      password_is_generated_(false),
      is_manually_triggered_(false),
      password_edited_(false),
      generation_popup_shown_(false),
      editing_popup_shown_(false),
      enabled_(password_generation::IsPasswordGenerationEnabled()),
      form_classifier_enabled_(false),
      mark_generation_element_(
          base::CommandLine::ForCurrentProcess()->HasSwitch(
              switches::kShowAutofillSignatures)),
      password_agent_(password_agent),
      binding_(this) {
  LogBoolean(Logger::STRING_GENERATION_RENDERER_ENABLED, enabled_);
  registry->AddInterface(base::Bind(&PasswordGenerationAgent::BindRequest,
                                    base::Unretained(this)));
  password_agent_->SetPasswordGenerationAgent(this);
}
PasswordGenerationAgent::~PasswordGenerationAgent() {}

void PasswordGenerationAgent::BindRequest(
    mojom::PasswordGenerationAgentRequest request) {
  binding_.Bind(std::move(request));
}

void PasswordGenerationAgent::DidCommitProvisionalLoad(
    bool /*is_new_navigation*/, bool is_same_document_navigation) {
  if (is_same_document_navigation)
    return;
  generation_element_.Reset();
  last_focused_password_element_.Reset();
}

void PasswordGenerationAgent::DidFinishDocumentLoad() {
  // Update stats for main frame navigation.
  if (!render_frame()->GetWebFrame()->Parent()) {
    // In every navigation, the IPC message sent by the password autofill
    // manager to query whether the current form is blacklisted or not happens
    // when the document load finishes, so we need to clear previous states
    // here before we hear back from the browser. We only clear this state on
    // main frame load as we don't want subframe loads to clear state that we
    // have received from the main frame. Note that we assume there is only one
    // account creation form, but there could be multiple password forms in
    // each frame.
    not_blacklisted_password_form_origins_.clear();
    generation_enabled_forms_.clear();
    generation_element_.Reset();
    possible_account_creation_forms_.clear();

    // Log statistics after navigation so that we only log once per page.
    if (generation_form_data_ &&
        generation_form_data_->password_elements.empty()) {
      password_generation::LogPasswordGenerationEvent(
          password_generation::NO_SIGN_UP_DETECTED);
    } else {
      password_generation::LogPasswordGenerationEvent(
          password_generation::SIGN_UP_DETECTED);
    }
    generation_form_data_.reset();
    password_is_generated_ = false;
    if (password_edited_) {
      password_generation::LogPasswordGenerationEvent(
          password_generation::PASSWORD_EDITED);
    }
    password_edited_ = false;

    if (generation_popup_shown_) {
      password_generation::LogPasswordGenerationEvent(
          password_generation::GENERATION_POPUP_SHOWN);
    }
    generation_popup_shown_ = false;

    if (editing_popup_shown_) {
      password_generation::LogPasswordGenerationEvent(
          password_generation::EDITING_POPUP_SHOWN);
    }
    editing_popup_shown_ = false;
  }

  FindPossibleGenerationForm();
}

void PasswordGenerationAgent::DidFinishLoad() {
  // Since forms on some sites are available only at this event (but not at
  // DidFinishDocumentLoad), again call FindPossibleGenerationForm to detect
  // these forms (crbug.com/617893).
  FindPossibleGenerationForm();
}

void PasswordGenerationAgent::OnDestruct() {
  binding_.Close();
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void PasswordGenerationAgent::OnDynamicFormsSeen() {
  FindPossibleGenerationForm();
}

void PasswordGenerationAgent::OnFieldAutofilled(
    const blink::WebInputElement& password_element) {
  if (password_is_generated_ && generation_element_ == password_element)
    PasswordNoLongerGenerated();
}

void PasswordGenerationAgent::AllowToRunFormClassifier() {
  form_classifier_enabled_ = true;
}

void PasswordGenerationAgent::RunFormClassifierAndSaveVote(
    const blink::WebFormElement& web_form,
    const PasswordForm& form) {
  DCHECK(form_classifier_enabled_);

  base::string16 generation_field;
  ClassifyFormAndFindGenerationField(web_form, &generation_field);
  GetPasswordManagerDriver()->SaveGenerationFieldDetectedByClassifier(
      form, generation_field);
}

void PasswordGenerationAgent::FindPossibleGenerationForm() {
  if (!enabled_ || !render_frame())
    return;

  // We don't want to generate passwords if the browser won't store or sync
  // them.
  if (!ShouldAnalyzeDocument())
    return;

  // If we have already found a signup form for this page, no need to continue.
  if (generation_form_data_)
    return;

  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  std::vector<std::pair<std::unique_ptr<PasswordForm>, blink::WebFormElement>>
      all_password_forms =
          GetAllPasswordFormsInFrame(password_agent_, web_frame);
  for (auto& form : all_password_forms) {
    PasswordForm* password_form = form.first.get();
    // If we can't get a valid PasswordForm, we skip this form because the
    // the password won't get saved even if we generate it.
    if (!password_form) {
      LogMessage(Logger::STRING_GENERATION_RENDERER_INVALID_PASSWORD_FORM);
      continue;
    }

    // Do not generate password for GAIA since it is used to retrieve the
    // generated paswords.
    GURL realm(password_form->signon_realm);
    if (realm == GaiaUrls::GetInstance()->gaia_login_form_realm())
      continue;

    std::vector<blink::WebInputElement> passwords;
    const blink::WebFormElement& web_form = form.second;
    if (GetAccountCreationPasswordFields(
            web_form.IsNull()
                ? form_util::GetUnownedFormFieldElements(
                      web_frame->GetDocument().All(), nullptr)
                : form_util::ExtractAutofillableElementsInForm(web_form),
            &passwords)) {
      if (form_classifier_enabled_ && !web_form.IsNull())
        RunFormClassifierAndSaveVote(web_form, *password_form);
      possible_account_creation_forms_.emplace_back(
          make_linked_ptr(form.first.release()), std::move(passwords));
    }
  }

  if (!possible_account_creation_forms_.empty()) {
    LogNumber(
        Logger::STRING_GENERATION_RENDERER_POSSIBLE_ACCOUNT_CREATION_FORMS,
        possible_account_creation_forms_.size());
    DetermineGenerationElement();
  }
}

bool PasswordGenerationAgent::ShouldAnalyzeDocument() {
  // Make sure that this frame is allowed to use password manager. Generating a
  // password that can't be saved is a bad idea.
  if (!render_frame() || !password_agent_->FrameCanAccessPasswordManager()) {
    LogMessage(Logger::STRING_GENERATION_RENDERER_NO_PASSWORD_MANAGER_ACCESS);
    return false;
  }

  return true;
}

void PasswordGenerationAgent::FormNotBlacklisted(const PasswordForm& form) {
  not_blacklisted_password_form_origins_.push_back(form.origin);
  DetermineGenerationElement();
}

void PasswordGenerationAgent::GeneratedPasswordAccepted(
    const base::string16& password) {
  password_is_generated_ = true;
  password_edited_ = false;
  password_generation::LogPasswordGenerationEvent(
      password_generation::PASSWORD_ACCEPTED);
  LogMessage(Logger::STRING_GENERATION_RENDERER_GENERATED_PASSWORD_ACCEPTED);
  for (auto& password_element : generation_form_data_->password_elements) {
    password_element.SetAutofillValue(blink::WebString::FromUTF16(password));
    // setAutofillValue() above may have resulted in JavaScript closing the
    // frame.
    if (!render_frame())
      return;
    password_element.SetAutofilled(true);
    // Advance focus to the next input field. We assume password fields in
    // an account creation form are always adjacent.
    render_frame()->GetRenderView()->GetWebView()->AdvanceFocus(false);
  }
  std::unique_ptr<PasswordForm> presaved_form(CreatePasswordFormToPresave());
  if (presaved_form)
    GetPasswordManagerDriver()->PresaveGeneratedPassword(*presaved_form);

  // Call UpdateStateForTextChange after the corresponding PasswordFormManager
  // is notified that the password was generated.
  for (auto& password_element : generation_form_data_->password_elements) {
    // Needed to notify password_autofill_agent that the content of the field
    // has changed. Without this we will overwrite the generated
    // password with an Autofilled password when saving.
    // https://crbug.com/493455
    password_agent_->UpdateStateForTextChange(password_element);
  }
}

std::unique_ptr<PasswordForm>
PasswordGenerationAgent::CreatePasswordFormToPresave() {
  DCHECK(!generation_element_.IsNull());
  // Since the form for presaving should match a form in the browser, create it
  // with the same algorithm (to match html attributes, action, etc.), but
  // change username and password values.
  std::unique_ptr<PasswordForm> password_form;
  if (!generation_element_.Form().IsNull()) {
    password_form =
        password_agent_->GetPasswordFormFromWebForm(generation_element_.Form());
  } else {
    password_form = password_agent_->GetPasswordFormFromUnownedInputElements();
  }
  if (password_form) {
    password_form->type = PasswordForm::TYPE_GENERATED;
    // TODO(kolos): when we are good in username detection, save username
    // as well.
    password_form->username_value = base::string16();
    password_form->password_value = generation_element_.Value().Utf16();
  }

  return password_form;
}

void PasswordGenerationAgent::FoundFormsEligibleForGeneration(
    const std::vector<PasswordFormGenerationData>& forms) {
  generation_enabled_forms_.insert(generation_enabled_forms_.end(),
                                   forms.begin(), forms.end());
  DetermineGenerationElement();
}

void PasswordGenerationAgent::DetermineGenerationElement() {
  if (generation_form_data_) {
    LogMessage(Logger::STRING_GENERATION_RENDERER_FORM_ALREADY_FOUND);
    return;
  }

  // Make sure local heuristics have identified a possible account creation
  // form.
  if (possible_account_creation_forms_.empty()) {
    LogMessage(Logger::STRING_GENERATION_RENDERER_NO_POSSIBLE_CREATION_FORMS);
    return;
  }

  // Note that no messages will be sent if this feature is disabled
  // (e.g. password saving is disabled).
  for (auto& possible_form_data : possible_account_creation_forms_) {
    PasswordForm* possible_password_form = possible_form_data.form.get();
    const PasswordFormGenerationData* generation_data = nullptr;

    std::vector<blink::WebInputElement> password_elements;
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kLocalHeuristicsOnlyForPasswordGeneration)) {
      password_elements = possible_form_data.password_elements;
      VLOG(2) << "Bypassing additional checks.";
    } else if (!ContainsURL(not_blacklisted_password_form_origins_,
                            possible_password_form->origin)) {
      LogMessage(Logger::STRING_GENERATION_RENDERER_NOT_BLACKLISTED);
      continue;
    } else {
      generation_data = FindFormGenerationData(generation_enabled_forms_,
                                               *possible_password_form);
      if (generation_data) {
        password_elements = FindPasswordElementsForGeneration(
            possible_form_data.password_elements, *generation_data);
      } else {
        if (!possible_password_form->new_password_marked_by_site) {
          LogMessage(Logger::STRING_GENERATION_RENDERER_NO_SERVER_SIGNAL);
          continue;
        }

        LogMessage(Logger::STRING_GENERATION_RENDERER_AUTOCOMPLETE_ATTRIBUTE);
        password_generation::LogPasswordGenerationEvent(
            password_generation::AUTOCOMPLETE_ATTRIBUTES_ENABLED_GENERATION);

        password_elements = FindNewPasswordElementsMarkedBySite(
            possible_form_data.password_elements);
      }
    }

    LogMessage(Logger::STRING_GENERATION_RENDERER_ELIGIBLE_FORM_FOUND);
    if (password_elements.empty()) {
      // It might be if JavaScript changes field names.
      LogMessage(Logger::STRING_GENERATION_RENDERER_NO_FIELD_FOUND);
      return;
    }

    generation_form_data_.reset(new AccountCreationFormData(
        possible_form_data.form, std::move(password_elements)));
    generation_element_ = generation_form_data_->password_elements[0];
    if (mark_generation_element_)
      generation_element_.SetAttribute("password_creation_field", "1");
    generation_element_.SetAttribute("aria-autocomplete", "list");
    password_generation::LogPasswordGenerationEvent(
        password_generation::GENERATION_AVAILABLE);
    possible_account_creation_forms_.clear();
    GetPasswordManagerClient()->GenerationAvailableForForm(
        *generation_form_data_->form);
    return;
  }
}

bool PasswordGenerationAgent::SetUpUserTriggeredGeneration() {
  if (last_focused_password_element_.IsNull() || !render_frame())
    return false;

  blink::WebFormElement form = last_focused_password_element_.Form();
  std::unique_ptr<PasswordForm> password_form;
  std::vector<blink::WebFormControlElement> control_elements;
  if (!form.IsNull()) {
    password_form = password_agent_->GetPasswordFormFromWebForm(form);
    control_elements = form_util::ExtractAutofillableElementsInForm(form);
  } else {
    const blink::WebLocalFrame& frame = *render_frame()->GetWebFrame();
    blink::WebDocument doc = frame.GetDocument();
    if (doc.IsNull())
      return false;
    password_form = password_agent_->GetPasswordFormFromUnownedInputElements();
    control_elements =
        form_util::GetUnownedFormFieldElements(doc.All(), nullptr);
  }

  if (!password_form)
    return false;

  generation_element_ = last_focused_password_element_;
  std::vector<blink::WebInputElement> password_elements;
  GetAccountCreationPasswordFields(control_elements, &password_elements);
  password_elements = FindPasswordElementsForGeneration(
      password_elements,
      PasswordFormGenerationData(
          0, /* form_signature */
          CalculateFieldSignatureByNameAndType(
              last_focused_password_element_.NameForAutofill().Utf16(),
              last_focused_password_element_.FormControlTypeForAutofill()
                  .Utf8())));
  generation_form_data_.reset(new AccountCreationFormData(
      make_linked_ptr(password_form.release()), password_elements));
  is_manually_triggered_ = true;
  return true;
}

bool PasswordGenerationAgent::FocusedNodeHasChanged(
    const blink::WebNode& node) {
  if (!generation_element_.IsNull())
    generation_element_.SetShouldRevealPassword(false);

  if (node.IsNull() || !node.IsElementNode())
    return false;

  const blink::WebElement web_element = node.ToConst<blink::WebElement>();
  if (!web_element.GetDocument().GetFrame())
    return false;

  const blink::WebInputElement* element = ToWebInputElement(&web_element);
  if (element && element->IsPasswordFieldForAutofill())
    last_focused_password_element_ = *element;
  if (!element || *element != generation_element_)
    return false;

  if (password_is_generated_) {
    if (generation_element_.Value().IsEmpty()) {
      PasswordNoLongerGenerated();
    } else {
      generation_element_.SetShouldRevealPassword(true);
      ShowEditingPopup();
    }
    return true;
  }

  // Assume that if the password field has less than kMaximumOfferSize
  // characters then the user is not finished typing their password and display
  // the password suggestion.
  if (!element->IsReadOnly() && element->IsEnabled() &&
      element->Value().length() <= kMaximumOfferSize) {
    ShowGenerationPopup();
    return true;
  }

  return false;
}

bool PasswordGenerationAgent::TextDidChangeInTextField(
    const blink::WebInputElement& element) {
  if (element != generation_element_)
    return false;

  if (element.Value().IsEmpty()) {
    if (password_is_generated_) {
      // User generated a password and then deleted it.
      PasswordNoLongerGenerated();
    }

    // Offer generation again.
    ShowGenerationPopup();
  } else if (password_is_generated_) {
    password_edited_ = true;
    // Mirror edits to any confirmation password fields.
    CopyElementValueToOtherInputElements(&element,
        &generation_form_data_->password_elements);
    std::unique_ptr<PasswordForm> presaved_form(CreatePasswordFormToPresave());
    if (presaved_form) {
      GetPasswordManagerDriver()->PresaveGeneratedPassword(*presaved_form);
    }
  } else if (element.Value().length() > kMaximumOfferSize) {
    // User has rejected the feature and has started typing a password.
    HidePopup();
  } else {
    // Password isn't generated and there are fewer than kMaximumOfferSize
    // characters typed, so keep offering the password. Note this function
    // will just keep the previous popup if one is already showing.
    ShowGenerationPopup();
  }

  return true;
}

void PasswordGenerationAgent::ShowGenerationPopup() {
  if (!render_frame() || generation_element_.IsNull())
    return;
  LogMessage(Logger::STRING_GENERATION_RENDERER_SHOW_GENERATION_POPUP);
  GetPasswordManagerClient()->ShowPasswordGenerationPopup(
      render_frame()->GetRenderView()->ElementBoundsInWindow(
          generation_element_),
      generation_element_.MaxLength(),
      generation_element_.NameForAutofill().Utf16(), is_manually_triggered_,
      *generation_form_data_->form);
  generation_popup_shown_ = true;
}

void PasswordGenerationAgent::ShowEditingPopup() {
  if (!render_frame())
    return;
  GetPasswordManagerClient()->ShowPasswordEditingPopup(
      render_frame()->GetRenderView()->ElementBoundsInWindow(
          generation_element_),
      *generation_form_data_->form);
  editing_popup_shown_ = true;
}

void PasswordGenerationAgent::HidePopup() {
  GetPasswordManagerClient()->HidePasswordGenerationPopup();
}

void PasswordGenerationAgent::PasswordNoLongerGenerated() {
  DCHECK(password_is_generated_);
  // Do not treat the password as generated, either here or in the browser.
  password_is_generated_ = false;
  password_edited_ = false;
  generation_element_.SetShouldRevealPassword(false);
  for (blink::WebInputElement& password :
       generation_form_data_->password_elements)
    password.SetAutofilled(false);
  password_generation::LogPasswordGenerationEvent(
      password_generation::PASSWORD_DELETED);
  CopyElementValueToOtherInputElements(
      &generation_element_, &generation_form_data_->password_elements);
  std::unique_ptr<PasswordForm> presaved_form(CreatePasswordFormToPresave());
  if (presaved_form)
    GetPasswordManagerDriver()->PasswordNoLongerGenerated(*presaved_form);
}

void PasswordGenerationAgent::UserTriggeredGeneratePassword() {
  if (SetUpUserTriggeredGeneration())
    ShowGenerationPopup();
}

void PasswordGenerationAgent::UserSelectedManualGenerationOption() {
  if (SetUpUserTriggeredGeneration()) {
    last_focused_password_element_.SetAutofillValue(blink::WebString());
    last_focused_password_element_.SetAutofilled(false);
    ShowGenerationPopup();
  }
}

const mojom::PasswordManagerDriverPtr&
PasswordGenerationAgent::GetPasswordManagerDriver() {
  DCHECK(password_agent_);
  return password_agent_->GetPasswordManagerDriver();
}

const mojom::PasswordManagerClientAssociatedPtr&
PasswordGenerationAgent::GetPasswordManagerClient() {
  if (!password_manager_client_) {
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
        &password_manager_client_);
  }

  return password_manager_client_;
}

void PasswordGenerationAgent::LogMessage(Logger::StringID message_id) {
  if (!password_agent_->logging_state_active())
    return;
  RendererSavePasswordProgressLogger logger(GetPasswordManagerDriver().get());
  logger.LogMessage(message_id);
}

void PasswordGenerationAgent::LogBoolean(Logger::StringID message_id,
                                         bool truth_value) {
  if (!password_agent_->logging_state_active())
    return;
  RendererSavePasswordProgressLogger logger(GetPasswordManagerDriver().get());
  logger.LogBoolean(message_id, truth_value);
}

void PasswordGenerationAgent::LogNumber(Logger::StringID message_id,
                                        int number) {
  if (!password_agent_->logging_state_active())
    return;
  RendererSavePasswordProgressLogger logger(GetPasswordManagerDriver().get());
  logger.LogNumber(message_id, number);
}

}  // namespace autofill
