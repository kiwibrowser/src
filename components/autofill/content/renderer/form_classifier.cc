// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/form_classifier.h"

#include <algorithm>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_form_control_element.h"
#include "third_party/blink/public/web/web_input_element.h"

using autofill::form_util::WebFormControlElementToFormField;
using blink::WebFormControlElement;
using blink::WebInputElement;
using blink::WebString;
using blink::WebVector;

namespace autofill {

namespace {

// The words that frequently appear in attribute values of signin forms.
const char* const kSigninTextFeatures[] = {"signin", "login", "logon", "auth"};
constexpr size_t kNumberOfSigninFeatures = arraysize(kSigninTextFeatures);

// The words that frequently appear in attribute values of signup forms.
const char* const kSignupTextFeatures[] = {"signup", "regist", "creat"};
constexpr size_t kNumberOfSignupFeatures = arraysize(kSignupTextFeatures);

// The words that frequently appear in attribute values of captcha elements.
const char* const kCaptchaFeatures[] = {"captcha", "security", "code"};
constexpr size_t kNumberOfCaptchaFeatures = arraysize(kCaptchaFeatures);

// Minimal number of input fields to classify form as signup or change password
// form. If at least one of the listed thresholds is reached or exceeded, the
// form is classified as a form where password generation should be available.
constexpr size_t MINIMAL_NUMBER_OF_TEXT_FIELDS = 2;
constexpr size_t MINIMAL_NUMBER_OF_PASSWORD_FIELDS = 2;
constexpr size_t MINIMAL_NUMBER_OF_CHECKBOX_FIELDS = 3;
constexpr size_t MINIMAL_NUMBER_OF_OTHER_FIELDS = 2;

// Removes some characters from attribute value.
void ClearAttributeValue(std::string* value) {
  value->erase(std::remove_if(value->begin(), value->end(),
                              [](char x) { return x == '-' || x == '_'; }),
               value->end());
}

// Find |features| in |element|'s attribute values. Returns true if at least one
// text feature was found.
bool FindTextFeaturesForClass(const blink::WebElement& element,
                              const char* const features[],
                              size_t number_of_features) {
  DCHECK(features);

  for (unsigned i = 0; i < element.AttributeCount(); ++i) {
    std::string filtered_value =
        base::ToLowerASCII(element.AttributeValue(i).Utf8());
    ClearAttributeValue(&filtered_value);

    if (filtered_value.empty())
      continue;
    for (size_t j = 0; j < number_of_features; ++j) {
      if (filtered_value.find(features[j]) != std::string::npos)
        return true;
    }
  }
  return false;
}

// Returns true if at least one captcha feature was found in |element|'s
// attribute values.
bool IsCaptchaInput(const blink::WebInputElement& element) {
  return FindTextFeaturesForClass(element, kCaptchaFeatures,
                                  kNumberOfCaptchaFeatures);
}

// Finds <img>'s inside |form| and checks if <img>'s attributes contains captcha
// text features. Returns true, if at least one occurrence was found.
bool FindCaptchaInImgElements(const blink::WebElement& form,
                              bool ingnore_invisible) {
  CR_DEFINE_STATIC_LOCAL(WebString, kImageTag, ("img"));

  blink::WebElementCollection img_elements =
      form.GetElementsByHTMLTagName(kImageTag);
  for (blink::WebElement element = img_elements.FirstItem(); !element.IsNull();
       element = img_elements.NextItem()) {
    if (ingnore_invisible && !form_util::IsWebElementVisible(element))
      continue;
    if (FindTextFeaturesForClass(element, kCaptchaFeatures,
                                 kNumberOfCaptchaFeatures)) {
      return true;
    }
  }
  return false;
}

// Finds signin and signup features in |element|'s attribute values. Sets to
// true |found_signin_text_features| or |found_signup_text_features| if
// appropriate features were found.
void FindTextFeaturesInElement(const blink::WebElement& element,
                               bool* found_signin_text_features,
                               bool* found_signup_text_features) {
  DCHECK(found_signin_text_features);
  DCHECK(found_signup_text_features);

  if (!*found_signin_text_features) {
    *found_signin_text_features = FindTextFeaturesForClass(
        element, kSigninTextFeatures, kNumberOfSigninFeatures);
  }
  if (!*found_signup_text_features) {
    *found_signup_text_features = FindTextFeaturesForClass(
        element, kSignupTextFeatures, kNumberOfSignupFeatures);
  }
}

// Returns true if |element| has type "button" or "image".
bool IsButtonOrImageElement(const WebFormControlElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kButton, ("button"));
  CR_DEFINE_STATIC_LOCAL(WebString, kImage, ("image"));

  return element.FormControlTypeForAutofill() == kButton ||
         element.FormControlTypeForAutofill() == kImage;
}

// Returns true if |element| has type "submit".
bool IsSubmitElement(const WebFormControlElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kSubmit, ("submit"));

  return element.FormControlTypeForAutofill() == kSubmit;
}

// Returns true if |element| has type "hidden";
bool IsHiddenElement(const WebFormControlElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kHidden, ("hidden"));

  return element.FormControlTypeForAutofill() == kHidden;
}

// Returns true if |element| has type "select-multiple" or "select-one".
bool IsSelectElement(const WebFormControlElement& element) {
  CR_DEFINE_STATIC_LOCAL(WebString, kSelectOne, ("select-one"));
  CR_DEFINE_STATIC_LOCAL(WebString, kSelectMultiple, ("select-multiple"));

  return element.FormControlTypeForAutofill() == kSelectOne ||
         element.FormControlTypeForAutofill() == kSelectMultiple;
}

// Return true if |form| contains at least one visible password element.
bool FormContainsVisiblePasswordFields(const blink::WebFormElement& form) {
  WebVector<WebFormControlElement> control_elements;
  form.GetFormControlElements(control_elements);
  for (auto& control_element : control_elements) {
    const WebInputElement* input_element = ToWebInputElement(&control_element);
    if (!input_element)
      continue;
    if (input_element->IsPasswordFieldForAutofill() &&
        form_util::IsWebElementVisible(*input_element)) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool ClassifyFormAndFindGenerationField(const blink::WebFormElement& form,
                                        base::string16* generation_field) {
  DCHECK(generation_field);

  if (form.IsNull())
    return false;

  bool ignore_invisible_elements = FormContainsVisiblePasswordFields(form);

  bool found_signin_text_features = false;
  bool found_signup_text_features = false;
  size_t number_of_text_input_fields = 0;
  size_t number_of_password_input_fields = 0;
  size_t number_of_checkbox_input_fields = 0;
  size_t number_of_other_input_fields = 0;
  bool found_captcha =
      FindCaptchaInImgElements(form, ignore_invisible_elements);

  FindTextFeaturesInElement(form, &found_signin_text_features,
                            &found_signup_text_features);

  std::vector<WebInputElement> passwords;
  WebVector<WebFormControlElement> control_elements;
  form.GetFormControlElements(control_elements);

  for (const WebFormControlElement& control_element : control_elements) {
    if (IsHiddenElement(control_element))
      continue;
    if (ignore_invisible_elements) {
      if (!form_util::IsWebElementVisible(control_element))
        continue;
    }

    // If type="button" or "image", skip them, because it might be a link
    // to another form.
    if (IsButtonOrImageElement(control_element))
      continue;

    FindTextFeaturesInElement(control_element, &found_signin_text_features,
                              &found_signup_text_features);

    // Since <select> is not WebInputElement, but WebSelectElement, process
    // them as a special case.
    if (IsSelectElement(control_element)) {
      number_of_other_input_fields++;
    } else {
      const WebInputElement* input_element =
          ToWebInputElement(&control_element);
      if (!input_element)
        continue;

      if (input_element->IsTextField()) {
        if (input_element->IsPasswordFieldForAutofill()) {
          ++number_of_password_input_fields;
          passwords.push_back(*input_element);
        } else {
          ++number_of_text_input_fields;
          found_captcha = found_captcha || IsCaptchaInput(*input_element);
        }
      } else {  // Non-text fields.
        if (input_element->IsCheckbox())
          ++number_of_checkbox_input_fields;
        else if (!IsSubmitElement(*input_element))
          ++number_of_other_input_fields;
      }
    }
  }

  if (number_of_password_input_fields == 0 ||
      number_of_password_input_fields > 3)
    return false;

  if ((number_of_text_input_fields - found_captcha >=
           MINIMAL_NUMBER_OF_TEXT_FIELDS ||
       number_of_password_input_fields >= MINIMAL_NUMBER_OF_PASSWORD_FIELDS ||
       number_of_checkbox_input_fields >= MINIMAL_NUMBER_OF_CHECKBOX_FIELDS ||
       number_of_other_input_fields >= MINIMAL_NUMBER_OF_OTHER_FIELDS) ||
      (found_signup_text_features && !found_signin_text_features)) {
    WebInputElement password_creation_field;

    // TODO(crbug.com/618309): Improve local classifier to distinguish password
    // creation and password usage fields on the change password forms.
    if (passwords.size() == 3)
      password_creation_field = passwords[1];
    else
      password_creation_field = passwords[0];

    *generation_field = password_creation_field.NameForAutofill().Utf16();
    return true;
  }
  return false;
}
}
