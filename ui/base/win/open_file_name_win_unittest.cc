// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/open_file_name_win.h"

#include <stddef.h>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const HWND kHwnd = reinterpret_cast<HWND>(0xDEADBEEF);
const DWORD kFlags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING;

void SetResult(const base::string16& result, ui::win::OpenFileName* ofn) {
  if (ofn->GetOPENFILENAME()->nMaxFile <= result.size()) {
    ADD_FAILURE() << "filename buffer insufficient.";
    return;
  }
  if (result.empty()) {
    ofn->GetOPENFILENAME()->lpstrFile[0] = 0;
  } else {
    // Because the result has embedded nulls, we must memcpy.
    memcpy(ofn->GetOPENFILENAME()->lpstrFile,
           result.c_str(),
           (result.size() + 1) * sizeof(result[0]));
  }
}

void CheckFilters(
    const std::vector<std::tuple<base::string16, base::string16>>& expected,
    const std::vector<std::tuple<base::string16, base::string16>>& actual) {
  if (expected.size() != actual.size()) {
    ADD_FAILURE() << "filter count mismatch. Got " << actual.size()
                  << " expected " << expected.size() << ".";
    return;
  }

  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(std::get<0>(expected[i]), std::get<0>(actual[i]))
        << "Mismatch at index " << i;
    EXPECT_EQ(std::get<1>(expected[i]), std::get<1>(actual[i]))
        << "Mismatch at index " << i;
  }
}

void CheckFilterString(const base::string16& expected,
                       const ui::win::OpenFileName& ofn) {
  if (!ofn.GetOPENFILENAME()->lpstrFilter) {
    ADD_FAILURE() << "Filter string is NULL.";
    return;
  }
  if (expected.size() == 0) {
    EXPECT_EQ(0, ofn.GetOPENFILENAME()->lpstrFilter[0]);
  } else {
    EXPECT_EQ(0,
              memcmp(expected.c_str(),
                     ofn.GetOPENFILENAME()->lpstrFilter,
                     expected.size() + 1 * sizeof(expected[0])));
  }
}

void CheckResult(const base::string16& expected,
                 const ui::win::OpenFileName& ofn) {
  if (!ofn.GetOPENFILENAME()->lpstrFile) {
    ADD_FAILURE() << "File string is NULL.";
    return;
  }
  if (expected.size() == 0) {
    EXPECT_EQ(0, ofn.GetOPENFILENAME()->lpstrFile[0]);
  } else {
    EXPECT_EQ(0,
              memcmp(expected.c_str(),
                     ofn.GetOPENFILENAME()->lpstrFile,
                     expected.size() + 1 * sizeof(expected[0])));
  }
}

}  // namespace

TEST(OpenFileNameTest, Initialization) {
  ui::win::OpenFileName ofn(kHwnd, kFlags);
  EXPECT_EQ(kHwnd, ofn.GetOPENFILENAME()->hwndOwner);
  EXPECT_EQ(kFlags, ofn.GetOPENFILENAME()->Flags);
  EXPECT_EQ(sizeof(OPENFILENAME), ofn.GetOPENFILENAME()->lStructSize);
  ASSERT_TRUE(ofn.GetOPENFILENAME()->lpstrFile);
  ASSERT_GT(ofn.GetOPENFILENAME()->nMaxFile, 0u);
  EXPECT_EQ(0, ofn.GetOPENFILENAME()->lpstrFile[0]);
}

TEST(OpenFileNameTest, SetInitialSelection) {
  const base::FilePath kDirectory(L"C:\\directory\\child_directory");
  const base::FilePath kFile(L"file_name.ext");
  ui::win::OpenFileName ofn(kHwnd, kFlags);
  ofn.SetInitialSelection(kDirectory, kFile);
  EXPECT_EQ(kDirectory, base::FilePath(ofn.GetOPENFILENAME()->lpstrInitialDir));
  EXPECT_EQ(kFile, base::FilePath(ofn.GetOPENFILENAME()->lpstrFile));

  ofn.SetInitialSelection(kDirectory, base::FilePath());
  EXPECT_EQ(kDirectory, base::FilePath(ofn.GetOPENFILENAME()->lpstrInitialDir));
  // Filename buffer will still be a valid pointer, to receive a result.
  ASSERT_TRUE(ofn.GetOPENFILENAME()->lpstrFile);
  EXPECT_EQ(base::FilePath(), base::FilePath(ofn.GetOPENFILENAME()->lpstrFile));

  ofn.SetInitialSelection(base::FilePath(), base::FilePath());
  // No initial directory will lead to a NULL buffer.
  ASSERT_FALSE(ofn.GetOPENFILENAME()->lpstrInitialDir);
  ASSERT_TRUE(ofn.GetOPENFILENAME()->lpstrFile);
  EXPECT_EQ(base::FilePath(), base::FilePath(ofn.GetOPENFILENAME()->lpstrFile));

  // Make sure that both values are cleared when directory is missing.
  ofn.SetInitialSelection(kDirectory, kFile);
  EXPECT_EQ(kDirectory, base::FilePath(ofn.GetOPENFILENAME()->lpstrInitialDir));
  EXPECT_EQ(kFile, base::FilePath(ofn.GetOPENFILENAME()->lpstrFile));
  ofn.SetInitialSelection(base::FilePath(), base::FilePath());
  ASSERT_FALSE(ofn.GetOPENFILENAME()->lpstrInitialDir);
  ASSERT_TRUE(ofn.GetOPENFILENAME()->lpstrFile);
  EXPECT_EQ(base::FilePath(), base::FilePath(ofn.GetOPENFILENAME()->lpstrFile));

  // File is ignored in absence of a directory.
  ofn.SetInitialSelection(base::FilePath(), kFile);
  ASSERT_FALSE(ofn.GetOPENFILENAME()->lpstrInitialDir);
  ASSERT_TRUE(ofn.GetOPENFILENAME()->lpstrFile);
  EXPECT_EQ(base::FilePath(), base::FilePath(ofn.GetOPENFILENAME()->lpstrFile));
}

TEST(OpenFileNameTest, GetSingleResultFromSingleSelect) {
  ui::win::OpenFileName ofn(kHwnd, kFlags);
  base::FilePath result;

  SetResult(L"C:\\dir\\file", &ofn);
  result = ofn.GetSingleResult();
  EXPECT_EQ(base::FilePath(L"C:\\dir\\file"), result);

  SetResult(L"", &ofn);
  result = ofn.GetSingleResult();
  EXPECT_EQ(base::FilePath(), result);
}

TEST(OpenFileNameTest, GetSingleResultFromMultiSelect) {
  const base::string16 kNull(L"\0", 1);
  ui::win::OpenFileName ofn(kHwnd, kFlags | OFN_ALLOWMULTISELECT);
  base::FilePath result;

  SetResult(L"C:\\dir\\file" + kNull, &ofn);
  result = ofn.GetSingleResult();
  EXPECT_EQ(base::FilePath(L"C:\\dir\\file"), result);

  SetResult(L"C:\\dir" + kNull + L"file" + kNull, &ofn);
  result = ofn.GetSingleResult();
  EXPECT_EQ(base::FilePath(L"C:\\dir\\file"), result);

  SetResult(L"C:\\dir" + kNull + L"file" + kNull + L"otherfile" + kNull, &ofn);
  result = ofn.GetSingleResult();
  EXPECT_EQ(base::FilePath(), result);

  SetResult(L"", &ofn);
  result = ofn.GetSingleResult();
  EXPECT_EQ(base::FilePath(), result);
}

TEST(OpenFileNameTest, GetResult) {
  const base::string16 kNull(L"\0", 1);

  ui::win::OpenFileName ofn(kHwnd, kFlags | OFN_ALLOWMULTISELECT);
  base::FilePath directory;
  std::vector<base::FilePath> filenames;

  SetResult(L"C:\\dir\\file" + kNull, &ofn);
  ofn.GetResult(&directory, &filenames);
  EXPECT_EQ(base::FilePath(L"C:\\dir"), directory);
  ASSERT_EQ(1u, filenames.size());
  EXPECT_EQ(base::FilePath(L"file"), filenames[0]);

  directory.clear();
  filenames.clear();

  SetResult(L"C:\\dir" + kNull + L"file" + kNull, &ofn);
  ofn.GetResult(&directory, &filenames);
  EXPECT_EQ(base::FilePath(L"C:\\dir"), directory);
  ASSERT_EQ(1u, filenames.size());
  EXPECT_EQ(base::FilePath(L"file"), filenames[0]);

  directory.clear();
  filenames.clear();

  SetResult(L"C:\\dir" + kNull + L"file" + kNull + L"otherfile" + kNull, &ofn);
  ofn.GetResult(&directory, &filenames);
  EXPECT_EQ(base::FilePath(L"C:\\dir"), directory);
  ASSERT_EQ(2u, filenames.size());
  EXPECT_EQ(base::FilePath(L"file"), filenames[0]);
  EXPECT_EQ(base::FilePath(L"otherfile"), filenames[1]);

  directory.clear();
  filenames.clear();

  SetResult(L"", &ofn);
  ofn.GetResult(&directory, &filenames);
  EXPECT_EQ(base::FilePath(), directory);
  ASSERT_EQ(0u, filenames.size());
}

TEST(OpenFileNameTest, SetAndGetFilters) {
  const base::string16 kNull(L"\0", 1);

  ui::win::OpenFileName ofn(kHwnd, kFlags);
  std::vector<std::tuple<base::string16, base::string16>> filters;
  ofn.SetFilters(filters);
  EXPECT_FALSE(ofn.GetOPENFILENAME()->lpstrFilter);
  CheckFilters(filters,
               ui::win::OpenFileName::GetFilters(ofn.GetOPENFILENAME()));

  filters.push_back(
      std::make_tuple(base::string16(L"a"), base::string16(L"b")));
  ofn.SetFilters(filters);
  CheckFilterString(L"a" + kNull + L"b" + kNull, ofn);
  CheckFilters(filters,
               ui::win::OpenFileName::GetFilters(ofn.GetOPENFILENAME()));

  filters.push_back(
      std::make_tuple(base::string16(L"X"), base::string16(L"Y")));
  ofn.SetFilters(filters);
  CheckFilterString(L"a" + kNull + L"b" + kNull + L"X" + kNull + L"Y" + kNull,
                    ofn);
  CheckFilters(filters,
               ui::win::OpenFileName::GetFilters(ofn.GetOPENFILENAME()));
}

TEST(OpenFileNameTest, SetResult) {
  const base::string16 kNull(L"\0", 1);

  ui::win::OpenFileName ofn(kHwnd, kFlags);
  base::FilePath directory;
  std::vector<base::FilePath> filenames;

  ui::win::OpenFileName::SetResult(directory, filenames, ofn.GetOPENFILENAME());
  CheckResult(L"", ofn);

  directory = base::FilePath(L"C:\\dir");
  filenames.push_back(base::FilePath(L"file"));
  ui::win::OpenFileName::SetResult(directory, filenames, ofn.GetOPENFILENAME());
  CheckResult(L"C:\\dir\\file" + kNull, ofn);

  filenames.push_back(base::FilePath(L"otherfile"));
  ui::win::OpenFileName::SetResult(directory, filenames, ofn.GetOPENFILENAME());
  CheckResult(L"C:\\dir" + kNull + L"file" + kNull + L"otherfile" + kNull, ofn);

  base::char16 short_buffer[10] = L"";

  ofn.GetOPENFILENAME()->lpstrFile = short_buffer;
  ofn.GetOPENFILENAME()->nMaxFile = arraysize(short_buffer);
  ui::win::OpenFileName::SetResult(directory, filenames, ofn.GetOPENFILENAME());
  CheckResult(L"", ofn);
}
