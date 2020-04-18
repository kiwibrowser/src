// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/omnibox/clipboard_utils.h"

#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

using base::ASCIIToUTF16;

namespace {

class ClipboardUtilsTest : public PlatformTest {
 public:
  ClipboardUtilsTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  void TearDown() override {
    ui::Clipboard::DestroyClipboardForCurrentThread();
  }

 private:
  // Windows requires a message loop for clipboard access.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(ClipboardUtilsTest, GetClipboardText) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  const base::string16 kPlainText(ASCIIToUTF16("test text"));
  const std::string kURL("http://www.example.com/");

  // Can we pull straight text off the clipboard?
  {
    ui::ScopedClipboardWriter clipboard_writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteText(kPlainText);
  }
  EXPECT_EQ(kPlainText, GetClipboardText());

  // Can we pull a string consists of white-space?
  const base::string16 kSpace6(ASCIIToUTF16("      "));
  const base::string16 kSpace1(ASCIIToUTF16(" "));
  {
    ui::ScopedClipboardWriter clipboard_writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteText(kSpace6);
  }
  EXPECT_EQ(kSpace1, GetClipboardText());

  // Does an empty clipboard get empty text?
  clipboard->Clear(ui::CLIPBOARD_TYPE_COPY_PASTE);
  EXPECT_EQ(base::string16(), GetClipboardText());

// Bookmark clipboard apparently not supported on Linux.
// See TODO on ClipboardText.BookmarkTest.
#if !defined(OS_POSIX) || defined(OS_MACOSX)
  const base::string16 kTitle(ASCIIToUTF16("The Example Company"));
  // Can we pull a bookmark off the clipboard?
  {
    ui::ScopedClipboardWriter clipboard_writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteBookmark(kTitle, kURL);
  }
  EXPECT_EQ(ASCIIToUTF16(kURL), GetClipboardText());

  // Do we pull text in preference to a bookmark?
  {
    ui::ScopedClipboardWriter clipboard_writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteText(kPlainText);
    clipboard_writer.WriteBookmark(kTitle, kURL);
  }
  EXPECT_EQ(kPlainText, GetClipboardText());
#endif

  // Do we get nothing if there is neither text nor a bookmark?
  {
    const base::string16 kMarkup(ASCIIToUTF16("<strong>Hi!</string>"));
    ui::ScopedClipboardWriter clipboard_writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
    clipboard_writer.WriteHTML(kMarkup, kURL);
  }
  EXPECT_TRUE(GetClipboardText().empty());
}

}  // namespace
