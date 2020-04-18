// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_button_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/resources/grit/ui_resources.h"

namespace {

class BookmarkBarFolderButtonCellTest : public CocoaTest {
};

// Basic creation.
TEST_F(BookmarkBarFolderButtonCellTest, Create) {
  base::scoped_nsobject<BookmarkBarFolderButtonCell> cell;
  cell.reset([[BookmarkBarFolderButtonCell buttonCellForNode:nil
                                                        text:nil
                                                       image:nil
                                              menuController:nil] retain]);
  EXPECT_TRUE(cell);
}

TEST_F(BookmarkBarFolderButtonCellTest, FaviconPositioning) {
  NSRect frame = NSMakeRect(0, 0, 50, 30);
  base::scoped_nsobject<NSButton> view([[NSButton alloc] initWithFrame:frame]);
  base::scoped_nsobject<NSButton> folder_view(
      [[NSButton alloc] initWithFrame:frame]);

  ASSERT_TRUE(view.get());
  ASSERT_TRUE(folder_view.get());

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  base::scoped_nsobject<NSImage> image(
      rb.GetNativeImageNamed(IDR_DEFAULT_FAVICON).CopyNSImage());
  ASSERT_TRUE(image.get());

  base::scoped_nsobject<BookmarkButtonCell> cell(
      [[BookmarkButtonCell alloc] initTextCell:@"Testing"]);
  base::scoped_nsobject<BookmarkBarFolderButtonCell> folder_cell(
      [[BookmarkBarFolderButtonCell buttonCellForNode:nil
                                                 text:@"Testing"
                                                image:image
                                       menuController:nil] retain]);

  ASSERT_TRUE(cell.get());
  ASSERT_TRUE(folder_cell.get());

  [view setCell:cell.get()];
  [folder_view setCell:folder_cell.get()];

  [[test_window() contentView] addSubview:view];
  [[test_window() contentView] addSubview:folder_view];

  NSRect rect = NSMakeRect(20, 20, 20, 20);

  [cell setBookmarkCellText:@"" image:image];
  float icon_x_without_title = ([cell imageRectForBounds:rect]).origin.x;
  float cell_width_without_title = ([cell cellSize]).width;

  [cell setBookmarkCellText:@"test" image:image];
  float icon_x_with_title = ([cell imageRectForBounds:rect]).origin.x;
  float cell_width_with_title = ([cell cellSize]).width;

  EXPECT_EQ(icon_x_without_title, icon_x_with_title);
  EXPECT_LT(cell_width_without_title, cell_width_with_title);

  [folder_cell setBookmarkCellText:@"" image:image];
  float folder_cell_x_without_title = ([cell imageRectForBounds:rect]).origin.x;
  float folder_cell_width_without_title = ([cell cellSize]).width;

  [folder_cell setBookmarkCellText:@"test" image:image];
  float folder_cell_x_with_title = ([cell imageRectForBounds:rect]).origin.x;
  float folder_cell_width_with_title = ([cell cellSize]).width;

  EXPECT_EQ(folder_cell_x_without_title, folder_cell_x_with_title);
  EXPECT_EQ(folder_cell_width_without_title, folder_cell_width_with_title);
}

} // namespace
