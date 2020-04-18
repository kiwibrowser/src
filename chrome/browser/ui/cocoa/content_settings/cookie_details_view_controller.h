// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIE_DETAILS_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIE_DETAILS_VIEW_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "net/cookies/cookie_monster.h"

@class CocoaCookieTreeNode;
@class GTMUILocalizerAndLayoutTweaker;

// Controller for the view that displays the details of a cookie,
// used both in the cookie prompt dialog as well as the
// show cookies preference sheet of content settings preferences.
@interface CookieDetailsViewController : NSViewController {
 @private
  // Allows direct access to the object controller for
  // the displayed cookie information.
  IBOutlet NSObjectController* objectController_;

  // This explicit reference to the layout tweaker is
  // required because it's necessary to reformat the view when
  // the content object changes, since the content object may
  // alter the widths of some of the fields displayed in the view.
  IBOutlet GTMUILocalizerAndLayoutTweaker* tweaker_;
}

@property(nonatomic, readonly) BOOL hasExpiration;

- (id)init;

// Configures the cookie detail view that is managed by the controller
// to display the information about a single cookie, the information
// for which is explicitly passed in the parameter |content|.
- (void)setContentObject:(id)content;

// Adjust the size of the view to exactly fix the information text fields
// that are visible inside it.
- (void)shrinkViewToFit;

// Called by the cookie tree dialog to establish a binding between
// the the detail view's object controller and the tree controller.
// This binding allows the cookie tree to use the detail view unmodified.
- (void)configureBindingsForTreeController:(NSTreeController*)controller;

// Action sent by the expiration date popup when the user
// selects the menu item "When I close my browser".
- (IBAction)setCookieDoesntHaveExplicitExpiration:(id)sender;

// Action sent by the expiration date popup when the user
// selects the menu item with an explicit date/time of expiration.
- (IBAction)setCookieHasExplicitExpiration:(id)sender;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIE_DETAILS_VIEW_CONTROLLER_H_
