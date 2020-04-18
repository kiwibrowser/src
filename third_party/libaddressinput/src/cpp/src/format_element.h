// Copyright (C) 2014 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// An object representing a token in a formatting string. This may be a
// placeholder for a field in the address data such as ADMIN_AREA, a literal
// string such as " ", or a newline.

#ifndef I18N_ADDRESSINPUT_FORMAT_ELEMENT_H_
#define I18N_ADDRESSINPUT_FORMAT_ELEMENT_H_

#include <libaddressinput/address_field.h>

#include <iosfwd>
#include <string>

namespace i18n {
namespace addressinput {

class FormatElement {
 public:
  // Builds a format element to represent |field|.
  explicit FormatElement(AddressField field);

  // Builds an element representing a literal string |literal|.
  explicit FormatElement(const std::string& literal);

  // Builds a newline element.
  FormatElement();

  // Returns true if this element represents a field element.
  bool IsField() const { return literal_.empty(); }

  // Returns true if this element represents a new line.
  bool IsNewline() const { return literal_ == "\n"; }

  AddressField GetField() const { return field_; }
  const std::string& GetLiteral() const { return literal_; }

  bool operator==(const FormatElement& other) const;

 private:
  // The field this element represents. Should only be used if |literal| is an
  // empty string.
  AddressField field_;

  // The literal string for this element. This will be "\n" if this is a
  // newline - but IsNewline() is preferred to determine this. If empty, then
  // this FormatElement represents an address field.
  std::string literal_;
};

}  // namespace addressinput
}  // namespace i18n

// Produces human-readable output in logging, for example in unit tests.
std::ostream& operator<<(std::ostream& o,
                         const i18n::addressinput::FormatElement& element);

#endif  // I18N_ADDRESSINPUT_FORMAT_ELEMENT_H_
