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
// See the License for the specific language_code governing permissions and
// limitations under the License.

#include <libaddressinput/address_formatter.h>

#include <libaddressinput/address_data.h>

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::AddressData;
using i18n::addressinput::GetFormattedNationalAddress;
using i18n::addressinput::GetFormattedNationalAddressLine;
using i18n::addressinput::GetStreetAddressLinesAsSingleLine;

TEST(AddressFormatterTest, GetStreetAddressLinesAsSingleLine_EmptyAddress) {
  AddressData address;
  std::string result;
  GetStreetAddressLinesAsSingleLine(address, &result);
  EXPECT_TRUE(result.empty());
}

TEST(AddressFormatterTest, GetStreetAddressLinesAsSingleLine_1Line) {
  AddressData address;
  address.region_code = "US";  // Not used.
  address.address_line.push_back("Line 1");

  std::string result;
  GetStreetAddressLinesAsSingleLine(address, &result);
  EXPECT_EQ("Line 1", result);

  // Setting the language_code, with one line, shouldn't affect anything.
  address.language_code = "en";
  GetStreetAddressLinesAsSingleLine(address, &result);
  EXPECT_EQ("Line 1", result);

  address.language_code = "zh-Hans";
  GetStreetAddressLinesAsSingleLine(address, &result);
  EXPECT_EQ("Line 1", result);
}

TEST(AddressFormatterTest, GetStreetAddressLinesAsSingleLine_2Lines) {
  AddressData address;
  address.region_code = "US";  // Not used.
  address.address_line.push_back("Line 1");
  address.address_line.push_back("Line 2");

  std::string result;
  GetStreetAddressLinesAsSingleLine(address, &result);
  // Default separator if no language_code specified: ", "
  EXPECT_EQ("Line 1, Line 2", result);

  address.language_code = "en";
  GetStreetAddressLinesAsSingleLine(address, &result);
  EXPECT_EQ("Line 1, Line 2", result);

  address.language_code = "zh-Hans";
  GetStreetAddressLinesAsSingleLine(address, &result);
  // Chinese has no separator.
  EXPECT_EQ("Line 1Line 2", result);

  address.language_code = "ko";
  GetStreetAddressLinesAsSingleLine(address, &result);
  EXPECT_EQ("Line 1 Line 2", result);

  address.language_code = "ar";
  GetStreetAddressLinesAsSingleLine(address, &result);
  EXPECT_EQ(u8"Line 1، Line 2", result);
}

TEST(AddressFormatterTest, GetStreetAddressLinesAsSingleLine_5Lines) {
  AddressData address;
  address.region_code = "US";  // Not used.
  address.address_line.push_back("Line 1");
  address.address_line.push_back("Line 2");
  address.address_line.push_back("Line 3");
  address.address_line.push_back("Line 4");
  address.address_line.push_back("Line 5");
  address.language_code = "fr";

  std::string result;
  GetStreetAddressLinesAsSingleLine(address, &result);
  EXPECT_EQ(result, "Line 1, Line 2, Line 3, Line 4, Line 5");
}

TEST(AddressFormatterTest, GetFormattedNationalAddressLocalLanguage) {
  AddressData address;
  address.region_code = "NZ";
  address.address_line.push_back("Rotopapa");
  address.address_line.push_back("Irwell 3RD");
  address.postal_code = "8704";
  address.locality = "Leeston";

  std::vector<std::string> expected;
  expected.push_back("Rotopapa");
  expected.push_back("Irwell 3RD");
  expected.push_back("Leeston 8704");

  std::vector<std::string> lines;
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  // Should be the same result no matter what the language_code is. We choose an
  // unlikely language_code code to illustrate this.
  address.language_code = "en-Latn-CN";

  lines.clear();
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  std::string one_line;
  GetFormattedNationalAddressLine(address, &one_line);
  EXPECT_EQ("Rotopapa, Irwell 3RD, Leeston 8704", one_line);
}

TEST(AddressFormatterTest, GetFormattedNationalAddressLatinFormat) {
  static const char kTaiwanCity[] = u8"大安區";
  static const char kTaiwanAdmin[] = u8"台北市";
  static const char kTaiwanStreetLine[] = u8"台灣信義路三段33號";
  static const char kPostalCode[] = "106";

  AddressData address;
  address.region_code = "TW";
  address.address_line.push_back(kTaiwanStreetLine);
  address.postal_code = kPostalCode;
  address.locality = kTaiwanCity;
  address.administrative_area = kTaiwanAdmin;
  address.language_code = "zh-Hant";

  std::vector<std::string> expected;
  expected.push_back(kPostalCode);
  expected.push_back(std::string(kTaiwanAdmin).append(kTaiwanCity));
  expected.push_back(kTaiwanStreetLine);

  std::vector<std::string> lines;
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  std::string one_line;
  GetFormattedNationalAddressLine(address, &one_line);
  // No separators expected for Chinese.
  EXPECT_EQ(std::string(kPostalCode).append(kTaiwanAdmin).append(kTaiwanCity)
            .append(kTaiwanStreetLine),
            one_line);

  // Changing to the Latin variant will change the output.
  AddressData latin_address;
  latin_address.region_code = "TW";
  latin_address.address_line.push_back("No. 33, Section 3 Xinyi Rd");
  latin_address.postal_code = kPostalCode;
  latin_address.locality = "Da-an District";
  latin_address.administrative_area = "Taipei City";
  latin_address.language_code = "zh-Latn";

  std::vector<std::string> expected_latin;
  expected_latin.push_back("No. 33, Section 3 Xinyi Rd");
  expected_latin.push_back("Da-an District, Taipei City 106");

  lines.clear();
  GetFormattedNationalAddress(latin_address, &lines);
  EXPECT_EQ(expected_latin, lines);

  GetFormattedNationalAddressLine(latin_address, &one_line);
  // We expect ", " as the new-line replacements for zh-Latn.
  EXPECT_EQ("No. 33, Section 3 Xinyi Rd, Da-an District, Taipei City 106",
            one_line);
}

TEST(AddressFormatterTest, GetFormattedNationalAddressMultilingualCountry) {
  AddressData address;
  address.region_code = "CA";
  address.address_line.push_back("5 Rue du Tresor");
  address.address_line.push_back("Apt. 4");
  address.administrative_area = "QC";
  address.postal_code = "G1R 123";
  address.locality = "Montmagny";
  address.language_code = "fr";

  std::vector<std::string> expected;
  expected.push_back("5 Rue du Tresor");
  expected.push_back("Apt. 4");
  expected.push_back("Montmagny QC G1R 123");

  std::vector<std::string> lines;
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);
}

TEST(AddressFormatterTest, GetFormattedNationalAddress_InlineStreetAddress) {
  AddressData address;
  address.region_code = "CI";
  address.address_line.push_back("32 Boulevard Carde");
  address.locality = "Abidjan";
  address.sorting_code = "64";

  std::vector<std::string> expected;
  expected.push_back("64 32 Boulevard Carde Abidjan 64");

  std::vector<std::string> lines;
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);
}

TEST(AddressFormatterTest,
     GetFormattedNationalAddressMissingFields_LiteralsAroundField) {
  AddressData address;
  address.region_code = "CH";
  std::vector<std::string> expected;
  std::vector<std::string> lines;
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.locality = "Zurich";
  expected.push_back("Zurich");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.postal_code = "8001";
  expected.back().assign("CH-8001 Zurich");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.locality.clear();
  expected.back().assign("CH-8001");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);
}

TEST(AddressFormatterTest,
     GetFormattedNationalAddressMissingFields_LiteralsBetweenFields) {
  AddressData address;
  address.region_code = "US";
  std::vector<std::string> expected;
  std::vector<std::string> lines;
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.administrative_area = "CA";
  expected.push_back("CA");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.locality = "Los Angeles";
  expected.back().assign("Los Angeles, CA");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.postal_code = "90291";
  expected.back().assign("Los Angeles, CA 90291");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.administrative_area.clear();
  expected.back().assign("Los Angeles 90291");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.locality.clear();
  address.administrative_area = "CA";
  expected.back().assign("CA 90291");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);
}

TEST(AddressFormatterTest,
     GetFormattedNationalAddressMissingFields_LiteralOnSeparateLine) {
  AddressData address;
  address.region_code = "AX";
  std::vector<std::string> expected;
  expected.push_back(u8"ÅLAND");
  std::vector<std::string> lines;
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.locality = "City";
  expected.insert(expected.begin(), "City");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.postal_code = "123";
  expected.front().assign("AX-123 City");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);
}

TEST(AddressFormatterTest,
     GetFormattedNationalAddressMissingFields_LiteralBeforeField) {
  AddressData address;
  address.region_code = "JP";
  address.language_code = "ja";
  std::vector<std::string> expected;
  std::vector<std::string> lines;
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.postal_code = "123";
  expected.push_back(u8"〒123");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.administrative_area = "Prefecture";
  expected.push_back("Prefecture");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.postal_code.clear();
  expected.erase(expected.begin());
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);
}

TEST(AddressFormatterTest,
     GetFormattedNationalAddressMissingFields_DuplicateField) {
  AddressData address;
  address.region_code = "CI";
  std::vector<std::string> expected;
  std::vector<std::string> lines;
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.sorting_code = "123";
  expected.push_back("123 123");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.address_line.push_back("456 Main St");
  expected.back().assign("123 456 Main St 123");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.locality = "Yamoussoukro";
  expected.back().assign("123 456 Main St Yamoussoukro 123");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.sorting_code.erase();
  expected.back().assign("456 Main St Yamoussoukro");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);

  address.address_line.clear();
  expected.back().assign("Yamoussoukro");
  GetFormattedNationalAddress(address, &lines);
  EXPECT_EQ(expected, lines);
}


}  // namespace
