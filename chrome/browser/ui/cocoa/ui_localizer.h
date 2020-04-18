// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_UI_LOCALIZER_H_
#define CHROME_BROWSER_UI_COCOA_UI_LOCALIZER_H_

#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizer.h"

@class NSString;

// A base class for generated localizers.
//
// To use this, include your xib file in the list generate_localizer scans (see
// chrome_browser.gypi).  Then add an instance of ChromeUILocalizer to the xib.
// Connect the owner_ outlet of the instance to the "File's Owner" of the xib.
// It expects the owner_ outlet to be an instance or subclass of
// NSWindowController or NSViewController.  It will then localize any items in
// the NSWindowController's window and subviews, or the NSViewController's view
// and subviews, when awakeFromNib is called on the instance.  You can
// optionally hook up otherObjectToLocalize_ and yetAnotherObjectToLocalize_ and
// those will also be localized. Strings in the xib that you want localized must
// start with ^IDS. The value must be a valid resource constant.
// Things that will be localized are:
// - Titles and altTitles (for menus, buttons, windows, menuitems, -tabViewItem)
// - -stringValue (for labels)
// - tooltips
// - accessibility help
// - accessibility descriptions
// - menus
@interface ChromeUILocalizer : GTMUILocalizer
@end

#endif  // CHROME_BROWSER_UI_COCOA_UI_LOCALIZER_H_
