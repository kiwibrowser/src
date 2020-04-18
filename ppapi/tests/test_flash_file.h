// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_TESTS_TEST_FLASH_FILE_H_
#define PPAPI_TESTS_TEST_FLASH_FILE_H_

#include <stddef.h>

#include <string>

#include "ppapi/tests/test_case.h"

class TestFlashFile: public TestCase {
 public:
  explicit TestFlashFile(TestingInstance* instance);
  virtual ~TestFlashFile();

  // TestCase implementation.
  virtual bool Init();
  virtual void RunTests(const std::string& filter);

 private:
  // TODO(raymes): We should have SetUp/TearDown methods for ppapi tests.
  void SetUp();

  std::string TestOpenFile();
  std::string TestRenameFile();
  std::string TestDeleteFileOrDir();
  std::string TestCreateDir();
  std::string TestQueryFile();
  std::string TestGetDirContents();
  std::string TestCreateTemporaryFile();

  // TODO(raymes): Add these when we can test file chooser properly.
  // std::string TestOpenFileRef();
  // std::string TestQueryFileRef();

  // Gets the number of files and directories under the module-local root
  // directory.
  std::string GetItemCountUnderModuleLocalRoot(size_t* item_count);
};

#endif  // PPAPI_TESTS_TEST_FLASH_FILE_H_
