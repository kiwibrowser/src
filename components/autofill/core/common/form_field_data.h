// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_COMMON_FORM_FIELD_DATA_H_
#define COMPONENTS_AUTOFILL_CORE_COMMON_FORM_FIELD_DATA_H_

#include <stddef.h>

#include <limits>
#include <vector>

#include "base/i18n/rtl.h"
#include "base/strings/string16.h"

namespace base {
class Pickle;
class PickleIterator;
}

namespace autofill {

// The flags describing form field properties.
enum FieldPropertiesFlags {
  NO_FLAGS = 0u,
  USER_TYPED = 1u << 0,
  // AUTOFILLED means that at least one character of the field value comes from
  // being autofilled. This is different from
  // WebFormControlElement::IsAutofilled(). It is meant to be used for password
  // fields, to determine whether viewing the value needs user reauthentication.
  AUTOFILLED = 1u << 1,
  HAD_FOCUS = 1u << 2,
  // Use this flag, if some error occurred in flags processing.
  ERROR_OCCURRED = 1u << 3,
  // On submission, the value of the field was recognised as a value which is
  // already stored.
  KNOWN_VALUE = 1u << 4
};

// FieldPropertiesMask is used to contain combinations of FieldPropertiesFlags
// values.
typedef uint32_t FieldPropertiesMask;

// Stores information about a field in a form.
struct FormFieldData {
  // Copied to components/autofill/ios/browser/resources/autofill_controller.js.
  enum RoleAttribute {
    // "presentation"
    ROLE_ATTRIBUTE_PRESENTATION,
    // Anything else.
    ROLE_ATTRIBUTE_OTHER,
  };

  enum CheckStatus {
    NOT_CHECKABLE,
    CHECKABLE_BUT_UNCHECKED,
    CHECKED,
  };

  // From which source the label is inferred.
  enum LabelSource {
    UNKNOWN,  // The source is unknown.
    LABEL_TAG,
    P_TAG,
    DIV_TABLE,
    TD_TAG,
    DD_TAG,
    LI_TAG,
    PLACE_HOLDER,
    ARIA_LABEL,
    COMBINED,  // Combined with various elements.
    VALUE,     // label is the value of element.
  };

  static constexpr uint32_t kNotSetFormControlRendererId =
      std::numeric_limits<uint32_t>::max();

  FormFieldData();
  FormFieldData(const FormFieldData& other);
  ~FormFieldData();

  // Returns true if two form fields are the same, not counting the value.
  bool SameFieldAs(const FormFieldData& field) const;

  // SameFieldAs() is a little restricted when field's style changed
  // dynamically, like css.
  // This method only compares critical attributes of field to check whether
  // they are similar enough to be considered as same field if form's
  // other information isn't changed.
  bool SimilarFieldAs(const FormFieldData& field) const;

  // Returns true for all of textfield-looking types such as text, password,
  // search, email, url, and number. It must work the same way as Blink function
  // WebInputElement::IsTextField(), and it returns false if |*this| represents
  // a textarea.
  bool IsTextInputElement() const;

  // Note: operator==() performs a full-field-comparison(byte by byte), this is
  // different from SameFieldAs(), which ignores comparison for those "values"
  // not regarded as part of identity of the field, such as is_autofilled and
  // the option_values/contents etc.
  bool operator==(const FormFieldData& field) const;
  bool operator!=(const FormFieldData& field) const;
  // Comparison operator exposed for STL map. Uses label, then name to sort.
  bool operator<(const FormFieldData& field) const;

  // If you add more, be sure to update the comparison operators, SameFieldAs,
  // serializing functions (in the .cc file) and the constructor.
  base::string16 label;
  base::string16 name;
  base::string16 id;
  base::string16 value;
  std::string form_control_type;
  std::string autocomplete_attribute;
  base::string16 placeholder;
  base::string16 css_classes;
  // Unique renderer id which is returned by function
  // WebFormControlElement::UniqueRendererFormControlId(). It is not persistant
  // between page loads, so it is not saved and not used in comparison in
  // SameFieldAs().
  uint32_t unique_renderer_id = kNotSetFormControlRendererId;

  // The unique identifier of the section (e.g. billing vs. shipping address)
  // of this field.
  std::string section;

  // Note: we use uint64_t instead of size_t because this struct is sent over
  // IPC which could span 32 & 64 bit processes. We chose uint64_t instead of
  // uint32_t to maintain compatibility with old code which used size_t
  // (base::Pickle used to serialize that as 64 bit).
  uint64_t max_length;
  bool is_autofilled;
  CheckStatus check_status;
  bool is_focusable;
  bool should_autocomplete;
  RoleAttribute role;
  base::i18n::TextDirection text_direction;
  FieldPropertiesMask properties_mask;

  // Data members from the next block are used for parsing only, they are not
  // serialised for storage.
  bool is_enabled;
  bool is_readonly;
  bool is_default;
  base::string16 typed_value;

  // For the HTML snippet |<option value="US">United States</option>|, the
  // value is "US" and the contents are "United States".
  std::vector<base::string16> option_values;
  std::vector<base::string16> option_contents;

  // Password Manager doesn't use labels nor client side nor server side, so
  // label_source isn't in serialize methods.
  LabelSource label_source;
};

// Serialize and deserialize FormFieldData. These are used when FormData objects
// are serialized and deserialized.
void SerializeFormFieldData(const FormFieldData& form_field_data,
                            base::Pickle* serialized);
bool DeserializeFormFieldData(base::PickleIterator* pickle_iterator,
                              FormFieldData* form_field_data);

// So we can compare FormFieldDatas with EXPECT_EQ().
std::ostream& operator<<(std::ostream& os, const FormFieldData& field);

// Prefer to use this macro in place of |EXPECT_EQ()| for comparing
// |FormFieldData|s in test code.
#define EXPECT_FORM_FIELD_DATA_EQUALS(expected, actual)                        \
  do {                                                                         \
    EXPECT_EQ(expected.label, actual.label);                                   \
    EXPECT_EQ(expected.name, actual.name);                                     \
    EXPECT_EQ(expected.value, actual.value);                                   \
    EXPECT_EQ(expected.form_control_type, actual.form_control_type);           \
    EXPECT_EQ(expected.autocomplete_attribute, actual.autocomplete_attribute); \
    EXPECT_EQ(expected.placeholder, actual.placeholder);                       \
    EXPECT_EQ(expected.max_length, actual.max_length);                         \
    EXPECT_EQ(expected.css_classes, actual.css_classes);                       \
    EXPECT_EQ(expected.is_autofilled, actual.is_autofilled);                   \
    EXPECT_EQ(expected.section, actual.section);                               \
    EXPECT_EQ(expected.check_status, actual.check_status);                     \
    EXPECT_EQ(expected.properties_mask, actual.properties_mask);               \
    EXPECT_EQ(expected.id, actual.id);                                         \
  } while (0)

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_COMMON_FORM_FIELD_DATA_H_
