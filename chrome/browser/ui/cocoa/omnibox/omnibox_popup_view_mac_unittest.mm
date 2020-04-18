// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_view_mac.h"

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/text_elider.h"

namespace {

class MockOmniboxPopupViewMac : public OmniboxPopupViewMac {
 public:
  MockOmniboxPopupViewMac(OmniboxView* omnibox_view,
                          OmniboxEditModel* edit_model,
                          NSTextField* field)
      : OmniboxPopupViewMac(omnibox_view, edit_model, field) {
  }

  void SetResultCount(size_t count) {
    ACMatches matches;
    for (size_t i = 0; i < count; ++i)
      matches.push_back(AutocompleteMatch());
    result_.Reset();
    result_.AppendMatches(AutocompleteInput(), matches);
  }

 protected:
  const AutocompleteResult& GetResult() const override { return result_; }

 private:
  AutocompleteResult result_;
};

class OmniboxPopupViewMacTest : public CocoaProfileTest {
 public:
  OmniboxPopupViewMacTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(OmniboxPopupViewMacTest);
};

TEST_F(OmniboxPopupViewMacTest, UpdatePopupAppearance) {
  base::scoped_nsobject<NSTextField> field(
      [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 100, 20)]);
  [[test_window() contentView] addSubview:field];

  OmniboxViewMac view(NULL, profile(), NULL, NULL);
  MockOmniboxPopupViewMac popup_view(&view, view.model(), field);

  popup_view.UpdatePopupAppearance();
  EXPECT_FALSE(popup_view.IsOpen());
  EXPECT_EQ(0, [popup_view.matrix() numberOfRows]);

  popup_view.SetResultCount(3);
  popup_view.UpdatePopupAppearance();
  EXPECT_TRUE(popup_view.IsOpen());
  EXPECT_EQ(3, [popup_view.matrix() numberOfRows]);

  popup_view.SetResultCount(5);
  popup_view.UpdatePopupAppearance();
  EXPECT_EQ(5, [popup_view.matrix() numberOfRows]);

  popup_view.SetResultCount(0);
  popup_view.UpdatePopupAppearance();
  EXPECT_FALSE(popup_view.IsOpen());
  EXPECT_EQ(0, [popup_view.matrix() numberOfRows]);
}

}  // namespace
