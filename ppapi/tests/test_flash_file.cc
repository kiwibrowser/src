// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_flash_file.h"

#include <algorithm>
#include <vector>

#include "ppapi/c/pp_file_info.h"
#include "ppapi/c/ppb_file_io.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/private/flash_file.h"
#include "ppapi/tests/testing_instance.h"
#include "ppapi/tests/test_utils.h"

#if defined(PPAPI_OS_WIN)
#include <windows.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

using pp::flash::FileModuleLocal;

namespace {

void CloseFileHandle(PP_FileHandle file_handle) {
#if defined(PPAPI_OS_WIN)
  CloseHandle(file_handle);
#else
  close(file_handle);
#endif
}

bool WriteFile(PP_FileHandle file_handle, const std::string& contents) {
#if defined(PPAPI_OS_WIN)
  DWORD bytes_written = 0;
  BOOL result = ::WriteFile(file_handle, contents.c_str(),
                            static_cast<DWORD>(contents.size()),
                            &bytes_written, NULL);
  return result && bytes_written == static_cast<DWORD>(contents.size());
#else
  ssize_t bytes_written = 0;
  do {
    bytes_written = write(file_handle, contents.c_str(), contents.size());
  } while (bytes_written == -1 && errno == EINTR);
  return bytes_written == static_cast<ssize_t>(contents.size());
#endif
}

bool ReadFile(PP_FileHandle file_handle, std::string* contents) {
  static const size_t kBufferSize = 1024;
  char* buffer = new char[kBufferSize];
  bool result = false;
  contents->clear();

#if defined(PPAPI_OS_WIN)
  SetFilePointer(file_handle, 0, NULL, FILE_BEGIN);
  DWORD bytes_read = 0;
  do {
    result = !!::ReadFile(file_handle, buffer, kBufferSize, &bytes_read, NULL);
    if (result && bytes_read > 0)
      contents->append(buffer, bytes_read);
  } while (result && bytes_read > 0);
#else
  lseek(file_handle, 0, SEEK_SET);
  ssize_t bytes_read = 0;
  do {
    do {
      bytes_read = read(file_handle, buffer, kBufferSize);
    } while (bytes_read == -1 && errno == EINTR);
    result = bytes_read != -1;
    if (bytes_read > 0)
      contents->append(buffer, bytes_read);
  } while (bytes_read > 0);
#endif

  delete[] buffer;
  return result;
}

bool DirEntryEqual(FileModuleLocal::DirEntry i,
                   FileModuleLocal::DirEntry j) {
  return i.name == j.name && i.is_dir == j.is_dir;
}

bool DirEntryLessThan(FileModuleLocal::DirEntry i,
                      FileModuleLocal::DirEntry j) {
  if (i.name == j.name)
    return i.is_dir < j.is_dir;
  return i.name < j.name;
}

}  // namespace

REGISTER_TEST_CASE(FlashFile);

TestFlashFile::TestFlashFile(TestingInstance* instance)
    : TestCase(instance) {
}

TestFlashFile::~TestFlashFile() {
}

bool TestFlashFile::Init() {
  return FileModuleLocal::IsAvailable();
}

void TestFlashFile::RunTests(const std::string& filter) {
  RUN_TEST(OpenFile, filter);
  RUN_TEST(RenameFile, filter);
  RUN_TEST(DeleteFileOrDir, filter);
  RUN_TEST(CreateDir, filter);
  RUN_TEST(QueryFile, filter);
  RUN_TEST(GetDirContents, filter);
  RUN_TEST(CreateTemporaryFile, filter);
}

void TestFlashFile::SetUp() {
  // Clear out existing test data.
  FileModuleLocal::DeleteFileOrDir(instance_, std::string(), true);
  // Make sure that the root directory exists.
  FileModuleLocal::CreateDir(instance_, std::string());
}

std::string TestFlashFile::TestOpenFile() {
  SetUp();
  std::string filename = "abc.txt";
  PP_FileHandle file_handle = FileModuleLocal::OpenFile(instance_,
                                                        filename,
                                                        PP_FILEOPENFLAG_WRITE |
                                                        PP_FILEOPENFLAG_CREATE);
  ASSERT_NE(PP_kInvalidFileHandle, file_handle);

  std::string contents = "This is file.";
  std::string read_contents;
  ASSERT_TRUE(WriteFile(file_handle, contents));
  ASSERT_FALSE(ReadFile(file_handle, &read_contents));
  CloseFileHandle(file_handle);

  file_handle = FileModuleLocal::OpenFile(instance_,
                                          filename,
                                          PP_FILEOPENFLAG_READ);
  ASSERT_NE(PP_kInvalidFileHandle, file_handle);

  ASSERT_FALSE(WriteFile(file_handle, contents));
  ASSERT_TRUE(ReadFile(file_handle, &read_contents));
  ASSERT_EQ(contents, read_contents);
  CloseFileHandle(file_handle);

  PASS();
}

std::string TestFlashFile::TestRenameFile() {
  SetUp();
  std::string filename = "abc.txt";
  std::string new_filename = "abc_new.txt";
  std::string contents = "This is file.";
  std::string read_contents;

  PP_FileHandle file_handle = FileModuleLocal::OpenFile(instance_,
                                                        filename,
                                                        PP_FILEOPENFLAG_WRITE |
                                                        PP_FILEOPENFLAG_CREATE);
  ASSERT_NE(PP_kInvalidFileHandle, file_handle);
  ASSERT_TRUE(WriteFile(file_handle, contents));
  CloseFileHandle(file_handle);

  ASSERT_TRUE(FileModuleLocal::RenameFile(instance_, filename, new_filename));

  file_handle = FileModuleLocal::OpenFile(instance_,
                                          new_filename,
                                          PP_FILEOPENFLAG_READ);
  ASSERT_NE(PP_kInvalidFileHandle, file_handle);
  ASSERT_TRUE(ReadFile(file_handle, &read_contents));
  ASSERT_EQ(contents, read_contents);
  CloseFileHandle(file_handle);

  // Check that the old file no longer exists.
  PP_FileInfo unused;
  ASSERT_FALSE(FileModuleLocal::QueryFile(instance_, filename, &unused));

  PASS();
}

std::string TestFlashFile::TestDeleteFileOrDir() {
  SetUp();
  std::string filename = "abc.txt";
  std::string dirname = "def";
  std::string contents = "This is file.";

  // Test file deletion.
  PP_FileHandle file_handle = FileModuleLocal::OpenFile(instance_,
                                                        filename,
                                                        PP_FILEOPENFLAG_WRITE |
                                                        PP_FILEOPENFLAG_CREATE);
  ASSERT_NE(PP_kInvalidFileHandle, file_handle);
  ASSERT_TRUE(WriteFile(file_handle, contents));
  CloseFileHandle(file_handle);
  ASSERT_TRUE(FileModuleLocal::DeleteFileOrDir(instance_, filename, false));
  PP_FileInfo unused;
  ASSERT_FALSE(FileModuleLocal::QueryFile(instance_, filename, &unused));

  // Test directory deletion.
  ASSERT_TRUE(FileModuleLocal::CreateDir(instance_, dirname));
  ASSERT_TRUE(FileModuleLocal::DeleteFileOrDir(instance_, dirname, false));
  ASSERT_FALSE(FileModuleLocal::QueryFile(instance_, dirname, &unused));

  // Test recursive directory deletion.
  ASSERT_TRUE(FileModuleLocal::CreateDir(instance_, dirname));
  file_handle = FileModuleLocal::OpenFile(
      instance_, dirname + "/" + filename,
      PP_FILEOPENFLAG_WRITE | PP_FILEOPENFLAG_CREATE);
  ASSERT_NE(PP_kInvalidFileHandle, file_handle);
  ASSERT_TRUE(WriteFile(file_handle, contents));
  CloseFileHandle(file_handle);
  ASSERT_FALSE(FileModuleLocal::DeleteFileOrDir(instance_, dirname, false));
  ASSERT_TRUE(FileModuleLocal::DeleteFileOrDir(instance_, dirname, true));
  ASSERT_FALSE(FileModuleLocal::QueryFile(instance_, filename, &unused));

  PASS();
}

std::string TestFlashFile::TestCreateDir() {
  SetUp();
  std::string dirname = "abc";
  PP_FileInfo info;
  ASSERT_FALSE(FileModuleLocal::QueryFile(instance_, dirname, &info));
  ASSERT_TRUE(FileModuleLocal::CreateDir(instance_, dirname));
  ASSERT_TRUE(FileModuleLocal::QueryFile(instance_, dirname, &info));
  ASSERT_EQ(PP_FILETYPE_DIRECTORY, info.type);

  PASS();
}

std::string TestFlashFile::TestQueryFile() {
  std::string filename = "abc.txt";
  std::string dirname = "def";
  std::string contents = "This is file.";
  PP_FileInfo info;

  // Test querying a file.
  PP_FileHandle file_handle = FileModuleLocal::OpenFile(instance_,
                                                        filename,
                                                        PP_FILEOPENFLAG_WRITE |
                                                        PP_FILEOPENFLAG_CREATE);
  ASSERT_NE(PP_kInvalidFileHandle, file_handle);
  ASSERT_TRUE(WriteFile(file_handle, contents));
  CloseFileHandle(file_handle);
  ASSERT_TRUE(FileModuleLocal::QueryFile(instance_, filename, &info));
  ASSERT_EQ(static_cast<size_t>(info.size), contents.size());
  ASSERT_EQ(PP_FILETYPE_REGULAR, info.type);
  // TODO(raymes): Test the other fields.

  // Test querying a directory.
  ASSERT_TRUE(FileModuleLocal::CreateDir(instance_, dirname));
  ASSERT_TRUE(FileModuleLocal::QueryFile(instance_, dirname, &info));
  ASSERT_EQ(PP_FILETYPE_DIRECTORY, info.type);
  // TODO(raymes): Test the other fields.

  // Test querying a non-existent file.
  ASSERT_FALSE(FileModuleLocal::QueryFile(instance_, "xx", &info));

  PASS();
}

std::string TestFlashFile::TestGetDirContents() {
  SetUp();
  std::vector<FileModuleLocal::DirEntry> result;
  ASSERT_TRUE(FileModuleLocal::GetDirContents(instance_, std::string(),
                                              &result));
  ASSERT_EQ(1, result.size());
  ASSERT_EQ(result[0].name, "..");
  ASSERT_EQ(result[0].is_dir, true);

  std::string filename = "abc.txt";
  std::string dirname = "def";
  std::string contents = "This is file.";
  PP_FileHandle file_handle = FileModuleLocal::OpenFile(instance_,
                                                        filename,
                                                        PP_FILEOPENFLAG_WRITE |
                                                        PP_FILEOPENFLAG_CREATE);
  ASSERT_NE(PP_kInvalidFileHandle, file_handle);
  ASSERT_TRUE(WriteFile(file_handle, contents));
  CloseFileHandle(file_handle);
  ASSERT_TRUE(FileModuleLocal::CreateDir(instance_, dirname));

  ASSERT_TRUE(
      FileModuleLocal::GetDirContents(instance_, std::string(), &result));
  FileModuleLocal::DirEntry expected[] = { { "..", true }, { filename, false },
                                           { dirname, true } };
  size_t expected_size = sizeof(expected) / sizeof(expected[0]);

  std::sort(expected, expected + expected_size, DirEntryLessThan);
  std::sort(result.begin(), result.end(), DirEntryLessThan);

  ASSERT_EQ(expected_size, result.size());
  ASSERT_TRUE(std::equal(expected, expected + expected_size, result.begin(),
                         DirEntryEqual));

  PASS();
}

std::string TestFlashFile::TestCreateTemporaryFile() {
  SetUp();
  size_t before_create = 0;
  ASSERT_SUBTEST_SUCCESS(GetItemCountUnderModuleLocalRoot(&before_create));

  PP_FileHandle file_handle = FileModuleLocal::CreateTemporaryFile(instance_);
  ASSERT_NE(PP_kInvalidFileHandle, file_handle);

  std::string contents = "This is a temp file.";
  ASSERT_TRUE(WriteFile(file_handle, contents));
  std::string read_contents;
  ASSERT_TRUE(ReadFile(file_handle, &read_contents));
  ASSERT_EQ(contents, read_contents);

  CloseFileHandle(file_handle);

  size_t after_close = 0;
  ASSERT_SUBTEST_SUCCESS(GetItemCountUnderModuleLocalRoot(&after_close));
  ASSERT_EQ(before_create, after_close);

  PASS();
}

std::string TestFlashFile::GetItemCountUnderModuleLocalRoot(
    size_t* item_count) {
  std::vector<FileModuleLocal::DirEntry> contents;
  ASSERT_TRUE(
      FileModuleLocal::GetDirContents(instance_, std::string(), &contents));
  *item_count = contents.size();
  PASS();
}
