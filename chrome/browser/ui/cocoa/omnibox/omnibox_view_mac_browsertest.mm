// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"

#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/location_bar/autocomplete_text_field_cell.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "ui/base/clipboard/clipboard_util_mac.h"

class OmniboxViewMacBrowserTest : public InProcessBrowserTest {
 public:
  OmniboxViewMac* GetOmnibox() {
    return static_cast<OmniboxViewMac*>(
        browser()->window()->GetLocationBar()->GetOmniboxView());
  }

 private:
  test::ScopedMacViewsBrowserMode cocoa_browser_mode_{false};
};

// Verify that the OmniboxViewMac::SetFocus API works.
IN_PROC_BROWSER_TEST_F(OmniboxViewMacBrowserTest, SetFocus) {
  EXPECT_FALSE([[GetOmnibox()->field() window] firstResponder] ==
                GetOmnibox()->field());
  GetOmnibox()->SetFocus();
  // Unfortunately there's no way to test that the field has keyboard focus.
}

// Verify that the OmniboxViewMac::ApplyCaretVisibility API works.
IN_PROC_BROWSER_TEST_F(OmniboxViewMacBrowserTest, ApplyCaretVisibility) {
  EXPECT_FALSE([[GetOmnibox()->field() cell] hideFocusState]);

  // |SetCaretVisibility(false)| should hide focus state.
  GetOmnibox()->model()->SetCaretVisibility(false);
  GetOmnibox()->ApplyCaretVisibility();
  EXPECT_TRUE([[GetOmnibox()->field() cell] hideFocusState]);

  // |SetCaretVisibility(true)| should show focus state.
  GetOmnibox()->model()->SetCaretVisibility(true);
  GetOmnibox()->ApplyCaretVisibility();
  EXPECT_FALSE([[GetOmnibox()->field() cell] hideFocusState]);

  // Focusing on the omnibox should show focus state.
  GetOmnibox()->model()->SetCaretVisibility(false);
  GetOmnibox()->ApplyCaretVisibility();
  EXPECT_TRUE([[GetOmnibox()->field() cell] hideFocusState]);
  GetOmnibox()->SetFocus();
  EXPECT_FALSE([[GetOmnibox()->field() cell] hideFocusState]);
}


// Verify that mouse down sets caret visibility to true.
IN_PROC_BROWSER_TEST_F(OmniboxViewMacBrowserTest, MouseDownCaretVisibility) {
  GetOmnibox()->model()->SetCaretVisibility(false);
  EXPECT_FALSE(GetOmnibox()->model()->is_caret_visible());
  GetOmnibox()->OnMouseDown(0);
  EXPECT_TRUE(GetOmnibox()->model()->is_caret_visible());
}

// Verify that copying text from the omnibox into the pasteboard works.
IN_PROC_BROWSER_TEST_F(OmniboxViewMacBrowserTest, CopyToPasteboard) {
  std::string text = "orange yam";
  NSString* pboard_type = @"com.google.chrome.asdf";

  scoped_refptr<ui::UniquePasteboard> pasteboard = new ui::UniquePasteboard;
  [[GetOmnibox()->field() currentEditor]
      setString:base::SysUTF8ToNSString(text)];
  [[GetOmnibox()->field() currentEditor]
      setSelectedRange:NSMakeRange(0, text.size())];

  GetOmnibox()->model()->SetUserText(base::UTF8ToUTF16(text));
  GetOmnibox()->CopyToPasteboard(pasteboard->get());

  NSString* pasteboard_string =
      [pasteboard->get() stringForType:NSPasteboardTypeString];
  EXPECT_EQ(text, pasteboard_string.UTF8String);

  // Clear the pasteboard and set some new contents, using a custom Pboard type.
  [pasteboard->get() clearContents];
  [pasteboard->get()
      setString:@"bad result"
        forType:ui::ClipboardUtil::UTIForPasteboardType(pboard_type)];
  GetOmnibox()->CopyToPasteboard(pasteboard->get());

  // Check that the custom Pboard type is no longer present.
  EXPECT_FALSE([pasteboard->get()
      stringForType:ui::ClipboardUtil::UTIForPasteboardType(pboard_type)]);

  // Check that the contents of the omnibox were copied to the clipboard.
  pasteboard_string = [pasteboard->get() stringForType:NSPasteboardTypeString];
  EXPECT_EQ(text, pasteboard_string.UTF8String);
}
