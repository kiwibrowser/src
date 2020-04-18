// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_TESTS_TEST_FLASH_CLIPBOARD_H_
#define PPAPI_TESTS_TEST_FLASH_CLIPBOARD_H_

#include <stdint.h>

#include <string>

#include "ppapi/c/private/ppb_flash_clipboard.h"
#include "ppapi/tests/test_case.h"
#include "ppapi/tests/test_utils.h"

class TestFlashClipboard : public TestCase {
 public:
  explicit TestFlashClipboard(TestingInstance* instance);

  // TestCase implementation.
  virtual void RunTests(const std::string& filter);

 private:
  // Helpers.
  bool ReadStringVar(uint32_t format, std::string* result);
  bool WriteStringVar(uint32_t format, const std::string& text);
  bool IsFormatAvailableMatches(uint32_t format, bool expected);
  bool ReadPlainTextMatches(const std::string& expected);
  bool ReadHTMLMatches(const std::string& expected);
  uint64_t GetSequenceNumber(uint64_t last_sequence_number);

  // Tests.
  std::string TestReadWritePlainText();
  std::string TestReadWriteHTML();
  std::string TestReadWriteRTF();
  std::string TestReadWriteMultipleFormats();
  std::string TestReadWriteCustomData();
  std::string TestClear();
  std::string TestInvalidFormat();
  std::string TestRegisterCustomFormat();
  std::string TestGetSequenceNumber();
};

#endif  // PAPPI_TESTS_TEST_FLASH_FULLSCREEN_H_
