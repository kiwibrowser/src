// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/contact_info.h"

#include <stddef.h>

#include "base/format_macros.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/field_types.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;
using base::UTF8ToUTF16;

namespace autofill {

struct FullNameTestCase {
  std::string full_name_input;
  std::string given_name_output;
  std::string middle_name_output;
  std::string family_name_output;
};

class SetFullNameTest : public testing::TestWithParam<FullNameTestCase> {};

TEST_P(SetFullNameTest, SetFullName) {
  auto test_case = GetParam();
  SCOPED_TRACE(test_case.full_name_input);

  NameInfo name;
  name.SetInfo(AutofillType(NAME_FULL), ASCIIToUTF16(test_case.full_name_input),
               "en-US");
  EXPECT_EQ(ASCIIToUTF16(test_case.given_name_output),
            name.GetInfo(AutofillType(NAME_FIRST), "en-US"));
  EXPECT_EQ(ASCIIToUTF16(test_case.middle_name_output),
            name.GetInfo(AutofillType(NAME_MIDDLE), "en-US"));
  EXPECT_EQ(ASCIIToUTF16(test_case.family_name_output),
            name.GetInfo(AutofillType(NAME_LAST), "en-US"));
  EXPECT_EQ(ASCIIToUTF16(test_case.full_name_input),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));
}

INSTANTIATE_TEST_CASE_P(
    ContactInfoTest,
    SetFullNameTest,
    testing::Values(
        FullNameTestCase{"", "", "", ""},
        FullNameTestCase{"John Smith", "John", "", "Smith"},
        FullNameTestCase{"Julien van der Poel", "Julien", "", "van der Poel"},
        FullNameTestCase{"John J Johnson", "John", "J", "Johnson"},
        FullNameTestCase{"John Smith, Jr.", "John", "", "Smith"},
        FullNameTestCase{"Mr John Smith", "John", "", "Smith"},
        FullNameTestCase{"Mr. John Smith", "John", "", "Smith"},
        FullNameTestCase{"Mr. John Smith, M.D.", "John", "", "Smith"},
        FullNameTestCase{"Mr. John Smith, MD", "John", "", "Smith"},
        FullNameTestCase{"Mr. John Smith MD", "John", "", "Smith"},
        FullNameTestCase{"William Hubert J.R.", "William", "Hubert", "J.R."},
        FullNameTestCase{"John Ma", "John", "", "Ma"},
        FullNameTestCase{"John Smith, MA", "John", "", "Smith"},
        FullNameTestCase{"John Jacob Jingleheimer Smith", "John Jacob",
                         "Jingleheimer", "Smith"},
        FullNameTestCase{"Virgil", "Virgil", "", ""},
        FullNameTestCase{"Murray Gell-Mann", "Murray", "", "Gell-Mann"},
        FullNameTestCase{"Mikhail Yevgrafovich Saltykov-Shchedrin", "Mikhail",
                         "Yevgrafovich", "Saltykov-Shchedrin"},
        FullNameTestCase{"Arthur Ignatius Conan Doyle", "Arthur Ignatius",
                         "Conan", "Doyle"}));

TEST(NameInfoTest, GetFullName) {
  NameInfo name;
  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("First"));
  name.SetRawInfo(NAME_MIDDLE, base::string16());
  name.SetRawInfo(NAME_LAST, base::string16());
  EXPECT_EQ(ASCIIToUTF16("First"), name.GetRawInfo(NAME_FIRST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_MIDDLE));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_LAST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FULL));
  EXPECT_EQ(ASCIIToUTF16("First"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  name.SetRawInfo(NAME_FIRST, base::string16());
  name.SetRawInfo(NAME_MIDDLE, ASCIIToUTF16("Middle"));
  name.SetRawInfo(NAME_LAST, base::string16());
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FIRST));
  EXPECT_EQ(ASCIIToUTF16("Middle"), name.GetRawInfo(NAME_MIDDLE));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_LAST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FULL));
  EXPECT_EQ(ASCIIToUTF16("Middle"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  name.SetRawInfo(NAME_FIRST, base::string16());
  name.SetRawInfo(NAME_MIDDLE, base::string16());
  name.SetRawInfo(NAME_LAST, ASCIIToUTF16("Last"));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FIRST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_MIDDLE));
  EXPECT_EQ(ASCIIToUTF16("Last"), name.GetRawInfo(NAME_LAST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FULL));
  EXPECT_EQ(ASCIIToUTF16("Last"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("First"));
  name.SetRawInfo(NAME_MIDDLE, ASCIIToUTF16("Middle"));
  name.SetRawInfo(NAME_LAST, base::string16());
  EXPECT_EQ(ASCIIToUTF16("First"), name.GetRawInfo(NAME_FIRST));
  EXPECT_EQ(ASCIIToUTF16("Middle"), name.GetRawInfo(NAME_MIDDLE));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_LAST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FULL));
  EXPECT_EQ(ASCIIToUTF16("First Middle"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("First"));
  name.SetRawInfo(NAME_MIDDLE, base::string16());
  name.SetRawInfo(NAME_LAST, ASCIIToUTF16("Last"));
  EXPECT_EQ(ASCIIToUTF16("First"), name.GetRawInfo(NAME_FIRST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_MIDDLE));
  EXPECT_EQ(ASCIIToUTF16("Last"), name.GetRawInfo(NAME_LAST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FULL));
  EXPECT_EQ(ASCIIToUTF16("First Last"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  name.SetRawInfo(NAME_FIRST, base::string16());
  name.SetRawInfo(NAME_MIDDLE, ASCIIToUTF16("Middle"));
  name.SetRawInfo(NAME_LAST, ASCIIToUTF16("Last"));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FIRST));
  EXPECT_EQ(ASCIIToUTF16("Middle"), name.GetRawInfo(NAME_MIDDLE));
  EXPECT_EQ(ASCIIToUTF16("Last"), name.GetRawInfo(NAME_LAST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FULL));
  EXPECT_EQ(ASCIIToUTF16("Middle Last"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("First"));
  name.SetRawInfo(NAME_MIDDLE, ASCIIToUTF16("Middle"));
  name.SetRawInfo(NAME_LAST, ASCIIToUTF16("Last"));
  EXPECT_EQ(ASCIIToUTF16("First"), name.GetRawInfo(NAME_FIRST));
  EXPECT_EQ(ASCIIToUTF16("Middle"), name.GetRawInfo(NAME_MIDDLE));
  EXPECT_EQ(ASCIIToUTF16("Last"), name.GetRawInfo(NAME_LAST));
  EXPECT_EQ(base::string16(), name.GetRawInfo(NAME_FULL));
  EXPECT_EQ(ASCIIToUTF16("First Middle Last"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  name.SetRawInfo(NAME_FULL, ASCIIToUTF16("First Middle Last, MD"));
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("First"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("First Middle Last, MD"));
  EXPECT_EQ(ASCIIToUTF16("First Middle Last, MD"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  // Setting a name to the value it already has: no change.
  name.SetInfo(AutofillType(NAME_FIRST), ASCIIToUTF16("First"), "en-US");
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("First"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("First Middle Last, MD"));
  EXPECT_EQ(ASCIIToUTF16("First Middle Last, MD"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  // Setting raw info: no change. (Even though this leads to a slightly
  // inconsitent state.)
  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("Second"));
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("Second"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("First Middle Last, MD"));
  EXPECT_EQ(ASCIIToUTF16("First Middle Last, MD"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));

  // Changing something (e.g., the first name) clears the stored full name.
  name.SetInfo(AutofillType(NAME_FIRST), ASCIIToUTF16("Third"), "en-US");
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("Third"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(ASCIIToUTF16("Third Middle Last"),
            name.GetInfo(AutofillType(NAME_FULL), "en-US"));
}

struct ParsedNamesAreEqualTestCase {
  std::string starting_names[3];
  std::string additional_names[3];
  bool expected_result;
  };

  class ParsedNamesAreEqualTest
      : public testing::TestWithParam<ParsedNamesAreEqualTestCase> {};

  TEST_P(ParsedNamesAreEqualTest, ParsedNamesAreEqual) {
    auto test_case = GetParam();

    // Construct the starting_profile.
    NameInfo starting_profile;
    starting_profile.SetRawInfo(NAME_FIRST,
                                UTF8ToUTF16(test_case.starting_names[0]));
    starting_profile.SetRawInfo(NAME_MIDDLE,
                                UTF8ToUTF16(test_case.starting_names[1]));
    starting_profile.SetRawInfo(NAME_LAST,
                                UTF8ToUTF16(test_case.starting_names[2]));

    // Construct the additional_profile.
    NameInfo additional_profile;
    additional_profile.SetRawInfo(NAME_FIRST,
                                  UTF8ToUTF16(test_case.additional_names[0]));
    additional_profile.SetRawInfo(NAME_MIDDLE,
                                  UTF8ToUTF16(test_case.additional_names[1]));
    additional_profile.SetRawInfo(NAME_LAST,
                                  UTF8ToUTF16(test_case.additional_names[2]));

    // Verify the test expectations.
    EXPECT_EQ(test_case.expected_result,
              starting_profile.ParsedNamesAreEqual(additional_profile));
  }

  INSTANTIATE_TEST_CASE_P(
      ContactInfoTest,
      ParsedNamesAreEqualTest,
      testing::Values(
          // Identical name comparison.
          ParsedNamesAreEqualTestCase{{"Marion", "Mitchell", "Morrison"},
                                      {"Marion", "Mitchell", "Morrison"},
                                      true},

          // Case-sensitive comparisons.
          ParsedNamesAreEqualTestCase{{"Marion", "Mitchell", "Morrison"},
                                      {"Marion", "Mitchell", "MORRISON"},
                                      false},
          ParsedNamesAreEqualTestCase{{"Marion", "Mitchell", "Morrison"},
                                      {"MARION", "Mitchell", "MORRISON"},
                                      false},
          ParsedNamesAreEqualTestCase{{"Marion", "Mitchell", "Morrison"},
                                      {"MARION", "MITCHELL", "MORRISON"},
                                      false},
          ParsedNamesAreEqualTestCase{{"Marion", "", "Mitchell Morrison"},
                                      {"MARION", "", "MITCHELL MORRISON"},
                                      false},
          ParsedNamesAreEqualTestCase{{"Marion Mitchell", "", "Morrison"},
                                      {"MARION MITCHELL", "", "MORRISON"},
                                      false},

          // Identical full names but different canonical forms.
          ParsedNamesAreEqualTestCase{{"Marion", "Mitchell", "Morrison"},
                                      {"Marion", "", "Mitchell Morrison"},
                                      false},
          ParsedNamesAreEqualTestCase{{"Marion", "Mitchell", "Morrison"},
                                      {"Marion Mitchell", "", "MORRISON"},
                                      false},

          // Different names.
          ParsedNamesAreEqualTestCase{{"Marion", "Mitchell", "Morrison"},
                                      {"Marion", "M.", "Morrison"},
                                      false},
          ParsedNamesAreEqualTestCase{{"Marion", "Mitchell", "Morrison"},
                                      {"MARION", "M.", "MORRISON"},
                                      false},
          ParsedNamesAreEqualTestCase{{"Marion", "Mitchell", "Morrison"},
                                      {"David", "Mitchell", "Morrison"},
                                      false},

          // Non-ASCII characters.
          ParsedNamesAreEqualTestCase{
              {"M\xc3\xa1rion Mitchell", "", "Morrison"},
              {"M\xc3\xa1rion Mitchell", "", "Morrison"},
              true}));

  struct OverwriteNameTestCase {
    std::string existing_name[4];
    std::string new_name[4];
    std::string expected_name[4];
  };

  class OverwriteNameTest
      : public testing::TestWithParam<OverwriteNameTestCase> {};

  TEST_P(OverwriteNameTest, OverwriteName) {
    auto test_case = GetParam();
    // Construct the starting_profile.
    NameInfo existing_name;
    existing_name.SetRawInfo(NAME_FIRST,
                             UTF8ToUTF16(test_case.existing_name[0]));
    existing_name.SetRawInfo(NAME_MIDDLE,
                             UTF8ToUTF16(test_case.existing_name[1]));
    existing_name.SetRawInfo(NAME_LAST,
                             UTF8ToUTF16(test_case.existing_name[2]));
    existing_name.SetRawInfo(NAME_FULL,
                             UTF8ToUTF16(test_case.existing_name[3]));

    // Construct the additional_profile.
    NameInfo new_name;
    new_name.SetRawInfo(NAME_FIRST, UTF8ToUTF16(test_case.new_name[0]));
    new_name.SetRawInfo(NAME_MIDDLE, UTF8ToUTF16(test_case.new_name[1]));
    new_name.SetRawInfo(NAME_LAST, UTF8ToUTF16(test_case.new_name[2]));
    new_name.SetRawInfo(NAME_FULL, UTF8ToUTF16(test_case.new_name[3]));

    existing_name.OverwriteName(new_name);

    // Verify the test expectations.
    EXPECT_EQ(UTF8ToUTF16(test_case.expected_name[0]),
              existing_name.GetRawInfo(NAME_FIRST));
    EXPECT_EQ(UTF8ToUTF16(test_case.expected_name[1]),
              existing_name.GetRawInfo(NAME_MIDDLE));
    EXPECT_EQ(UTF8ToUTF16(test_case.expected_name[2]),
              existing_name.GetRawInfo(NAME_LAST));
    EXPECT_EQ(UTF8ToUTF16(test_case.expected_name[3]),
              existing_name.GetRawInfo(NAME_FULL));
}

INSTANTIATE_TEST_CASE_P(
    ContactInfoTest,
    OverwriteNameTest,
    testing::Values(
        // Missing information in the original name gets filled with the new
        // name's information.
        OverwriteNameTestCase{
            {"", "", "", ""},
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
        },
        // The new name's values overwrite the exsiting name values if they are
        // different
        OverwriteNameTestCase{
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
            {"Mario", "Mitchell", "Thompson", "Mario Mitchell Morrison"},
            {"Mario", "Mitchell", "Thompson", "Mario Mitchell Morrison"},
        },
        // An existing name values do not get replaced with empty values.
        OverwriteNameTestCase{
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
            {"", "", "", ""},
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
        },
        // An existing full middle not does not get replaced by a middle name
        // initial.
        OverwriteNameTestCase{
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
            {"Marion", "M", "Morrison", "Marion Mitchell Morrison"},
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
        },
        // An existing middle name initial is overwritten by the new profile's
        // middle name value.
        OverwriteNameTestCase{
            {"Marion", "M", "Morrison", "Marion Mitchell Morrison"},
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
        },
        // A NameInfo with only the full name set overwritten with a NameInfo
        // with only the name parts set result in a NameInfo with all the name
        // parts and name full set.
        OverwriteNameTestCase{
            {"", "", "", "Marion Mitchell Morrison"},
            {"Marion", "Mitchell", "Morrison", ""},
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
        },
        // A NameInfo with only the name parts set overwritten with a NameInfo
        // with only the full name set result in a NameInfo with all the name
        // parts and name full set.
        OverwriteNameTestCase{
            {"Marion", "Mitchell", "Morrison", ""},
            {"", "", "", "Marion Mitchell Morrison"},
            {"Marion", "Mitchell", "Morrison", "Marion Mitchell Morrison"},
        }));

struct NamePartsAreEmptyTestCase {
  std::string first;
  std::string middle;
  std::string last;
  std::string full;
  bool expected_result;
  };

  class NamePartsAreEmptyTest
      : public testing::TestWithParam<NamePartsAreEmptyTestCase> {};

  TEST_P(NamePartsAreEmptyTest, NamePartsAreEmpty) {
    auto test_case = GetParam();
    // Construct the NameInfo.
    NameInfo name;
    name.SetRawInfo(NAME_FIRST, UTF8ToUTF16(test_case.first));
    name.SetRawInfo(NAME_MIDDLE, UTF8ToUTF16(test_case.middle));
    name.SetRawInfo(NAME_LAST, UTF8ToUTF16(test_case.last));
    name.SetRawInfo(NAME_FULL, UTF8ToUTF16(test_case.full));

    // Verify the test expectations.
    EXPECT_EQ(test_case.expected_result, name.NamePartsAreEmpty());
}

INSTANTIATE_TEST_CASE_P(
    ContactInfoTest,
    NamePartsAreEmptyTest,
    testing::Values(NamePartsAreEmptyTestCase{"", "", "", "", true},
                    NamePartsAreEmptyTestCase{"", "", "",
                                              "Marion Mitchell Morrison", true},
                    NamePartsAreEmptyTestCase{"Marion", "", "", "", false},
                    NamePartsAreEmptyTestCase{"", "Mitchell", "", "", false},
                    NamePartsAreEmptyTestCase{"", "", "Morrison", "", false}));

}  // namespace autofill
