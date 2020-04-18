// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/form_field_data.h"

#include "base/pickle.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_util.h"

namespace autofill {

namespace {

// Increment this anytime pickle format is modified as well as provide
// deserialization routine from previous kPickleVersion format.
const int kPickleVersion = 7;

void AddVectorToPickle(std::vector<base::string16> strings,
                       base::Pickle* pickle) {
  pickle->WriteInt(static_cast<int>(strings.size()));
  for (size_t i = 0; i < strings.size(); ++i) {
    pickle->WriteString16(strings[i]);
  }
}

bool ReadStringVector(base::PickleIterator* iter,
                      std::vector<base::string16>* strings) {
  int size;
  if (!iter->ReadInt(&size))
    return false;

  base::string16 pickle_data;
  for (int i = 0; i < size; i++) {
    if (!iter->ReadString16(&pickle_data))
      return false;

    strings->push_back(pickle_data);
  }
  return true;
}

template <typename T>
bool ReadAsInt(base::PickleIterator* iter, T* target_value) {
  int pickle_data;
  if (!iter->ReadInt(&pickle_data))
    return false;

  *target_value = static_cast<T>(pickle_data);
  return true;
}

bool DeserializeSection1(base::PickleIterator* iter,
                         FormFieldData* field_data) {
  return iter->ReadString16(&field_data->label) &&
         iter->ReadString16(&field_data->name) &&
         iter->ReadString16(&field_data->value) &&
         iter->ReadString(&field_data->form_control_type) &&
         iter->ReadString(&field_data->autocomplete_attribute) &&
         iter->ReadUInt64(&field_data->max_length) &&
         iter->ReadBool(&field_data->is_autofilled);
}

bool DeserializeSection5(base::PickleIterator* iter,
                         FormFieldData* field_data) {
  bool is_checked = false;
  bool is_checkable = false;
  const bool success =
      iter->ReadBool(&is_checked) && iter->ReadBool(&is_checkable);

  if (success)
    SetCheckStatus(field_data, is_checkable, is_checked);

  return success;
}

bool DeserializeSection6(base::PickleIterator* iter,
                         FormFieldData* field_data) {
  return ReadAsInt(iter, &field_data->check_status);
  ;
}

bool DeserializeSection7(base::PickleIterator* iter,
                         FormFieldData* field_data) {
  return iter->ReadBool(&field_data->is_focusable) &&
         iter->ReadBool(&field_data->should_autocomplete);
}

bool DeserializeSection3(base::PickleIterator* iter,
                         FormFieldData* field_data) {
  return ReadAsInt(iter, &field_data->text_direction) &&
         ReadStringVector(iter, &field_data->option_values) &&
         ReadStringVector(iter, &field_data->option_contents);
}

bool DeserializeSection2(base::PickleIterator* iter,
                         FormFieldData* field_data) {
  return ReadAsInt(iter, &field_data->role);
}

bool DeserializeSection4(base::PickleIterator* iter,
                         FormFieldData* field_data) {
  return iter->ReadString16(&field_data->placeholder);
}

bool DeserializeSection8(base::PickleIterator* iter,
                         FormFieldData* field_data) {
  return iter->ReadString16(&field_data->css_classes);
}

bool DeserializeSection9(base::PickleIterator* iter,
                         FormFieldData* field_data) {
  return iter->ReadUInt32(&field_data->properties_mask);
}

bool DeserializeSection10(base::PickleIterator* iter,
                          FormFieldData* field_data) {
  return iter->ReadString16(&field_data->id);
}

bool HaveSameLabel(const FormFieldData& field1, const FormFieldData& field2) {
  if (field1.label == field2.label)
    return true;

  // Assume the labels same if they come from same source but not LABEL tag
  // when kAutofillSkipComparingInferredLabels is enabled.
  if (base::FeatureList::IsEnabled(
          features::kAutofillSkipComparingInferredLabels)) {
    return field1.label_source == field2.label_source &&
           field1.label_source != FormFieldData::LABEL_TAG;
  }
  return false;
}

}  // namespace

FormFieldData::FormFieldData()
    : max_length(0),
      is_autofilled(false),
      check_status(NOT_CHECKABLE),
      is_focusable(true),
      should_autocomplete(true),
      role(ROLE_ATTRIBUTE_OTHER),
      text_direction(base::i18n::UNKNOWN_DIRECTION),
      properties_mask(0),
      is_enabled(false),
      is_readonly(false),
      is_default(false),
      label_source(LabelSource::UNKNOWN) {}

FormFieldData::FormFieldData(const FormFieldData& other) = default;

FormFieldData::~FormFieldData() {}

bool FormFieldData::SameFieldAs(const FormFieldData& field) const {
  // A FormFieldData stores a value, but the value is not part of the identity
  // of the field, so we don't want to compare the values.
  // Similarly, flags like is_enabled, which are only used for parsing but are
  // not stored persistently, are not used for comparison.
  // is_autofilled and section are also secondary properties of a field. Two
  // fields could be the same, and have different sections, because the section
  // is updated for one, but not for the other.
  return name == field.name && id == field.id &&
         form_control_type == field.form_control_type &&
         autocomplete_attribute == field.autocomplete_attribute &&
         placeholder == field.placeholder && max_length == field.max_length &&
         css_classes == field.css_classes &&
         // is_checked and is_autofilled counts as "value" since these change
         // when we fill things in.
         IsCheckable(check_status) == IsCheckable(field.check_status) &&
         is_focusable == field.is_focusable &&
         should_autocomplete == field.should_autocomplete &&
         role == field.role && text_direction == field.text_direction &&
         HaveSameLabel(*this, field);
  // The option values/contents which are the list of items in the list
  // of a drop-down are currently not considered part of the identity of
  // a form element. This is debatable, since one might base heuristics
  // on the types of elements that are available. Alternatively, one
  // could imagine some forms that dynamically change the element
  // contents (say, insert years starting from the current year) that
  // should not be considered changes in the structure of the form.
}

bool FormFieldData::SimilarFieldAs(const FormFieldData& field) const {
  return HaveSameLabel(*this, field) && name == field.name && id == field.id &&
         form_control_type == field.form_control_type &&
         IsCheckable(check_status) == IsCheckable(field.check_status);
}

bool FormFieldData::IsTextInputElement() const {
  return form_control_type == "text" || form_control_type == "password" ||
         form_control_type == "search" || form_control_type == "tel" ||
         form_control_type == "url" || form_control_type == "email";
}

bool FormFieldData::operator==(const FormFieldData& field) const {
  return SameFieldAs(field) && unique_renderer_id == field.unique_renderer_id &&
         is_autofilled == field.is_autofilled &&
         check_status == field.check_status &&
         option_values == field.option_values &&
         option_contents == field.option_contents &&
         properties_mask == field.properties_mask;
}

bool FormFieldData::operator!=(const FormFieldData& field) const {
  return !(*this == field);
}

bool FormFieldData::operator<(const FormFieldData& field) const {
  // This does not use std::tie() as that generates more implicit variables
  // than the max-vartrack-size for var-tracking-assignments when compiling
  // for Android, producing build warnings. (See https://crbug.com/555171 for
  // context.)

  // Like SameFieldAs this ignores the value.
  if (label < field.label)
    return true;
  if (label > field.label)
    return false;
  if (name < field.name)
    return true;
  if (name > field.name)
    return false;
  if (id < field.id)
    return true;
  if (id > field.id)
    return false;
  if (form_control_type < field.form_control_type)
    return true;
  if (form_control_type > field.form_control_type)
    return false;
  if (autocomplete_attribute < field.autocomplete_attribute)
    return true;
  if (autocomplete_attribute > field.autocomplete_attribute)
    return false;
  if (placeholder < field.placeholder)
    return true;
  if (placeholder > field.placeholder)
    return false;
  if (max_length < field.max_length)
    return true;
  if (max_length > field.max_length)
    return false;
  if (css_classes < field.css_classes)
    return true;
  if (css_classes > field.css_classes)
    return false;
  // Skip |is_checked| and |is_autofilled| as in SameFieldAs.
  if (IsCheckable(check_status) < IsCheckable(field.check_status))
    return true;
  if (IsCheckable(check_status) > IsCheckable(field.check_status))
    return false;
  if (is_focusable < field.is_focusable)
    return true;
  if (is_focusable > field.is_focusable)
    return false;
  if (should_autocomplete < field.should_autocomplete)
    return true;
  if (should_autocomplete > field.should_autocomplete)
    return false;
  if (role < field.role)
    return true;
  if (role > field.role)
    return false;
  if (text_direction < field.text_direction)
    return true;
  if (text_direction > field.text_direction)
    return false;
  // See SameFieldAs above for why we don't check option_values/contents and
  // flags like is_enabled.
  return false;
}

void SerializeFormFieldData(const FormFieldData& field_data,
                            base::Pickle* pickle) {
  pickle->WriteInt(kPickleVersion);
  pickle->WriteString16(field_data.label);
  pickle->WriteString16(field_data.name);
  pickle->WriteString16(field_data.value);
  pickle->WriteString(field_data.form_control_type);
  pickle->WriteString(field_data.autocomplete_attribute);
  pickle->WriteUInt64(field_data.max_length);
  pickle->WriteBool(field_data.is_autofilled);
  pickle->WriteInt(field_data.check_status);
  pickle->WriteBool(field_data.is_focusable);
  pickle->WriteBool(field_data.should_autocomplete);
  pickle->WriteInt(field_data.role);
  pickle->WriteInt(field_data.text_direction);
  AddVectorToPickle(field_data.option_values, pickle);
  AddVectorToPickle(field_data.option_contents, pickle);
  pickle->WriteString16(field_data.placeholder);
  pickle->WriteString16(field_data.css_classes);
  pickle->WriteUInt32(field_data.properties_mask);
  pickle->WriteString16(field_data.id);
}

bool DeserializeFormFieldData(base::PickleIterator* iter,
                              FormFieldData* field_data) {
  int version;
  FormFieldData temp_form_field_data;
  if (!iter->ReadInt(&version)) {
    LOG(ERROR) << "Bad pickle of FormFieldData, no version present";
    return false;
  }

  switch (version) {
    case 1: {
      if (!DeserializeSection1(iter, &temp_form_field_data) ||
          !DeserializeSection5(iter, &temp_form_field_data) ||
          !DeserializeSection7(iter, &temp_form_field_data) ||
          !DeserializeSection3(iter, &temp_form_field_data)) {
        LOG(ERROR) << "Could not deserialize FormFieldData from pickle";
        return false;
      }
      break;
    }
    case 2: {
      if (!DeserializeSection1(iter, &temp_form_field_data) ||
          !DeserializeSection5(iter, &temp_form_field_data) ||
          !DeserializeSection7(iter, &temp_form_field_data) ||
          !DeserializeSection2(iter, &temp_form_field_data) ||
          !DeserializeSection3(iter, &temp_form_field_data)) {
        LOG(ERROR) << "Could not deserialize FormFieldData from pickle";
        return false;
      }
      break;
    }
    case 3: {
      if (!DeserializeSection1(iter, &temp_form_field_data) ||
          !DeserializeSection5(iter, &temp_form_field_data) ||
          !DeserializeSection7(iter, &temp_form_field_data) ||
          !DeserializeSection2(iter, &temp_form_field_data) ||
          !DeserializeSection3(iter, &temp_form_field_data) ||
          !DeserializeSection4(iter, &temp_form_field_data)) {
        LOG(ERROR) << "Could not deserialize FormFieldData from pickle";
        return false;
      }
      break;
    }
    case 4: {
      if (!DeserializeSection1(iter, &temp_form_field_data) ||
          !DeserializeSection6(iter, &temp_form_field_data) ||
          !DeserializeSection7(iter, &temp_form_field_data) ||
          !DeserializeSection2(iter, &temp_form_field_data) ||
          !DeserializeSection3(iter, &temp_form_field_data) ||
          !DeserializeSection4(iter, &temp_form_field_data)) {
        LOG(ERROR) << "Could not deserialize FormFieldData from pickle";
        return false;
      }
      break;
    }
    case 5: {
      if (!DeserializeSection1(iter, &temp_form_field_data) ||
          !DeserializeSection6(iter, &temp_form_field_data) ||
          !DeserializeSection7(iter, &temp_form_field_data) ||
          !DeserializeSection2(iter, &temp_form_field_data) ||
          !DeserializeSection3(iter, &temp_form_field_data) ||
          !DeserializeSection4(iter, &temp_form_field_data) ||
          !DeserializeSection8(iter, &temp_form_field_data)) {
        LOG(ERROR) << "Could not deserialize FormFieldData from pickle";
        return false;
      }
      break;
    }
    case 6: {
      if (!DeserializeSection1(iter, &temp_form_field_data) ||
          !DeserializeSection6(iter, &temp_form_field_data) ||
          !DeserializeSection7(iter, &temp_form_field_data) ||
          !DeserializeSection2(iter, &temp_form_field_data) ||
          !DeserializeSection3(iter, &temp_form_field_data) ||
          !DeserializeSection4(iter, &temp_form_field_data) ||
          !DeserializeSection8(iter, &temp_form_field_data) ||
          !DeserializeSection9(iter, &temp_form_field_data)) {
        LOG(ERROR) << "Could not deserialize FormFieldData from pickle";
        return false;
      }
      break;
    }
    case 7: {
      if (!DeserializeSection1(iter, &temp_form_field_data) ||
          !DeserializeSection6(iter, &temp_form_field_data) ||
          !DeserializeSection7(iter, &temp_form_field_data) ||
          !DeserializeSection2(iter, &temp_form_field_data) ||
          !DeserializeSection3(iter, &temp_form_field_data) ||
          !DeserializeSection4(iter, &temp_form_field_data) ||
          !DeserializeSection8(iter, &temp_form_field_data) ||
          !DeserializeSection9(iter, &temp_form_field_data) ||
          !DeserializeSection10(iter, &temp_form_field_data)) {
        LOG(ERROR) << "Could not deserialize FormFieldData from pickle";
        return false;
      }
      break;
    }
    default: {
      LOG(ERROR) << "Unknown FormFieldData pickle version " << version;
      return false;
    }
  }
  *field_data = temp_form_field_data;
  return true;
}

std::ostream& operator<<(std::ostream& os, const FormFieldData& field) {
  const char* check_status_str = nullptr;
  switch (field.check_status) {
    case FormFieldData::CheckStatus::NOT_CHECKABLE:
      check_status_str = "NOT_CHECKABLE";
      break;
    case FormFieldData::CheckStatus::CHECKABLE_BUT_UNCHECKED:
      check_status_str = "CHECKABLE_BUT_UNCHECKED";
      break;
    case FormFieldData::CheckStatus::CHECKED:
      check_status_str = "CHECKED";
      break;
    default:
      NOTREACHED();
      check_status_str = "<invalid>";
  }

  const char* role_str = nullptr;
  switch (field.role) {
    case FormFieldData::RoleAttribute::ROLE_ATTRIBUTE_PRESENTATION:
      role_str = "ROLE_ATTRIBUTE_PRESENTATION";
      break;
    case FormFieldData::RoleAttribute::ROLE_ATTRIBUTE_OTHER:
      role_str = "ROLE_ATTRIBUTE_OTHER";
      break;
    default:
      NOTREACHED();
      role_str = "<invalid>";
  }

  return os << "label='" << base::UTF16ToUTF8(field.label) << "' "
            << "name='" << base::UTF16ToUTF8(field.name) << "' "
            << "id='" << base::UTF16ToUTF8(field.id) << "' "
            << "value='" << base::UTF16ToUTF8(field.value) << "' "
            << "control='" << field.form_control_type << "' "
            << "autocomplete='" << field.autocomplete_attribute << "' "
            << "placeholder='" << field.placeholder << "' "
            << "max_length=" << field.max_length << " "
            << "css_classes='" << field.css_classes << "' "
            << "autofilled=" << field.is_autofilled << " "
            << "check_status=" << check_status_str << " "
            << "is_focusable=" << field.is_focusable << " "
            << "should_autocomplete=" << field.should_autocomplete << " "
            << "role=" << role_str << " "
            << "text_direction=" << field.text_direction << " "
            << "is_enabled=" << field.is_enabled << " "
            << "is_readonly=" << field.is_readonly << " "
            << "is_default=" << field.is_default << " "
            << "typed_value=" << field.typed_value << " "
            << "properties_mask=" << field.properties_mask << " "
            << "label_source=" << field.label_source;
}

}  // namespace autofill
