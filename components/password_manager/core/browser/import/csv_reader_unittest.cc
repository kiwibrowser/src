// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/import/csv_reader.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_manager {

TEST(CSVReaderTest, EmptyCSV) {
  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  EXPECT_TRUE(ReadCSV(std::string(), &column_names, &records));
  EXPECT_THAT(column_names, testing::ElementsAre(""));
  EXPECT_EQ(0u, records.size());
}

TEST(CSVReaderTest, OutputArgumentsAreOverwritten) {
  std::vector<std::string> column_names(3);
  std::vector<std::map<std::string, std::string>> records(4);
  EXPECT_TRUE(ReadCSV(std::string(), &column_names, &records));
  EXPECT_THAT(column_names, testing::ElementsAre(""));
  EXPECT_EQ(0u, records.size());
}

TEST(CSVReaderTest, CSVConsistingOfSingleNewLine) {
  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  EXPECT_TRUE(ReadCSV(std::string("\n"), &column_names, &records));
  EXPECT_THAT(column_names, testing::ElementsAre(""));
  EXPECT_EQ(0u, records.size());
}

TEST(CSVReaderTest, SingleColumn) {
  const char kTestCSVInput[] =
      "foo\n"
      "alpha\n"
      "beta\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  EXPECT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("foo"));
  ASSERT_EQ(2u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("foo", "alpha")));
  EXPECT_THAT(records[1], testing::ElementsAre(
      std::make_pair("foo", "beta")));
}

TEST(CSVReaderTest, HeaderOnly) {
  const char kTestCSVInput[] =
      "foo,bar\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("foo", "bar"));
  EXPECT_EQ(0u, records.size());
}

TEST(CSVReaderTest, HeaderAndSimpleRecords) {
  const char kTestCSVInput[] =
      "foo,bar,baz\n"
      "alpha,beta,gamma\n"
      "delta,epsilon,zeta\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("foo", "bar", "baz"));
  ASSERT_EQ(2u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("bar", "beta"),
      std::make_pair("baz", "gamma"),
      std::make_pair("foo", "alpha")));
  EXPECT_THAT(records[1], testing::ElementsAre(
      std::make_pair("bar", "epsilon"),
      std::make_pair("baz", "zeta"),
      std::make_pair("foo", "delta")));
}

TEST(CSVReaderTest, EmptyStringColumnNamesAreSupported) {
  const char kTestCSVInput[] =
      "foo,,bar\n"
      "alpha,beta,gamma\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("foo", "", "bar"));
  ASSERT_EQ(1u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("", "beta"),
      std::make_pair("bar", "gamma"),
      std::make_pair("foo", "alpha")));
}

TEST(CSVReaderTest, ExtraSpacesArePreserved) {
  const char kTestCSVInput[] =
      "left,right\n"
      " alpha  beta ,  \n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("left", "right"));
  ASSERT_EQ(1u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("left", " alpha  beta "),
      std::make_pair("right", "  ")));
}

TEST(CSVReaderTest, CharactersOutsideASCIIPrintableArePreservedVerbatim) {
  const char kTestCSVInput[] =
      "left,right\n"
      "\x07\t\x0B\x1F,$\xc2\xa2\xe2\x98\x83\xf0\xa4\xad\xa2\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("left", "right"));
  ASSERT_EQ(1u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      // Characters below 0x20: bell, horizontal + vertical tab, unit separator.
      std::make_pair("left", "\x07\t\x0B\x1F"),
      // Unicode code points having 1..4 byte UTF-8 representation: dollar sign
      // (U+0024), cent sign (U+00A2), snowman (U+2603), Han character U+24B62.
      std::make_pair("right", "$\xc2\xa2\xe2\x98\x83\xf0\xa4\xad\xa2")));
}

TEST(CSVReaderTest, EnclosingDoubleQuotesAreTrimmed) {
  const char kTestCSVInput[] =
      "\"left\",\"right\"\n"
      "\"alpha\",\"beta\"\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("left", "right"));
  ASSERT_EQ(1u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("left", "alpha"),
      std::make_pair("right", "beta")));
}

TEST(CSVReaderTest, SeparatorsInsideDoubleQuotesAreTreatedVerbatim) {
  const char kTestCSVInput[] =
      "left,right\n"
      "\"A\rB\",\"B\nC\"\n"
      "\"C\r\nD\",\"D\n\"\n"
      "\",\",\",,\"\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("left", "right"));
  ASSERT_EQ(3u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("left", "A\rB"),
      std::make_pair("right", "B\nC")));
  EXPECT_THAT(records[1], testing::ElementsAre(
      std::make_pair("left", "C\nD"),
      std::make_pair("right", "D\n")));
  EXPECT_THAT(records[2], testing::ElementsAre(
      std::make_pair("left", ","),
      std::make_pair("right", ",,")));
}

TEST(CSVReaderTest, EscapedDoubleQuotes) {
  const char kTestCSVInput[] =
      "left,right\n"
      "\"\",\"\"\"\"\"\"\n"
      "\"\"\"\",\"A\"\"B\"\"\"\"C\"\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("left", "right"));
  ASSERT_EQ(2u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("left", ""),
      std::make_pair("right", "\"\"")));
  EXPECT_THAT(records[1], testing::ElementsAre(
      std::make_pair("left", "\""),
      std::make_pair("right", "A\"B\"\"C")));
}

TEST(CSVReaderTest, InconsistentFieldsCountIsTreatedGracefully) {
  const char kTestCSVInput[] =
      "left,right\n"
      "A\n"
      "B,C,D\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("left", "right"));
  ASSERT_EQ(2u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("left", "A")));
  EXPECT_THAT(records[1], testing::ElementsAre(
      std::make_pair("left", "B"),
      std::make_pair("right", "C")));
}

TEST(CSVReaderTest, SupportMissingNewLineAtEOF) {
  const char kTestCSVInput[] =
      "left,right\n"
      "alpha,beta";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("left", "right"));
  ASSERT_EQ(1u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("left", "alpha"),
      std::make_pair("right", "beta")));
}

TEST(CSVReaderTest, EmptyFields) {
  const char kTestCSVInput[] =
      "left,middle,right\n"
      "alpha,beta,\n"
      ",,gamma\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("left", "middle", "right"));
  ASSERT_EQ(2u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("left", "alpha"),
      std::make_pair("middle", "beta"),
      std::make_pair("right", "")));
  EXPECT_THAT(records[1], testing::ElementsAre(
      std::make_pair("left", ""),
      std::make_pair("middle", ""),
      std::make_pair("right", "gamma")));
}

TEST(CSVReaderTest, CRLFTreatedAsAndConvertedToLF) {
  const char kTestCSVInput[] =
      "left,right\r\n"
      "\"\r\",\"\r\n\"\r\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("left", "right"));
  ASSERT_EQ(1u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("left", "\r"),
      std::make_pair("right", "\n")));
}

TEST(CSVReaderTest, LastValueForRepeatedColumnNamesIsPreserved) {
  const char kTestCSVInput[] =
      "foo,bar,bar\n"
      "alpha,beta,gamma\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_TRUE(ReadCSV(kTestCSVInput, &column_names, &records));

  EXPECT_THAT(column_names, testing::ElementsAre("foo", "bar", "bar"));
  ASSERT_EQ(1u, records.size());
  EXPECT_THAT(records[0], testing::ElementsAre(
      std::make_pair("bar", "gamma"),
      std::make_pair("foo", "alpha")));
}

TEST(CSVReaderTest, FailureWhenEOFInsideQuotes) {
  const char kTestCSVInput[] =
      "left,right\n"
      "\"alpha\",\"unmatched\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_FALSE(ReadCSV(kTestCSVInput, &column_names, &records));
}

TEST(CSVReaderTest, FailureWhenSemiQuotedContentInHeader) {
  const char kTestCSVInput[] =
      "\"a\"b\"c\",right\n"
      "alpha,beta\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_FALSE(ReadCSV(kTestCSVInput, &column_names, &records));
}

TEST(CSVReaderTest, FailureWhenSemiQuotedContentOnSubsequentLine) {
  const char kTestCSVInput[] =
      "alpha,beta\n"
      "left,\"a\"b\"c\"\n";

  std::vector<std::string> column_names;
  std::vector<std::map<std::string, std::string>> records;
  ASSERT_FALSE(ReadCSV(kTestCSVInput, &column_names, &records));
}

}  // namespace password_manager
