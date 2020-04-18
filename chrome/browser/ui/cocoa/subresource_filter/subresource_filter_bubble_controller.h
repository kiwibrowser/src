// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_BUBBLE_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/content_settings/content_setting_bubble_cocoa.h"

// Displays the content filtering bubble. This is a bubble which is shown
// through content settings when the user proceeds through a Safe Browsing
// warning interstitial that is displayed when the site ahead contains deceptive
// embedded content. It explains to the user that some subresources were
// filtered and presents the checkbox to reload the page. If the check box is
// checked, the OK button's text is replaced with 'Reload' giving the user
// the ability to reload the page with filtering disabled.
@interface SubresourceFilterBubbleController : ContentSettingBubbleController

@end

// The methods on this category are used internally by the controller and are
// only exposed for testing purposes. DO NOT USE OTHERWISE.
@interface SubresourceFilterBubbleController (ExposedForTesting)
- (void)manageCheckboxChecked:(id)sender;
- (id)messageLabel;
- (id)learnMoreLink;
- (id)manageCheckbox;
- (id)doneButton;
@end

#endif  // CHROME_BROWSER_UI_COCOA_SUBRESOURCE_FILTER_SUBRESOURCE_FILTER_BUBBLE_CONTROLLER_H_
