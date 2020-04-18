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
// A struct for storing address data: country code, administrative area,
// locality, etc. The field names correspond to the OASIS xAL standard:
// https://www.oasis-open.org/committees/ciq/download.shtml

#ifndef I18N_ADDRESSINPUT_ADDRESS_DATA_H_
#define I18N_ADDRESSINPUT_ADDRESS_DATA_H_

#include <libaddressinput/address_field.h>

#include <iosfwd>
#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

struct AddressData {
  // CLDR (Common Locale Data Repository) region code.
  std::string region_code;

  // The address lines represent the most specific part of any address.
  std::vector<std::string> address_line;

  // Top-level administrative subdivision of this country.
  std::string administrative_area;

  // Generally refers to the city/town portion of an address.
  std::string locality;

  // Dependent locality or sublocality. Used for UK dependent localities, or
  // neighborhoods or boroughs in other locations.
  std::string dependent_locality;

  // Values are frequently alphanumeric.
  std::string postal_code;

  // This corresponds to the SortingCode sub-element of the xAL
  // PostalServiceElements element. Use is very country-specific.
  std::string sorting_code;

  // Language code of the address. Should be in BCP-47 format.
  std::string language_code;

  // The organization, firm, company, or institution at this address. This
  // corresponds to the FirmName sub-element of the xAL FirmType element.
  std::string organization;

  // Name of recipient or contact person. Not present in xAL.
  std::string recipient;

  // Returns whether the |field| is empty.
  bool IsFieldEmpty(AddressField field) const;

  // Returns the value of the |field|. The parameter must not be STREET_ADDRESS,
  // which comprises multiple fields (will crash otherwise).
  const std::string& GetFieldValue(AddressField field) const;

  // Copies |value| into the |field|. The parameter must not be STREET_ADDRESS,
  // which comprises multiple fields (will crash otherwise).
  void SetFieldValue(AddressField field, const std::string& value);

  // Returns the value of the |field|. The parameter must be STREET_ADDRESS,
  // which comprises multiple fields (will crash otherwise).
  const std::vector<std::string>& GetRepeatedFieldValue(
      AddressField field) const;

  bool operator==(const AddressData& other) const;

  // Returns true if the parameter comprises multiple fields, false otherwise.
  // Use it to determine whether to call |GetFieldValue| or
  // |GetRepeatedFieldValue|.
  static bool IsRepeatedFieldValue(AddressField field);
};

}  // namespace addressinput
}  // namespace i18n

// Produces human-readable output in logging, for example in unit tests.
std::ostream& operator<<(std::ostream& o,
                         const i18n::addressinput::AddressData& address);

#endif  // I18N_ADDRESSINPUT_ADDRESS_DATA_H_
