// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_SELECTED_KEYWORD_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_SELECTED_KEYWORD_DECORATION_H_

#include <string>

#import <Cocoa/Cocoa.h>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/cocoa/location_bar/bubble_decoration.h"

class SelectedKeywordDecoration : public BubbleDecoration {
 public:
  SelectedKeywordDecoration();
  ~SelectedKeywordDecoration() override;

  // Return the color used to draw the SelectedKeywordDecoration in MD.
  NSColor* GetBackgroundBorderColor() override;

  // Calculates appropriate full and partial label strings based on
  // inputs.
  void SetKeyword(const base::string16& keyword, bool is_extension_keyword);

  // Determines what combination of labels and image will best fit
  // within |width|, makes those current for |BubbleDecoration|, and
  // return the resulting width.
  CGFloat GetWidthForSpace(CGFloat width) override;

  void SetImage(NSImage* image);

  NSString* GetAccessibilityLabel() override;
  bool IsAccessibilityIgnored() override;

 private:
  friend class SelectedKeywordDecorationTest;
  FRIEND_TEST_ALL_PREFIXES(SelectedKeywordDecorationTest,
                           UsesPartialKeywordIfNarrow);

  base::scoped_nsobject<NSImage> search_image_;
  base::scoped_nsobject<NSString> full_string_;
  base::scoped_nsobject<NSString> partial_string_;

  DISALLOW_COPY_AND_ASSIGN(SelectedKeywordDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_SELECTED_KEYWORD_DECORATION_H_
