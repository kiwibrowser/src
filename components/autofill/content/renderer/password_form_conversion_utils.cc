// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/password_form_conversion_utils.h"

#include <stddef.h>

#include <algorithm>
#include <set>
#include <string>

#include "base/containers/flat_set.h"
#include "base/i18n/case_conversion.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/html_based_username_detector.h"
#include "components/autofill/core/common/autofill_regex_constants.h"
#include "components/autofill/core/common/autofill_regexes.h"
#include "components/autofill/core/common/autofill_util.h"
#include "components/autofill/core/common/password_form.h"
#include "components/autofill/core/common/password_form_field_prediction_map.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/base/url_util.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_form_control_element.h"
#include "third_party/blink/public/web/web_input_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

using blink::WebFormControlElement;
using blink::WebFormElement;
using blink::WebInputElement;
using blink::WebLocalFrame;
using blink::WebString;

namespace autofill {

namespace {

constexpr char kAutocompleteUsername[] = "username";
constexpr char kAutocompleteCurrentPassword[] = "current-password";
constexpr char kAutocompleteNewPassword[] = "new-password";
constexpr char kAutocompleteCreditCardPrefix[] = "cc-";

// Parses the string with the value of an autocomplete attribute. If any of the
// tokens "username", "current-password" or "new-password" are present, returns
// an appropriate enum value, picking an arbitrary one if more are applicable.
// Otherwise, it returns CREDIT_CARD if a token with a "cc-" prefix is found.
// Otherwise, returns NONE.
AutocompleteFlag ExtractAutocompleteFlag(const std::string& attribute) {
  std::vector<base::StringPiece> tokens =
      base::SplitStringPiece(attribute, base::kWhitespaceASCII,
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  bool cc_seen = false;
  for (base::StringPiece token : tokens) {
    if (base::LowerCaseEqualsASCII(token, kAutocompleteUsername))
      return AutocompleteFlag::USERNAME;
    if (base::LowerCaseEqualsASCII(token, kAutocompleteCurrentPassword))
      return AutocompleteFlag::CURRENT_PASSWORD;
    if (base::LowerCaseEqualsASCII(token, kAutocompleteNewPassword))
      return AutocompleteFlag::NEW_PASSWORD;

    if (!cc_seen) {
      cc_seen = base::StartsWith(token, kAutocompleteCreditCardPrefix,
                                 base::CompareCase::SENSITIVE);
    }
  }
  return cc_seen ? AutocompleteFlag::CREDIT_CARD : AutocompleteFlag::NONE;
}

// Helper to spare map::find boilerplate when caching element's autocomplete
// attributes.
class AutocompleteCache {
 public:
  AutocompleteCache();

  ~AutocompleteCache();

  // Computes and stores the AutocompleteFlag for |element| based on its
  // autocomplete attribute. Note that this cannot be done on-demand during
  // RetrieveFor, because the cache spares space and look-up time by not storing
  // AutocompleteFlag::NONE values, hence for all elements without an
  // autocomplete attribute, every retrieval would result in a new computation.
  void Store(const WebInputElement& element);

  // Retrieves the value previously stored for |element|.
  AutocompleteFlag RetrieveFor(const WebInputElement& element) const;

 private:
  std::map<WebInputElement, AutocompleteFlag> cache_;

  DISALLOW_COPY_AND_ASSIGN(AutocompleteCache);
};

AutocompleteCache::AutocompleteCache() = default;

AutocompleteCache::~AutocompleteCache() = default;

void AutocompleteCache::Store(const WebInputElement& element) {
  const AutocompleteFlag flag = AutocompleteFlagForElement(element);
  // Only store non-trivial flags. Most of the elements will have the NONE
  // value, so spare storage and lookup time by assuming anything not stored in
  // |cache_| has the NONE flag.
  if (flag != AutocompleteFlag::NONE)
    cache_[element] = flag;
}

AutocompleteFlag AutocompleteCache::RetrieveFor(
    const WebInputElement& element) const {
  auto it = cache_.find(element);
  if (it == cache_.end())
    return AutocompleteFlag::NONE;
  return it->second;
}

// Describes fields filtering criteria. More priority criteria has higher value
// in the enum. The fields with the maximal criteria are considered in a form,
// others are ignored. Criteria for password and username fields are calculated
// separately. For example, if there is a password field with user input, the
// password fields without user input are ignored (independently whether the
// fields are visible or not).
enum class FieldFilteringLevel {
  NO_FILTER = 0,
  VISIBILITY = 1,
  USER_INPUT = 2
};

// PasswordForms can be constructed for both WebFormElements and for collections
// of WebInputElements that are not in a WebFormElement. This intermediate
// aggregating structure is provided so GetPasswordForm() only has one
// view of the underlying data, regardless of its origin.
struct SyntheticForm {
  SyntheticForm();
  SyntheticForm(SyntheticForm&& other);
  ~SyntheticForm();

  // Contains control elements of the represented form, including not fillable
  // ones.
  std::vector<blink::WebFormControlElement> control_elements;
  // The origin of the containing document.
  GURL origin;

 private:
  DISALLOW_COPY_AND_ASSIGN(SyntheticForm);
};

SyntheticForm::SyntheticForm() = default;
SyntheticForm::SyntheticForm(SyntheticForm&& other) = default;
SyntheticForm::~SyntheticForm() = default;

// Layout classification of password forms
// A layout sequence of a form is the sequence of it's non-password and password
// input fields, represented by "N" and "P", respectively. A form like this
// <form>
//   <input type='text' ...>
//   <input type='hidden' ...>
//   <input type='password' ...>
//   <input type='submit' ...>
// </form>
// has the layout sequence "NP" -- "N" for the first field, and "P" for the
// third. The second and fourth fields are ignored, because they are not text
// fields.
//
// The code below classifies the layout (see PasswordForm::Layout) of a form
// based on its layout sequence. This is done by assigning layouts regular
// expressions over the alphabet {N, P}. LAYOUT_OTHER is implicitly the type
// corresponding to all layout sequences not matching any other layout.
//
// LAYOUT_LOGIN_AND_SIGNUP is classified by NPN+P.*. This corresponds to a form
// which starts with a login section (NP) and continues with a sign-up section
// (N+P.*). The aim is to distinguish such forms from change password-forms
// (N*PPP?.*) and forms which use password fields to store private but
// non-password data (could look like, e.g., PN+P.*).
const char kLoginAndSignupRegex[] =
    "NP"   // Login section.
    "N+P"  // Sign-up section.
    ".*";  // Anything beyond that.

struct LoginAndSignupLazyInstanceTraits
    : public base::internal::DestructorAtExitLazyInstanceTraits<re2::RE2> {
  static re2::RE2* New(void* instance) {
    return CreateMatcher(instance, kLoginAndSignupRegex);
  }
};

base::LazyInstance<re2::RE2, LoginAndSignupLazyInstanceTraits>
    g_login_and_signup_matcher = LAZY_INSTANCE_INITIALIZER;

// Return a pointer to WebInputElement iff |control_element| is an enabled text
// input element. Otherwise, returns nullptr.
const WebInputElement* GetEnabledTextInputFieldOrNull(
    const WebFormControlElement& control_element) {
  const WebInputElement* input_element = ToWebInputElement(&control_element);
  if (input_element && input_element->IsEnabled() &&
      input_element->IsTextField()) {
    return input_element;
  }
  return nullptr;
}

// Given the sequence of non-password and password text input fields of a form,
// represented as a string of Ns (non-password) and Ps (password), computes the
// layout type of that form.
PasswordForm::Layout SequenceToLayout(base::StringPiece layout_sequence) {
  if (re2::RE2::FullMatch(
          re2::StringPiece(layout_sequence.data(),
                           base::checked_cast<int>(layout_sequence.size())),
          g_login_and_signup_matcher.Get())) {
    return PasswordForm::Layout::LAYOUT_LOGIN_AND_SIGNUP;
  }
  return PasswordForm::Layout::LAYOUT_OTHER;
}

void PopulateSyntheticFormFromWebForm(const WebFormElement& web_form,
                                      SyntheticForm* synthetic_form) {
  // TODO(vabr): The fact that we are actually passing all form fields, not just
  // autofillable ones (cause of http://crbug.com/537396, see also
  // http://crbug.com/543006) is not tested yet, due to difficulties to fake
  // test frame origin to match GAIA login page. Once this code gets moved to
  // browser, we need to add tests for this as well.
  blink::WebVector<blink::WebFormControlElement> web_control_elements;
  web_form.GetFormControlElements(web_control_elements);
  synthetic_form->control_elements.assign(web_control_elements.begin(),
                                          web_control_elements.end());
  synthetic_form->origin =
      form_util::GetCanonicalOriginForDocument(web_form.GetDocument());
}

// Helper to determine which password is the main (current) one, and which is
// the new password (e.g., on a sign-up or change password form), if any. If the
// new password is found and there is another password field with the same user
// input, the function also sets |confirmation_password| to this field.
void LocateSpecificPasswords(std::vector<WebInputElement> passwords,
                             WebInputElement* current_password,
                             WebInputElement* new_password,
                             WebInputElement* confirmation_password,
                             const AutocompleteCache& autocomplete_cache) {
  DCHECK(!passwords.empty());
  DCHECK(current_password && current_password->IsNull());
  DCHECK(new_password && new_password->IsNull());
  DCHECK(confirmation_password && confirmation_password->IsNull());

  // First, look for elements marked with either autocomplete='current-password'
  // or 'new-password' -- if we find any, take the hint, and treat the first of
  // each kind as the element we are looking for.
  for (const WebInputElement& password : passwords) {
    const AutocompleteFlag flag = autocomplete_cache.RetrieveFor(password);
    if (flag == AutocompleteFlag::CURRENT_PASSWORD &&
        current_password->IsNull()) {
      *current_password = password;
    } else if (flag == AutocompleteFlag::NEW_PASSWORD &&
               new_password->IsNull()) {
      *new_password = password;
    } else if (!new_password->IsNull() &&
               (new_password->Value() == password.Value())) {
      *confirmation_password = password;
    }
  }

  // If we have seen an element with either of autocomplete attributes above,
  // take that as a signal that the page author must have intentionally left the
  // rest of the password fields unmarked. Perhaps they are used for other
  // purposes, e.g., PINs, OTPs, and the like. So we skip all the heuristics we
  // normally do, and ignore the rest of the password fields.
  if (!current_password->IsNull() || !new_password->IsNull())
    return;

  switch (passwords.size()) {
    case 1:
      // Single password, easy.
      *current_password = passwords[0];
      break;
    case 2:
      if (!passwords[0].Value().IsEmpty() &&
          passwords[0].Value() == passwords[1].Value()) {
        // Two identical non-empty passwords: assume we are seeing a new
        // password with a confirmation. This can be either a sign-up form or a
        // password change form that does not ask for the old password.
        *new_password = passwords[0];
        *confirmation_password = passwords[1];
      } else {
        // Assume first is old password, second is new (no choice but to guess).
        // This case also includes empty passwords in order to allow filling of
        // password change forms (that also could autofill for sign up form, but
        // we can't do anything with this using only client side information).
        *current_password = passwords[0];
        *new_password = passwords[1];
      }
      break;
    default:
      if (!passwords[0].Value().IsEmpty() &&
          passwords[0].Value() == passwords[1].Value() &&
          passwords[0].Value() == passwords[2].Value()) {
        // All three passwords are the same and non-empty? It may be a change
        // password form where old and new passwords are the same. It doesn't
        // matter what field is correct, let's save the value.
        *current_password = passwords[0];
      } else if (passwords[1].Value() == passwords[2].Value()) {
        // New password is the duplicated one, and comes second; or empty form
        // with 3 password fields, in which case we will assume this layout.
        *current_password = passwords[0];
        *new_password = passwords[1];
        *confirmation_password = passwords[2];
      } else if (passwords[0].Value() == passwords[1].Value()) {
        // It is strange that the new password comes first, but trust more which
        // fields are duplicated than the ordering of fields. Assume that
        // any password fields after the new password contain sensitive
        // information that isn't actually a password (security hint, SSN, etc.)
        *new_password = passwords[0];
        *confirmation_password = passwords[1];
      } else {
        // Three different passwords, or first and last match with middle
        // different. No idea which is which. Let's save the first password.
        // Password selection in a prompt will allow to correct the choice.
        *current_password = passwords[0];
      }
  }
}

void FindPredictedElements(
    const std::vector<blink::WebFormControlElement>& control_elements,
    const FormData& form_data,
    const FormsPredictionsMap& form_predictions,
    std::map<WebInputElement, PasswordFormFieldPredictionType>*
        predicted_elements) {
  // Matching only requires that action and name of the form match to allow
  // the username to be updated even if the form is changed after page load.
  // See https://crbug.com/476092 for more details.
  const PasswordFormFieldPredictionMap* field_predictions = nullptr;
  for (const auto& form_predictions_pair : form_predictions) {
    if (form_predictions_pair.first.action == form_data.action &&
        form_predictions_pair.first.name == form_data.name) {
      field_predictions = &form_predictions_pair.second;
      break;
    }
  }

  if (!field_predictions)
    return;

  std::vector<blink::WebFormControlElement> autofillable_elements =
      form_util::ExtractAutofillableElementsFromSet(control_elements);
  for (const auto& prediction : *field_predictions) {
    const FormFieldData& target_field = prediction.first;
    const PasswordFormFieldPredictionType& type = prediction.second;
    for (const auto& control_element : autofillable_elements)  {
      if (control_element.NameForAutofill().Utf16() == target_field.name) {
        const WebInputElement* input_element =
            ToWebInputElement(&control_element);

        // TODO(sebsg): Investigate why this guard is necessary, see
        // https://crbug.com/517490 for more details.
        if (input_element) {
          (*predicted_elements)[*input_element] = type;
        }
        break;
      }
    }
  }
}

const char kPasswordSiteUrlRegex[] =
    "passwords(?:-[a-z-]+\\.corp)?\\.google\\.com";

struct PasswordSiteUrlLazyInstanceTraits
    : public base::internal::DestructorAtExitLazyInstanceTraits<re2::RE2> {
  static re2::RE2* New(void* instance) {
    return CreateMatcher(instance, kPasswordSiteUrlRegex);
  }
};

base::LazyInstance<re2::RE2, PasswordSiteUrlLazyInstanceTraits>
    g_password_site_matcher = LAZY_INSTANCE_INITIALIZER;

// Returns the |input_field| name if its non-empty; otherwise a |dummy_name|.
base::string16 FieldName(const WebInputElement& input_field,
                         const char dummy_name[]) {
  base::string16 field_name = input_field.NameForAutofill().Utf16();
  return field_name.empty() ? base::ASCIIToUTF16(dummy_name) : field_name;
}

// Returns true iff the properties mask of |element| intersects with |mask|.
bool FieldHasPropertiesMask(const FieldValueAndPropertiesMaskMap* field_map,
                            const blink::WebFormControlElement& element,
                            FieldPropertiesMask mask) {
  if (!field_map)
    return false;
  FieldValueAndPropertiesMaskMap::const_iterator it = field_map->find(element);
  return it != field_map->end() && (it->second.second & mask);
}

// Return the maximal filtering criteria that |element| passes.
// If |ignore_autofilled_values|, autofilled value isn't considered user input.
FieldFilteringLevel GetFiltertingLevelForField(
    const blink::WebFormControlElement& element,
    const FieldValueAndPropertiesMaskMap* field_value_and_properties_map,
    bool ignore_autofilled_values) {
  FieldPropertiesMask user_input_mask =
      ignore_autofilled_values
          ? FieldPropertiesFlags::USER_TYPED
          : FieldPropertiesFlags::USER_TYPED | FieldPropertiesFlags::AUTOFILLED;
  if (FieldHasPropertiesMask(field_value_and_properties_map, element,
                             user_input_mask)) {
    return FieldFilteringLevel::USER_INPUT;
  }
  return form_util::IsWebElementVisible(element)
             ? FieldFilteringLevel::VISIBILITY
             : FieldFilteringLevel::NO_FILTER;
}

// Calculates the maximal filtering levels for password and username fields and
// saves them to |username_fields_level| and |password_fields_level|. The
// criteria for username fields considers only the fields before the first
// password field that has the maximal filtering level.
void GetFieldFilteringLevels(
    const std::vector<blink::WebFormControlElement>& control_elements,
    const FieldValueAndPropertiesMaskMap* field_value_and_properties_map,
    FieldFilteringLevel* username_fields_level,
    FieldFilteringLevel* password_fields_level) {
  DCHECK(password_fields_level);
  DCHECK(username_fields_level);
  *username_fields_level = FieldFilteringLevel::NO_FILTER;
  *password_fields_level = FieldFilteringLevel::NO_FILTER;

  FieldFilteringLevel max_level_found_for_username_fields =
      FieldFilteringLevel::NO_FILTER;
  for (auto& control_element : control_elements) {
    const WebInputElement* input_element = ToWebInputElement(&control_element);
    if (!input_element || !input_element->IsEnabled() ||
        !input_element->IsTextField()) {
      continue;
    }

    // TODO(crbug.com/789917): Ignore autofilled values here because if there
    // are only autofilled values then a form may not be filled completely (i.e.
    // some user input is still expected). So, user input shouldn't be used for
    // fields filtering. Once the bug is resolved, autofilled values will not be
    // ignored.
    FieldFilteringLevel current_field_filtering_level =
        GetFiltertingLevelForField(control_element,
                                   field_value_and_properties_map,
                                   true /* ignore_autofilled_values */);

    if (input_element->IsPasswordFieldForAutofill()) {
      if (*password_fields_level < current_field_filtering_level) {
        *password_fields_level = current_field_filtering_level;
        *username_fields_level = max_level_found_for_username_fields;
      }
    } else {
      max_level_found_for_username_fields = std::max(
          max_level_found_for_username_fields, current_field_filtering_level);
    }
  }
}

ValueElementPair MakePossibleUsernamePair(const blink::WebInputElement& input) {
  base::string16 trimmed_input_value, trimmed_input_autofill;
  base::TrimString(input.Value().Utf16(), base::ASCIIToUTF16(" "),
                   &trimmed_input_value);
  return {trimmed_input_value, input.NameForAutofill().Utf16()};
}

// Check if a script modified username is suitable for Password Manager to
// remember.
bool ScriptModifiedUsernameAcceptable(
    const base::string16& username_value,
    const base::string16& typed_username_value,
    const PasswordForm* password_form,
    const FieldValueAndPropertiesMaskMap* field_value_and_properties_map) {
  DCHECK(password_form);
  DCHECK(field_value_and_properties_map);
  // The minimal size of a field value that will be matched.
  const size_t kMinMatchSize = 3u;
  const auto username = base::i18n::ToLower(username_value);
  const auto typed_username = base::i18n::ToLower(typed_username_value);
  if (base::StartsWith(username, typed_username, base::CompareCase::SENSITIVE))
    return true;

  // Check if the username was generated by javascript based on user typed name.
  if (typed_username.size() >= kMinMatchSize &&
      username_value.find(typed_username) != base::string16::npos)
    return true;

  // Check if the username was generated by javascript based on user typed or
  // autofilled field values.
  for (const auto& field_prop : *field_value_and_properties_map) {
    if (!field_prop.second.first)
      continue;
    const auto& field_value = *field_prop.second.first;
    const WebInputElement* input_element = ToWebInputElement(&field_prop.first);
    if (input_element && input_element->IsTextField() &&
        !input_element->IsPasswordFieldForAutofill() &&
        field_value.size() >= kMinMatchSize &&
        username_value.find(base::i18n::ToLower(field_value)) !=
            base::string16::npos) {
      return true;
    }
  }

  return false;
}

bool StringMatchesCVC(const base::string16& str) {
  static const base::NoDestructor<base::string16> kCardCvcReCached(
      base::UTF8ToUTF16(kCardCvcRe));

  return MatchesPattern(str, *kCardCvcReCached);
}

bool IsEnabledPasswordFieldPresent(const std::vector<FormFieldData>& fields) {
  return std::find_if(
             fields.begin(), fields.end(), [](const FormFieldData& field) {
               return field.is_enabled && field.form_control_type == "password";
             }) != fields.end();
}

// Find the first element in |username_predictions| (i.e. the most reliable
// prediction) that occurs in |possible_usernames|. If the element is found, the
// method saves it to |username_element| and returns true.
bool FindUsernameInPredictions(
    const std::vector<blink::WebInputElement>& username_predictions,
    const std::vector<blink::WebInputElement>& possible_usernames,
    WebInputElement* username_element) {
  // To speed-up the matching for-loop below, convert |possible_usernames| to a
  // set. Creating is O(N log N) for N=possible_usernames.size(). Retrieval is
  // O(log N), so the whole for-loop is O(M log N) for
  // M=username_predictions.size(). Use flat_set, because of cache locality (the
  // M and N are likely small, so this can make a difference) and less heap
  // allocations.
  const base::flat_set<blink::WebInputElement> usernames(
      possible_usernames.begin(), possible_usernames.end());

  for (const blink::WebInputElement& prediction : username_predictions) {
    auto iter = usernames.find(prediction);
    if (iter != usernames.end()) {
      *username_element = *iter;
      return true;
    }
  }
  return false;
}

// Get information about a login form encapsulated in a PasswordForm struct.
// If an element of |form| has an entry in |nonscript_modified_values|, the
// associated string is used instead of the element's value to create
// the PasswordForm.
bool GetPasswordForm(
    SyntheticForm form,
    PasswordForm* password_form,
    const FieldValueAndPropertiesMaskMap* field_value_and_properties_map,
    const FormsPredictionsMap* form_predictions,
    UsernameDetectorCache* username_detector_cache) {
  DCHECK(!form.control_elements.empty());

  // Early exit if no passwords to be typed into.
  if (!IsEnabledPasswordFieldPresent(password_form->form_data.fields))
    return false;

  // Narrow the scope to enabled inputs.
  std::vector<WebInputElement> enabled_inputs;
  enabled_inputs.reserve(form.control_elements.size());
  for (const WebFormControlElement& control_element : form.control_elements) {
    const WebInputElement* input_element =
        GetEnabledTextInputFieldOrNull(control_element);
    if (input_element)
      enabled_inputs.push_back(*input_element);
  }

  // Remember the list of password fields without any heuristics applied in case
  // the heuristics fail and a fall-back is needed:
  // All password fields.
  std::vector<WebInputElement> passwords_without_heuristics;
  // Map from all password fields to the most recent non-password text input.
  std::map<WebInputElement, WebInputElement>
      preceding_text_input_for_password_without_heuristics;
  WebInputElement most_recent_text_input;  // Just a temporary.
  for (const WebInputElement& input : enabled_inputs) {
    if (input.IsPasswordFieldForAutofill()) {
      passwords_without_heuristics.push_back(input);
      preceding_text_input_for_password_without_heuristics[input] =
          most_recent_text_input;
    } else {
      most_recent_text_input = input;
    }
  }

  // Fill the cache with autocomplete flags.
  AutocompleteCache autocomplete_cache;
  for (const WebInputElement& input : enabled_inputs) {
    autocomplete_cache.Store(input);
  }

  // Narrow the scope further: drop credit-card fields.
  std::vector<WebInputElement> plausible_inputs;
  plausible_inputs.reserve(enabled_inputs.size());
  for (const WebInputElement& input : enabled_inputs) {
    const AutocompleteFlag flag = autocomplete_cache.RetrieveFor(input);
    if (flag == AutocompleteFlag::CURRENT_PASSWORD ||
        flag == AutocompleteFlag::NEW_PASSWORD) {
      // A field marked as a password is considered not a credit-card field, no
      // matter what.
      plausible_inputs.push_back(input);
    } else if (flag != AutocompleteFlag::CREDIT_CARD &&
               !IsCreditCardVerificationPasswordField(input)) {
      // Otherwise ensure that nothing hints that |input| is a credit-card
      // field.
      plausible_inputs.push_back(input);
    }
  }

  // Further narrow to interesting fields (e.g., with user input, visible), if
  // present.
  // Compute the best filtering levels for usernames and for passwords.
  FieldFilteringLevel username_fields_level = FieldFilteringLevel::NO_FILTER;
  FieldFilteringLevel password_fields_level = FieldFilteringLevel::NO_FILTER;
  GetFieldFilteringLevels(form.control_elements, field_value_and_properties_map,
                          &username_fields_level, &password_fields_level);
  // Remove all fields with filtering level below the best.
  base::EraseIf(
      plausible_inputs, [&field_value_and_properties_map, password_fields_level,
                         username_fields_level](const WebInputElement& input) {
        FieldFilteringLevel current_field_level =
            GetFiltertingLevelForField(input, field_value_and_properties_map,
                                       false /* ignore_autofilled_values */);
        if (input.IsPasswordFieldForAutofill())
          return (current_field_level < password_fields_level);
        else
          return (current_field_level < username_fields_level);
      });

  // Further, remove all readonly passwords. If the password field is readonly,
  // the page is likely using a virtual keyboard and bypassing the password
  // field value (see http://crbug.com/475488). There is nothing Chrome can do
  // to fill passwords for now. Notable exceptions: if the password field was
  // made readonly by JavaScript before submission, it remains interesting. If
  // the password was marked via the autocomplete attribute, it also remains
  // interesting.
  base::EraseIf(plausible_inputs, [&field_value_and_properties_map,
                                   &autocomplete_cache](
                                      const WebInputElement& input) {
    if (!input.IsPasswordFieldForAutofill())
      return false;
    if (!input.IsReadOnly())
      return false;
    // Check if the field was only made readonly before submission.
    if (FieldHasPropertiesMask(field_value_and_properties_map, input,
                               FieldPropertiesFlags::USER_TYPED |
                                   FieldPropertiesFlags::AUTOFILLED)) {
      return false;
    }
    // Check whether the field was explicitly marked as password.
    const AutocompleteFlag flag = autocomplete_cache.RetrieveFor(input);
    if (flag == AutocompleteFlag::CURRENT_PASSWORD ||
        flag == AutocompleteFlag::NEW_PASSWORD) {
      return false;
    }
    return true;
  });

  // Evaluate available server-side predictions.
  std::map<WebInputElement, PasswordFormFieldPredictionType> predicted_elements;
  WebInputElement predicted_username_element;
  if (form_predictions) {
    FindPredictedElements(form.control_elements, password_form->form_data,
                          *form_predictions, &predicted_elements);

    for (const auto& predicted_pair : predicted_elements) {
      if (predicted_pair.second == PREDICTION_USERNAME) {
        predicted_username_element = predicted_pair.first;
        break;
      }
    }
  }

  // Finally, remove all password fields for which we have a negative
  // prediction, unless they are explicitly marked by the autocomplete attribute
  // as a password.
  base::EraseIf(plausible_inputs, [&predicted_elements, &autocomplete_cache](
                                      const WebInputElement& input) {
    if (!input.IsPasswordFieldForAutofill())
      return false;
    const AutocompleteFlag flag = autocomplete_cache.RetrieveFor(input);
    if (flag == AutocompleteFlag::CURRENT_PASSWORD ||
        flag == AutocompleteFlag::NEW_PASSWORD) {
      return false;
    }
    auto possible_password_element_iterator = predicted_elements.find(input);
    return possible_password_element_iterator != predicted_elements.end() &&
           possible_password_element_iterator->second ==
               PREDICTION_NOT_PASSWORD;
  });

  // Derive the list of all plausible passwords, usernames and the non-password
  // inputs preceding the plausible passwords.
  std::vector<WebInputElement> plausible_passwords;
  std::vector<WebInputElement> plausible_usernames;
  std::map<WebInputElement, WebInputElement>
      preceding_text_input_for_plausible_password;
  most_recent_text_input.Reset();
  plausible_usernames.reserve(plausible_inputs.size());
  for (const WebInputElement& input : plausible_inputs) {
    if (input.IsPasswordFieldForAutofill()) {
      plausible_passwords.push_back(input);
      preceding_text_input_for_plausible_password[input] =
          most_recent_text_input;
    } else {
      plausible_usernames.push_back(input);
      most_recent_text_input = input;
    }
  }

  // Evaluate autocomplete attributes for username.
  WebInputElement username_by_attribute;
  for (const WebInputElement& input : plausible_inputs) {
    if (!input.IsPasswordFieldForAutofill()) {
      if (autocomplete_cache.RetrieveFor(input) == AutocompleteFlag::USERNAME) {
        // Only consider the first occurrence of autocomplete='username'.
        // Multiple occurences hint at the attribute being used incorrectly, in
        // which case sticking to the first one is just a bet.
        if (username_by_attribute.IsNull()) {
          username_by_attribute = input;
        }
      }
    }
  }

  // Evaluate the context of the fields.
  WebInputElement username_element_by_context;
  if (base::FeatureList::IsEnabled(
          password_manager::features::kHtmlBasedUsernameDetector)) {
    // Call HTML based username detector only if neither server predictions nor
    // autocomplete attributes were useful to detect the username.
    if (predicted_username_element.IsNull() && username_by_attribute.IsNull()) {
      // Dummy cache stores the predictions in case no real cache was passed to
      // here.
      UsernameDetectorCache dummy_cache;
      if (!username_detector_cache)
        username_detector_cache = &dummy_cache;

      const std::vector<blink::WebInputElement>& username_predictions =
          GetPredictionsFieldBasedOnHtmlAttributes(form.control_elements,
                                                   password_form->form_data,
                                                   username_detector_cache);

      if (!FindUsernameInPredictions(username_predictions, plausible_usernames,
                                     &username_element_by_context)) {
        username_element_by_context.Reset();
      }
    }
  }

  // Evaluate the structure of the form for determining the form type (e.g.,
  // sign-up, sign-in, etc.).
  std::string layout_sequence;
  layout_sequence.reserve(plausible_inputs.size());
  for (const WebInputElement& input : plausible_inputs) {
    layout_sequence.push_back((input.IsPasswordFieldForAutofill()) ? 'P' : 'N');
  }

  // Populate all_possible_passwords and form_has_autofilled_value in
  // |password_form|.
  // Contains the first password element for each non-empty password value.
  std::vector<ValueElementPair> all_possible_passwords;
  // Reserve enough space to prevent re-allocation. A re-allocation would
  // invalidate the contents of |seen_values|.
  all_possible_passwords.reserve(passwords_without_heuristics.size());
  std::set<base::StringPiece16> seen_values;
  // Pretend that an empty value has been already seen, so that empty-valued
  // password elements won't get added to |all_possible_passwords|.
  seen_values.insert(base::StringPiece16());
  for (const WebInputElement& password_element : passwords_without_heuristics) {
    const base::string16 value = password_element.Value().Utf16();
    if (seen_values.count(value) > 0)
      continue;
    all_possible_passwords.push_back(
        {std::move(value), password_element.NameForAutofill().Utf16()});
    seen_values.insert(
        base::StringPiece16(all_possible_passwords.back().first));
  }

  bool form_has_autofilled_value = false;
  for (const WebInputElement& password_element : passwords_without_heuristics) {
    bool element_has_autofilled_value =
        FieldHasPropertiesMask(field_value_and_properties_map, password_element,
                               FieldPropertiesFlags::AUTOFILLED);
    form_has_autofilled_value |= element_has_autofilled_value;
  }

  if (!all_possible_passwords.empty()) {
    password_form->all_possible_passwords = std::move(all_possible_passwords);
    password_form->form_has_autofilled_value = form_has_autofilled_value;
  }

  // If for some reason (e.g. only credit card fields, confusing autocomplete
  // attributes) the passwords list is empty, build list based on user input (if
  // there is any non-empty password field) and the type of a field. Also mark
  // that the form should be available only for fallback saving (automatic
  // bubble will not pop up).
  password_form->only_for_fallback_saving = plausible_passwords.empty();
  if (plausible_passwords.empty()) {
    plausible_passwords = std::move(passwords_without_heuristics);
    preceding_text_input_for_plausible_password =
        std::move(preceding_text_input_for_password_without_heuristics);
  }

  // Find the password fields.
  WebInputElement password;
  WebInputElement new_password;
  WebInputElement confirmation_password;
  LocateSpecificPasswords(std::move(plausible_passwords), &password,
                          &new_password, &confirmation_password,
                          autocomplete_cache);

  // Choose the username element.
  WebInputElement username_element;
  UsernameDetectionMethod username_detection_method =
      UsernameDetectionMethod::NO_USERNAME_DETECTED;
  password_form->username_marked_by_site = false;

  if (!predicted_username_element.IsNull()) {
    // Server predictions are most trusted, so try them first. Only if the form
    // already has user input and the predicted username field has an empty
    // value, then don't trust the prediction (can be caused by, e.g., a <form>
    // actually contains several forms).
    if ((password_fields_level < FieldFilteringLevel::USER_INPUT ||
         !predicted_username_element.Value().IsEmpty())) {
      username_element = predicted_username_element;
      password_form->was_parsed_using_autofill_predictions = true;
      username_detection_method =
          UsernameDetectionMethod::SERVER_SIDE_PREDICTION;
    }
  }

  if (username_element.IsNull() && !username_by_attribute.IsNull()) {
    // Next in the trusted queue: autocomplete attributes.
    username_element = username_by_attribute;
    username_detection_method = UsernameDetectionMethod::AUTOCOMPLETE_ATTRIBUTE;
  }

  if (username_element.IsNull() && !username_element_by_context.IsNull()) {
    // Last step before base heuristics: HTML-based classifier.
    username_element = username_element_by_context;
    username_detection_method = UsernameDetectionMethod::HTML_BASED_CLASSIFIER;
  }

  // Compute base heuristic for username detection.
  WebInputElement base_heuristic_username;
  if (!password.IsNull()) {
    base_heuristic_username =
        preceding_text_input_for_plausible_password[password];
  }
  if (base_heuristic_username.IsNull() && !new_password.IsNull()) {
    base_heuristic_username =
        preceding_text_input_for_plausible_password[new_password];
  }

  // Apply base heuristic for username detection.
  if (username_element.IsNull()) {
    username_element = base_heuristic_username;
    if (!username_element.IsNull())
      username_detection_method = UsernameDetectionMethod::BASE_HEURISTIC;
  } else if (base_heuristic_username == username_element &&
             username_detection_method !=
                 UsernameDetectionMethod::AUTOCOMPLETE_ATTRIBUTE) {
    // TODO(crbug.com/786404): when the bug is fixed, remove this block and
    // calculate |base_heuristic_username| only if |username_element.IsNull()|
    // This block was added to measure the impact of server-side predictions and
    // HTML based classifier compared to "old classifiers" (the based heuristic
    // and 'autocomplete' attribute).
    username_detection_method = UsernameDetectionMethod::BASE_HEURISTIC;
  }
  UMA_HISTOGRAM_ENUMERATION(
      "PasswordManager.UsernameDetectionMethod", username_detection_method,
      UsernameDetectionMethod::USERNAME_DETECTION_METHOD_COUNT);

  // Populate the username fields in |password_form|.
  if (!username_element.IsNull()) {
    password_form->username_element =
        FieldName(username_element, "anonymous_username");
    base::string16 username_value = username_element.Value().Utf16();
    if (FieldHasPropertiesMask(field_value_and_properties_map, username_element,
                               FieldPropertiesFlags::USER_TYPED |
                                   FieldPropertiesFlags::AUTOFILLED)) {
      base::string16 typed_username_value =
          *field_value_and_properties_map->at(username_element).first;

      if (!ScriptModifiedUsernameAcceptable(username_value,
                                            typed_username_value, password_form,
                                            field_value_and_properties_map)) {
        // If |username_value| was obtained by autofilling
        // |typed_username_value|, |typed_username_value| might be incomplete,
        // so we should leave autofilled value.
        username_value = typed_username_value;
      }
    }
    password_form->username_value = username_value;
  }

  // Populate the password fields in |password_form|.
  if (!password.IsNull()) {
    password_form->password_element = FieldName(password, "anonymous_password");
    blink::WebString password_value = password.Value();
    if (FieldHasPropertiesMask(field_value_and_properties_map, password,
                               FieldPropertiesFlags::USER_TYPED |
                                   FieldPropertiesFlags::AUTOFILLED)) {
      password_value = blink::WebString::FromUTF16(
          *field_value_and_properties_map->at(password).first);
    }
    password_form->password_value = password_value.Utf16();
  }
  if (!new_password.IsNull()) {
    password_form->new_password_element =
        FieldName(new_password, "anonymous_new_password");
    password_form->new_password_value = new_password.Value().Utf16();
    password_form->new_password_value_is_default =
        new_password.GetAttribute("value") == new_password.Value();
    if (autocomplete_cache.RetrieveFor(new_password) ==
        AutocompleteFlag::NEW_PASSWORD) {
      password_form->new_password_marked_by_site = true;
    }
    if (!confirmation_password.IsNull()) {
      password_form->confirmation_password_element =
          FieldName(confirmation_password, "anonymous_confirmation_password");
    }
  }

  // Populate |other_possible_usernames| in |password_form|.
  ValueElementVector other_possible_usernames;
  for (const WebInputElement& plausible_username : plausible_usernames) {
    if (plausible_username == username_element)
      continue;
    auto pair = MakePossibleUsernamePair(plausible_username);
    if (!pair.first.empty())
      other_possible_usernames.push_back(std::move(pair));
  }
  password_form->other_possible_usernames = std::move(other_possible_usernames);

  // Report metrics.
  if (username_element.IsNull()) {
    // To get a better idea on how password forms without a username field
    // look like, report the total number of text and password fields.
    UMA_HISTOGRAM_COUNTS_100(
        "PasswordManager.EmptyUsernames.TextAndPasswordFieldCount",
        layout_sequence.size());
    // For comparison, also report the number of password fields.
    UMA_HISTOGRAM_COUNTS_100(
        "PasswordManager.EmptyUsernames.PasswordFieldCount",
        std::count(layout_sequence.begin(), layout_sequence.end(), 'P'));
  }

  password_form->origin = std::move(form.origin);
  password_form->signon_realm = GetSignOnRealm(password_form->origin);
  password_form->scheme = PasswordForm::SCHEME_HTML;
  password_form->preferred = false;
  password_form->blacklisted_by_user = false;
  password_form->type = PasswordForm::TYPE_MANUAL;
  password_form->layout = SequenceToLayout(layout_sequence);

  return true;
}

}  // namespace

AutocompleteFlag AutocompleteFlagForElement(const WebInputElement& element) {
  static const base::NoDestructor<WebString> kAutocomplete(("autocomplete"));
  return ExtractAutocompleteFlag(
      element.GetAttribute(*kAutocomplete)
          .Utf8(WebString::UTF8ConversionMode::kStrictReplacingErrorsWithFFFD));
}

re2::RE2* CreateMatcher(void* instance, const char* pattern) {
  re2::RE2::Options options;
  options.set_case_sensitive(false);
  // Use placement new to initialize the instance in the preallocated space.
  // The "(instance)" is very important to force POD type initialization.
  re2::RE2* matcher = new (instance) re2::RE2(pattern, options);
  DCHECK(matcher->ok());
  return matcher;
}

bool IsGaiaReauthenticationForm(const blink::WebFormElement& form) {
  if (GURL(form.GetDocument().Url()).GetOrigin() !=
      GaiaUrls::GetInstance()->gaia_url().GetOrigin()) {
    return false;
  }

  bool has_rart_field = false;
  bool has_continue_field = false;

  blink::WebVector<blink::WebFormControlElement> web_control_elements;
  form.GetFormControlElements(web_control_elements);

  for (const blink::WebFormControlElement& element : web_control_elements) {
    // We're only interested in the presence
    // of <input type="hidden" /> elements.
    CR_DEFINE_STATIC_LOCAL(WebString, kHidden, ("hidden"));
    const blink::WebInputElement* input = blink::ToWebInputElement(&element);
    if (!input || input->FormControlTypeForAutofill() != kHidden)
      continue;

    // There must be a hidden input named "rart".
    if (input->FormControlName() == "rart")
      has_rart_field = true;

    // There must be a hidden input named "continue", whose value points
    // to a password (or password testing) site.
    if (input->FormControlName() == "continue" &&
        re2::RE2::PartialMatch(input->Value().Utf8(),
                               g_password_site_matcher.Get())) {
      has_continue_field = true;
    }
  }

  return has_rart_field && has_continue_field;
}

bool IsGaiaWithSkipSavePasswordForm(const blink::WebFormElement& form) {
  GURL url(form.GetDocument().Url());
  if (url.GetOrigin() != GaiaUrls::GetInstance()->gaia_url().GetOrigin()) {
    return false;
  }

  std::string should_skip_password;
  if (!net::GetValueForKeyInQuery(url, "ssp", &should_skip_password))
    return false;
  return should_skip_password == "1";
}

std::unique_ptr<PasswordForm> CreatePasswordFormFromWebForm(
    const WebFormElement& web_form,
    const FieldValueAndPropertiesMaskMap* field_value_and_properties_map,
    const FormsPredictionsMap* form_predictions,
    UsernameDetectorCache* username_detector_cache) {
  if (web_form.IsNull())
    return nullptr;

  auto password_form = std::make_unique<PasswordForm>();
  password_form->action = form_util::GetCanonicalActionForForm(web_form);
  if (!password_form->action.is_valid())
    return nullptr;

  SyntheticForm synthetic_form;
  PopulateSyntheticFormFromWebForm(web_form, &synthetic_form);

  if (!WebFormElementToFormData(
          web_form, blink::WebFormControlElement(),
          field_value_and_properties_map, form_util::EXTRACT_VALUE,
          &password_form->form_data, nullptr /* FormFieldData */)) {
    return nullptr;
  }

  if (!GetPasswordForm(std::move(synthetic_form), password_form.get(),
                       field_value_and_properties_map, form_predictions,
                       username_detector_cache)) {
    return nullptr;
  }
  return password_form;
}

std::unique_ptr<PasswordForm> CreatePasswordFormFromUnownedInputElements(
    const WebLocalFrame& frame,
    const FieldValueAndPropertiesMaskMap* field_value_and_properties_map,
    const FormsPredictionsMap* form_predictions,
    UsernameDetectorCache* username_detector_cache) {
  SyntheticForm synthetic_form;
  std::vector<blink::WebElement> fieldsets;
  synthetic_form.control_elements = form_util::GetUnownedFormFieldElements(
      frame.GetDocument().All(), &fieldsets);
  synthetic_form.origin =
      form_util::GetCanonicalOriginForDocument(frame.GetDocument());

  if (synthetic_form.control_elements.empty())
    return nullptr;

  auto password_form = std::make_unique<PasswordForm>();
  if (!UnownedPasswordFormElementsAndFieldSetsToFormData(
          fieldsets, synthetic_form.control_elements, nullptr,
          frame.GetDocument(), field_value_and_properties_map,
          form_util::EXTRACT_VALUE, &password_form->form_data,
          nullptr /* FormFieldData */)) {
    return nullptr;
  }

  if (!GetPasswordForm(std::move(synthetic_form), password_form.get(),
                       field_value_and_properties_map, form_predictions,
                       username_detector_cache)) {
    return nullptr;
  }

  // No actual action on the form, so use the the origin as the action.
  password_form->action = password_form->origin;
  return password_form;
}

bool IsCreditCardVerificationPasswordField(
    const blink::WebInputElement& field) {
  if (!field.IsPasswordFieldForAutofill())
    return false;
  return StringMatchesCVC(field.GetAttribute("id").Utf16()) ||
         StringMatchesCVC(field.GetAttribute("name").Utf16());
}

std::string GetSignOnRealm(const GURL& origin) {
  GURL::Replacements rep;
  rep.SetPathStr("");
  return origin.ReplaceComponents(rep).spec();
}

}  // namespace autofill
