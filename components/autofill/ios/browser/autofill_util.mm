// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/ios/browser/autofill_util.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/common/autofill_util.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace autofill {

bool IsContextSecureForWebState(web::WebState* web_state) {
  // This implementation differs slightly from other platforms. Other platforms'
  // implementations check for the presence of active mixed content, but because
  // the iOS web view blocks active mixed content without an option to run it,
  // there is no need to check for active mixed content here.
  web::NavigationManager* manager = web_state->GetNavigationManager();
  web::NavigationItem* nav_item = manager->GetLastCommittedItem();
  if (!nav_item)
    return false;

  const web::SSLStatus& ssl = nav_item->GetSSL();
  return nav_item->GetURL().SchemeIsCryptographic() && ssl.certificate &&
         (!net::IsCertStatusError(ssl.cert_status) ||
          net::IsCertStatusMinorError(ssl.cert_status));
}

std::unique_ptr<base::Value> ParseJson(NSString* json_string) {
  // Convert JSON string to JSON object |JSONValue|.
  int error_code = 0;
  std::string error_message;
  std::unique_ptr<base::Value> json_value(base::JSONReader::ReadAndReturnError(
      base::SysNSStringToUTF8(json_string), base::JSON_PARSE_RFC, &error_code,
      &error_message));
  if (error_code)
    return nullptr;

  return json_value;
}

bool ExtractFormsData(NSString* forms_json,
                      bool filtered,
                      const base::string16& form_name,
                      const GURL& page_url,
                      std::vector<FormData>* forms_data) {
  DCHECK(forms_data);
  std::unique_ptr<base::Value> forms_value = ParseJson(forms_json);
  if (!forms_value)
    return false;

  // Returned data should be a list of forms.
  const base::ListValue* forms_list = nullptr;
  if (!forms_value->GetAsList(&forms_list))
    return false;

  // Iterate through all the extracted forms and copy the data from JSON into
  // AutofillManager structures.
  for (const auto& form_dict : *forms_list) {
    autofill::FormData form;
    if (ExtractFormData(form_dict, filtered, form_name, page_url, &form))
      forms_data->push_back(std::move(form));
  }
  return true;
}

bool ExtractFormData(const base::Value& form_value,
                     bool filtered,
                     const base::string16& form_name,
                     const GURL& page_url,
                     autofill::FormData* form_data) {
  DCHECK(form_data);
  // Each form should be a JSON dictionary.
  const base::DictionaryValue* form_dictionary = nullptr;
  if (!form_value.GetAsDictionary(&form_dictionary))
    return false;

  // Form data is copied into a FormData object field-by-field.
  if (!form_dictionary->GetString("name", &form_data->name))
    return false;
  if (filtered && form_name != form_data->name)
    return false;

  // Origin is mandatory.
  base::string16 origin;
  if (!form_dictionary->GetString("origin", &origin))
    return false;

  // Use GURL object to verify origin of host page URL.
  form_data->origin = GURL(origin);
  if (form_data->origin.GetOrigin() != page_url.GetOrigin())
    return false;

  // main_frame_origin is used for logging UKM.
  form_data->main_frame_origin = url::Origin::Create(page_url);

  // Action is optional.
  base::string16 action;
  form_dictionary->GetString("action", &action);
  form_data->action = GURL(action);

  // Optional fields.
  form_dictionary->GetBoolean("is_form_tag", &form_data->is_form_tag);
  form_dictionary->GetBoolean("is_formless_checkout",
                              &form_data->is_formless_checkout);

  // Field list (mandatory) is extracted.
  const base::ListValue* fields_list = nullptr;
  if (!form_dictionary->GetList("fields", &fields_list))
    return false;
  for (const auto& field_dict : *fields_list) {
    const base::DictionaryValue* field;
    autofill::FormFieldData field_data;
    if (field_dict.GetAsDictionary(&field) &&
        ExtractFormFieldData(*field, &field_data)) {
      form_data->fields.push_back(std::move(field_data));
    } else {
      return false;
    }
  }
  return true;
}

bool ExtractFormFieldData(const base::DictionaryValue& field,
                          autofill::FormFieldData* field_data) {
  if (!field.GetString("name", &field_data->name) ||
      !field.GetString("identifier", &field_data->id) ||
      !field.GetString("form_control_type", &field_data->form_control_type)) {
    return false;
  }

  // Optional fields.
  field.GetString("label", &field_data->label);
  field.GetString("value", &field_data->value);
  field.GetString("autocomplete_attribute",
                  &field_data->autocomplete_attribute);
  field.GetBoolean("is_autofilled", &field_data->is_autofilled);

  int max_length = 0;
  if (field.GetInteger("max_length", &max_length))
    field_data->max_length = max_length;

  // TODO(crbug.com/427614): Extract |is_checked|.
  bool is_checkable = false;
  field.GetBoolean("is_checkable", &is_checkable);
  autofill::SetCheckStatus(field_data, is_checkable, false);

  field.GetBoolean("is_focusable", &field_data->is_focusable);
  field.GetBoolean("should_autocomplete", &field_data->should_autocomplete);

  // ROLE_ATTRIBUTE_OTHER is the default value. The only other value as of this
  // writing is ROLE_ATTRIBUTE_PRESENTATION.
  int role = 0;
  if (field.GetInteger("role", &role) &&
      role == autofill::AutofillField::ROLE_ATTRIBUTE_PRESENTATION) {
    field_data->role = autofill::AutofillField::ROLE_ATTRIBUTE_PRESENTATION;
  }

  // TODO(crbug.com/427614): Extract |text_direction|.

  // Load option values where present.
  const base::ListValue* option_values = nullptr;
  if (field.GetList("option_values", &option_values)) {
    for (const auto& optionValue : *option_values) {
      base::string16 value;
      if (optionValue.GetAsString(&value))
        field_data->option_values.push_back(std::move(value));
    }
  }

  // Load option contents where present.
  const base::ListValue* option_contents = nullptr;
  if (field.GetList("option_contents", &option_contents)) {
    for (const auto& option_content : *option_contents) {
      base::string16 content;
      if (option_content.GetAsString(&content))
        field_data->option_contents.push_back(std::move(content));
    }
  }

  return field_data->option_values.size() == field_data->option_contents.size();
}

}  // namespace autofill
