// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_manager/file_manager_jstest_base.h"

class GalleryJsTest : public FileManagerJsTestBase {
 protected:
  GalleryJsTest() : FileManagerJsTestBase(
      base::FilePath(FILE_PATH_LITERAL("ui/file_manager/gallery/js"))) {}
};

IN_PROC_BROWSER_TEST_F(GalleryJsTest, ImageEncoderTest) {
  RunTest(base::FilePath(
      FILE_PATH_LITERAL("image_editor/image_encoder_unittest.html")));
}

// Disabled on ASan builds due to a consistent failure. https://crbug.com/762831
#if defined(ADDRESS_SANITIZER)
#define MAYBE_ExifEncoderTest DISABLED_ExifEncoderTest
#else
#define MAYBE_ExifEncoderTest ExifEncoderTest
#endif
IN_PROC_BROWSER_TEST_F(GalleryJsTest, MAYBE_ExifEncoderTest) {
  RunTest(base::FilePath(
      FILE_PATH_LITERAL("image_editor/exif_encoder_unittest.html")));
}

IN_PROC_BROWSER_TEST_F(GalleryJsTest, ImageViewTest) {
  RunTest(base::FilePath(
      FILE_PATH_LITERAL("image_editor/image_view_unittest.html")));
}

IN_PROC_BROWSER_TEST_F(GalleryJsTest, EntryListWatcherTest) {
  RunTest(base::FilePath(
      FILE_PATH_LITERAL("entry_list_watcher_unittest.html")));
}

IN_PROC_BROWSER_TEST_F(GalleryJsTest, GalleryUtilTest) {
  RunTest(base::FilePath(
      FILE_PATH_LITERAL("gallery_util_unittest.html")));
}

IN_PROC_BROWSER_TEST_F(GalleryJsTest, GalleryItemTest) {
  RunTest(base::FilePath(
      FILE_PATH_LITERAL("gallery_item_unittest.html")));
}

IN_PROC_BROWSER_TEST_F(GalleryJsTest, GalleryDataModelTest) {
  RunTest(base::FilePath(
      FILE_PATH_LITERAL("gallery_data_model_unittest.html")));
}

IN_PROC_BROWSER_TEST_F(GalleryJsTest, RibbonTest) {
  RunTest(base::FilePath(FILE_PATH_LITERAL("ribbon_unittest.html")));
}

IN_PROC_BROWSER_TEST_F(GalleryJsTest, SlideModeTest) {
  RunTest(base::FilePath(FILE_PATH_LITERAL("slide_mode_unittest.html")));
}

IN_PROC_BROWSER_TEST_F(GalleryJsTest, DimmableUIControllerTest) {
  RunTest(base::FilePath(
      FILE_PATH_LITERAL("dimmable_ui_controller_unittest.html")));
}
