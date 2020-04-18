// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_flash_clipboard.h"

#include <algorithm>
#include <vector>

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/private/flash_clipboard.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/tests/testing_instance.h"

// http://crbug.com/176822
#if !defined(OS_WIN)
REGISTER_TEST_CASE(FlashClipboard);
#endif

// WriteData() sends an async request to the browser process. As a result, the
// string written may not be reflected by IsFormatAvailable() or ReadPlainText()
// immediately. We need to wait and retry.
const int kIntervalMs = 250;
const int kMaxIntervals = kActionTimeoutMs / kIntervalMs;

TestFlashClipboard::TestFlashClipboard(TestingInstance* instance)
    : TestCase(instance) {
}

void TestFlashClipboard::RunTests(const std::string& filter) {
  RUN_TEST(ReadWritePlainText, filter);
  RUN_TEST(ReadWriteHTML, filter);
  RUN_TEST(ReadWriteRTF, filter);
  RUN_TEST(ReadWriteCustomData, filter);
  RUN_TEST(ReadWriteMultipleFormats, filter);
  RUN_TEST(Clear, filter);
  RUN_TEST(InvalidFormat, filter);
  RUN_TEST(RegisterCustomFormat, filter);
  RUN_TEST(GetSequenceNumber, filter);
}

bool TestFlashClipboard::ReadStringVar(uint32_t format, std::string* result) {
  pp::Var text;
  bool success = pp::flash::Clipboard::ReadData(
      instance_,
      PP_FLASH_CLIPBOARD_TYPE_STANDARD,
      format,
      &text);
  if (success && text.is_string()) {
    *result = text.AsString();
    return true;
  }
  return false;
}

bool TestFlashClipboard::WriteStringVar(uint32_t format,
                                        const std::string& text) {
  std::vector<uint32_t> formats_vector(1, format);
  std::vector<pp::Var> data_vector(1, pp::Var(text));
  bool success = pp::flash::Clipboard::WriteData(
      instance_,
      PP_FLASH_CLIPBOARD_TYPE_STANDARD,
      formats_vector,
      data_vector);
  return success;
}

bool TestFlashClipboard::IsFormatAvailableMatches(uint32_t format,
                                                  bool expected) {
  for (int i = 0; i < kMaxIntervals; ++i) {
    bool is_available = pp::flash::Clipboard::IsFormatAvailable(
        instance_,
        PP_FLASH_CLIPBOARD_TYPE_STANDARD,
        format);
    if (is_available == expected)
      return true;

    PlatformSleep(kIntervalMs);
  }
  return false;
}

bool TestFlashClipboard::ReadPlainTextMatches(const std::string& expected) {
  for (int i = 0; i < kMaxIntervals; ++i) {
    std::string result;
    bool success = ReadStringVar(PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT, &result);
    if (success && result == expected)
      return true;

    PlatformSleep(kIntervalMs);
  }
  return false;
}

bool TestFlashClipboard::ReadHTMLMatches(const std::string& expected) {
  for (int i = 0; i < kMaxIntervals; ++i) {
    std::string result;
    bool success = ReadStringVar(PP_FLASH_CLIPBOARD_FORMAT_HTML, &result);
    // Harmless markup may be inserted around the copied html on some
    // platforms, so just check that the pasted string contains the
    // copied string. Also check that we only paste the copied fragment, see
    // http://code.google.com/p/chromium/issues/detail?id=130827.
    if (success && result.find(expected) != std::string::npos &&
        result.find("<!--StartFragment-->") == std::string::npos &&
        result.find("<!--EndFragment-->") == std::string::npos) {
      return true;
    }

    PlatformSleep(kIntervalMs);
  }
  return false;
}

uint64_t TestFlashClipboard::GetSequenceNumber(uint64_t last_sequence_number) {
  uint64_t next_sequence_number = last_sequence_number;
  for (int i = 0; i < kMaxIntervals; ++i) {
    pp::flash::Clipboard::GetSequenceNumber(
        instance_, PP_FLASH_CLIPBOARD_TYPE_STANDARD, &next_sequence_number);
    if (next_sequence_number != last_sequence_number)
      return next_sequence_number;

    PlatformSleep(kIntervalMs);
  }
  return next_sequence_number;
}

std::string TestFlashClipboard::TestReadWritePlainText() {
  std::string input = "Hello world plain text!";
  ASSERT_TRUE(WriteStringVar(PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT, input));
  ASSERT_TRUE(IsFormatAvailableMatches(PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT,
                                       true));
  ASSERT_TRUE(ReadPlainTextMatches(input));

  PASS();
}

std::string TestFlashClipboard::TestReadWriteHTML() {
  std::string input = "Hello world html!";
  ASSERT_TRUE(WriteStringVar(PP_FLASH_CLIPBOARD_FORMAT_HTML, input));
  ASSERT_TRUE(IsFormatAvailableMatches(PP_FLASH_CLIPBOARD_FORMAT_HTML, true));
  ASSERT_TRUE(ReadHTMLMatches(input));

  PASS();
}

std::string TestFlashClipboard::TestReadWriteRTF() {
  std::string rtf_string =
        "{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\n"
        "This is some {\\b bold} text.\\par\n"
        "}";
  pp::VarArrayBuffer array_buffer(static_cast<uint32_t>(rtf_string.size()));
  char* bytes = static_cast<char*>(array_buffer.Map());
  std::copy(rtf_string.data(), rtf_string.data() + rtf_string.size(), bytes);
  std::vector<uint32_t> formats_vector(1, PP_FLASH_CLIPBOARD_FORMAT_RTF);
  std::vector<pp::Var> data_vector(1, array_buffer);
  ASSERT_TRUE(pp::flash::Clipboard::WriteData(
      instance_,
      PP_FLASH_CLIPBOARD_TYPE_STANDARD,
      formats_vector,
      data_vector));

  ASSERT_TRUE(IsFormatAvailableMatches(PP_FLASH_CLIPBOARD_FORMAT_RTF, true));

  pp::Var rtf_result;
  ASSERT_TRUE(pp::flash::Clipboard::ReadData(
        instance_,
        PP_FLASH_CLIPBOARD_TYPE_STANDARD,
        PP_FLASH_CLIPBOARD_FORMAT_RTF,
        &rtf_result));
  ASSERT_TRUE(rtf_result.is_array_buffer());
  pp::VarArrayBuffer array_buffer_result(rtf_result);
  ASSERT_TRUE(array_buffer_result.ByteLength() == array_buffer.ByteLength());
  char* bytes_result = static_cast<char*>(array_buffer_result.Map());
  ASSERT_TRUE(std::equal(bytes, bytes + array_buffer.ByteLength(),
      bytes_result));

  PASS();
}

std::string TestFlashClipboard::TestReadWriteCustomData() {
  std::string custom_data = "custom_data";
  pp::VarArrayBuffer array_buffer(static_cast<uint32_t>(custom_data.size()));
  char* bytes = static_cast<char*>(array_buffer.Map());
  std::copy(custom_data.begin(), custom_data.end(), bytes);
  uint32_t format_id =
      pp::flash::Clipboard::RegisterCustomFormat(instance_, "my-format");
  ASSERT_NE(static_cast<uint32_t>(PP_FLASH_CLIPBOARD_FORMAT_INVALID),
            format_id);

  std::vector<uint32_t> formats_vector(1, format_id);
  std::vector<pp::Var> data_vector(1, array_buffer);
  ASSERT_TRUE(pp::flash::Clipboard::WriteData(
      instance_,
      PP_FLASH_CLIPBOARD_TYPE_STANDARD,
      formats_vector,
      data_vector));

  ASSERT_TRUE(IsFormatAvailableMatches(format_id, true));

  pp::Var custom_data_result;
  ASSERT_TRUE(pp::flash::Clipboard::ReadData(
      instance_,
      PP_FLASH_CLIPBOARD_TYPE_STANDARD,
      format_id,
      &custom_data_result));
  ASSERT_TRUE(custom_data_result.is_array_buffer());
  pp::VarArrayBuffer array_buffer_result(custom_data_result);
  ASSERT_EQ(array_buffer_result.ByteLength(), array_buffer.ByteLength());
  char* bytes_result = static_cast<char*>(array_buffer_result.Map());
  ASSERT_TRUE(std::equal(bytes, bytes + array_buffer.ByteLength(),
      bytes_result));

  PASS();
}

std::string TestFlashClipboard::TestReadWriteMultipleFormats() {
  std::vector<uint32_t> formats;
  std::vector<pp::Var> data;
  formats.push_back(PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT);
  data.push_back(pp::Var("plain text"));
  formats.push_back(PP_FLASH_CLIPBOARD_FORMAT_HTML);
  data.push_back(pp::Var("html"));
  bool success = pp::flash::Clipboard::WriteData(
      instance_,
      PP_FLASH_CLIPBOARD_TYPE_STANDARD,
      formats,
      data);
  ASSERT_TRUE(success);
  ASSERT_TRUE(IsFormatAvailableMatches(PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT,
                                       true));
  ASSERT_TRUE(IsFormatAvailableMatches(PP_FLASH_CLIPBOARD_FORMAT_HTML, true));
  ASSERT_TRUE(ReadPlainTextMatches(data[0].AsString()));
  ASSERT_TRUE(ReadHTMLMatches(data[1].AsString()));

  PASS();
}

std::string TestFlashClipboard::TestClear() {
  std::string input = "Hello world plain text!";
  ASSERT_TRUE(WriteStringVar(PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT, input));
  ASSERT_TRUE(IsFormatAvailableMatches(PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT,
                                       true));
  bool success = pp::flash::Clipboard::WriteData(
      instance_,
      PP_FLASH_CLIPBOARD_TYPE_STANDARD,
      std::vector<uint32_t>(),
      std::vector<pp::Var>());
  ASSERT_TRUE(success);
  ASSERT_TRUE(IsFormatAvailableMatches(PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT,
                                       false));

  PASS();
}

std::string TestFlashClipboard::TestInvalidFormat() {
  uint32_t invalid_format = 999;
  ASSERT_FALSE(WriteStringVar(invalid_format, "text"));
  ASSERT_TRUE(IsFormatAvailableMatches(invalid_format, false));
  std::string unused;
  ASSERT_FALSE(ReadStringVar(invalid_format, &unused));

  PASS();
}

std::string TestFlashClipboard::TestRegisterCustomFormat() {
  // Test an empty name is rejected.
  uint32_t format_id =
      pp::flash::Clipboard::RegisterCustomFormat(instance_, std::string());
  ASSERT_EQ(static_cast<uint32_t>(PP_FLASH_CLIPBOARD_FORMAT_INVALID),
            format_id);

  // Test a valid format name.
  format_id = pp::flash::Clipboard::RegisterCustomFormat(instance_, "a-b");
  ASSERT_NE(static_cast<uint32_t>(PP_FLASH_CLIPBOARD_FORMAT_INVALID),
            format_id);
  // Make sure the format doesn't collide with predefined formats.
  ASSERT_NE(static_cast<uint32_t>(PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT),
            format_id);
  ASSERT_NE(static_cast<uint32_t>(PP_FLASH_CLIPBOARD_FORMAT_HTML),
            format_id);
  ASSERT_NE(static_cast<uint32_t>(PP_FLASH_CLIPBOARD_FORMAT_RTF),
            format_id);

  // Check that if the same name is registered, the same id comes out.
  uint32_t format_id2 =
      pp::flash::Clipboard::RegisterCustomFormat(instance_, "a-b");
  ASSERT_EQ(format_id, format_id2);

  // Check that the second format registered has a different id.
  uint32_t format_id3 =
      pp::flash::Clipboard::RegisterCustomFormat(instance_, "a-b-c");
  ASSERT_NE(format_id, format_id3);

  PASS();
}

std::string TestFlashClipboard::TestGetSequenceNumber() {
  uint64_t sequence_number_before = 0;
  uint64_t sequence_number_after = 0;
  ASSERT_TRUE(pp::flash::Clipboard::GetSequenceNumber(
      instance_, PP_FLASH_CLIPBOARD_TYPE_STANDARD, &sequence_number_before));

  // Test the sequence number changes after writing html.
  ASSERT_TRUE(WriteStringVar(PP_FLASH_CLIPBOARD_FORMAT_HTML, "<html>"));
  sequence_number_after = GetSequenceNumber(sequence_number_before);
  ASSERT_NE(sequence_number_before, sequence_number_after);
  sequence_number_before = sequence_number_after;

  // Test the sequence number changes after writing some custom data.
  std::string custom_data = "custom_data";
  pp::VarArrayBuffer array_buffer(static_cast<uint32_t>(custom_data.size()));
  char* bytes = static_cast<char*>(array_buffer.Map());
  std::copy(custom_data.begin(), custom_data.end(), bytes);
  uint32_t format_id =
      pp::flash::Clipboard::RegisterCustomFormat(instance_, "my-format");
  std::vector<uint32_t> formats_vector(1, format_id);
  std::vector<pp::Var> data_vector(1, array_buffer);
  ASSERT_TRUE(pp::flash::Clipboard::WriteData(instance_,
                                              PP_FLASH_CLIPBOARD_TYPE_STANDARD,
                                              formats_vector,
                                              data_vector));
  sequence_number_after = GetSequenceNumber(sequence_number_before);
  ASSERT_NE(sequence_number_before, sequence_number_after);
  sequence_number_before = sequence_number_after;

  // Read the data and make sure the sequence number doesn't change.
  pp::Var custom_data_result;
  ASSERT_TRUE(pp::flash::Clipboard::ReadData(
      instance_,
      PP_FLASH_CLIPBOARD_TYPE_STANDARD,
      format_id,
      &custom_data_result));
  ASSERT_TRUE(pp::flash::Clipboard::GetSequenceNumber(
      instance_, PP_FLASH_CLIPBOARD_TYPE_STANDARD, &sequence_number_after));
  ASSERT_EQ(sequence_number_before, sequence_number_after);
  sequence_number_before = sequence_number_after;

  // Clear the clipboard and check the sequence number changes.
  pp::flash::Clipboard::WriteData(instance_,
                                  PP_FLASH_CLIPBOARD_TYPE_STANDARD,
                                  std::vector<uint32_t>(),
                                  std::vector<pp::Var>());
  sequence_number_after = GetSequenceNumber(sequence_number_before);
  ASSERT_NE(sequence_number_before, sequence_number_after);

  PASS();
}
