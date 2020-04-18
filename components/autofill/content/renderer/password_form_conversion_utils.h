// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_PASSWORD_FORM_CONVERSION_UTILS_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_PASSWORD_FORM_CONVERSION_UTILS_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_piece.h"
#include "components/autofill/content/renderer/html_based_username_detector.h"
#include "components/autofill/core/common/password_form.h"
#include "components/autofill/core/common/password_form_field_prediction_map.h"
#include "third_party/blink/public/platform/web_string.h"
#include "url/gurl.h"

namespace blink {
class WebFormElement;
class WebFormControlElement;
class WebInputElement;
class WebLocalFrame;
}

namespace re2 {
class RE2;
}

namespace autofill {

struct PasswordForm;

enum UsernameDetectionMethod {
  NO_USERNAME_DETECTED,
  BASE_HEURISTIC,
  HTML_BASED_CLASSIFIER,
  AUTOCOMPLETE_ATTRIBUTE,
  SERVER_SIDE_PREDICTION,
  USERNAME_DETECTION_METHOD_COUNT
};

// The susbset of autocomplete flags related to passwords.
enum class AutocompleteFlag {
  NONE,
  USERNAME,
  CURRENT_PASSWORD,
  NEW_PASSWORD,
  // Represents the whole family of cc-* flags.
  CREDIT_CARD
};

// Returns the AutocompleteFlag derived from |element|'s autocomplete attribute.
AutocompleteFlag AutocompleteFlagForElement(
    const blink::WebInputElement& element);

// The caller of this function is responsible for deleting the returned object.
re2::RE2* CreateMatcher(void* instance, const char* pattern);

// Tests whether the given form is a GAIA reauthentication form.
bool IsGaiaReauthenticationForm(const blink::WebFormElement& form);

// Tests whether the given form is a GAIA form with a skip password argument.
bool IsGaiaWithSkipSavePasswordForm(const blink::WebFormElement& form);

typedef std::map<
    const blink::WebFormControlElement,
    std::pair<std::unique_ptr<base::string16>, FieldPropertiesMask>>
    FieldValueAndPropertiesMaskMap;

// Create a PasswordForm from DOM form. Webkit doesn't allow storing
// custom metadata to DOM nodes, so we have to do this every time an event
// happens with a given form and compare against previously Create'd forms
// to identify..which sucks.
// If an element of |form| has an entry in |nonscript_modified_values|, the
// associated string is used instead of the element's value to create
// the PasswordForm.
// |form_predictions| is Autofill server response, if present it's used for
// overwriting default username element selection.
// |username_detector_cache| is used by the built-in HTML based username
// detector to cache results. Can be null.
std::unique_ptr<PasswordForm> CreatePasswordFormFromWebForm(
    const blink::WebFormElement& form,
    const FieldValueAndPropertiesMaskMap* nonscript_modified_values,
    const FormsPredictionsMap* form_predictions,
    UsernameDetectorCache* username_detector_cache);

// Same as CreatePasswordFormFromWebForm() but for input elements that are not
// enclosed in <form> element.
std::unique_ptr<PasswordForm> CreatePasswordFormFromUnownedInputElements(
    const blink::WebLocalFrame& frame,
    const FieldValueAndPropertiesMaskMap* nonscript_modified_values,
    const FormsPredictionsMap* form_predictions,
    UsernameDetectorCache* username_detector_cache);

// Returns whether the form |field| has a "password" type, but looks like a
// credit card verification field.
bool IsCreditCardVerificationPasswordField(const blink::WebInputElement& field);

// The "Realm" for the sign-on. This is scheme, host, port.
std::string GetSignOnRealm(const GURL& origin);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_PASSWORD_FORM_CONVERSION_UTILS_H__
