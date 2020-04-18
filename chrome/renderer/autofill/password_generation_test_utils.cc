// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/autofill/password_generation_test_utils.h"

#include <base/strings/utf_string_conversions.h>
#include "base/strings/stringprintf.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/test_password_generation_agent.h"
#include "components/autofill/core/common/password_form_generation_data.h"
#include "components/autofill/core/common/signatures_util.h"
#include "net/base/escape.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_form_element.h"

namespace autofill {

namespace {

// Events that should be triggered when Chrome fills a field.
const char* const kEvents[] = {"focus",  "keydown", "input",
                               "change", "keyup",   "blur"};
}  // namespace

void SetNotBlacklistedMessage(TestPasswordGenerationAgent* generation_agent,
                              const char* form_str) {
  std::string escaped_form = net::EscapeQueryParamValue(form_str, false);
  autofill::PasswordForm form;
  form.origin = form_util::StripAuthAndParams(GURL(base::StringPrintf(
      "data:text/html;charset=utf-8,%s", escaped_form.c_str())));
  generation_agent->FormNotBlacklisted(form);
}

// Sends a message that the |form_index| form on the page is valid for
// account creation.
void SetAccountCreationFormsDetectedMessage(
    TestPasswordGenerationAgent* generation_agent,
    blink::WebDocument document,
    int form_index,
    int field_index) {
  blink::WebVector<blink::WebFormElement> web_forms;
  document.Forms(web_forms);

  autofill::FormData form_data;
  WebFormElementToFormData(
      web_forms[form_index], blink::WebFormControlElement(), nullptr,
      form_util::EXTRACT_NONE, &form_data, nullptr /* FormFieldData */);

  std::vector<autofill::PasswordFormGenerationData> forms;
  forms.push_back(autofill::PasswordFormGenerationData{
      CalculateFormSignature(form_data),
      CalculateFieldSignatureForField(form_data.fields[field_index])});
  generation_agent->FoundFormsEligibleForGeneration(forms);
}

// Creates script that registers event listeners for |element_name| field. To
// check whether the listeners are called, check that the variables from
// |variables_to_check| are set to 1.
std::string CreateScriptToRegisterListeners(
    const char* const element_name,
    std::vector<base::string16>* variables_to_check) {
  DCHECK(variables_to_check);
  std::string element = element_name;

  std::string all_scripts = "<script>";
  for (const char* const event : kEvents) {
    std::string script = base::StringPrintf(
        "%s_%s_event = 0;"
        "document.getElementById('%s').on%s = function() {"
        "  %s_%s_event = 1;"
        "};",
        element_name, event, element_name, event, element_name, event);
    all_scripts += script;
    variables_to_check->push_back(base::UTF8ToUTF16(
        base::StringPrintf("%s_%s_event", element_name, event)));
  }

  all_scripts += "</script>";
  return all_scripts;
}

}  // namespace autofill
