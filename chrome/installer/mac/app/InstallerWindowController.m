// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "InstallerWindowController.h"

#import "AppDelegate.h"

@interface InstallerWindowController () {
  NSButton* importButton_;
  NSButton* defaultBrowserButton_;
  NSButton* optInButton_;
  NSButton* launchButton_;
  NSTextField* statusDescription_;
  NSTextField* downloadProgressDescription_;
  NSProgressIndicator* progressBar_;
}
@end

@implementation InstallerWindowController

// Simplify styling and naming buttons.
- (void)stylizeButton:(NSButton*)button withTitle:(NSString*)title {
  button.buttonType = NSSwitchButton;
  button.bezelStyle = NSRoundedBezelStyle;
  button.title = title;
}

// Positions and stylizes buttons.
- (void)setUpButtons {
  importButton_ = [[NSButton alloc] initWithFrame:NSMakeRect(30, 20, 300, 25)];
  [self stylizeButton:importButton_
            withTitle:@"Import from... Wait import what?"];

  defaultBrowserButton_.state = NSOnState;
  defaultBrowserButton_ =
      [[NSButton alloc] initWithFrame:NSMakeRect(30, 45, 300, 25)];
  [self stylizeButton:defaultBrowserButton_
            withTitle:@"Make Chrome the default browser."];

  optInButton_ = [[NSButton alloc] initWithFrame:NSMakeRect(30, 70, 300, 25)];
  [self stylizeButton:optInButton_ withTitle:@"Say yes to UMA."];

  launchButton_ = [[NSButton alloc] initWithFrame:NSMakeRect(310, 6, 100, 50)];
  launchButton_.buttonType = NSPushOnPushOffButton;
  launchButton_.bezelStyle = NSRoundedBezelStyle;
  launchButton_.title = @"Launch";
  [launchButton_ setEnabled:NO];
  [launchButton_ setAction:@selector(launchButtonClicked)];
}

// Simplfy styling NSTextField objects.
- (void)stylizeTextField:(NSTextField*)textField
         withDescription:(NSString*)description {
  textField.backgroundColor = NSColor.clearColor;
  textField.textColor = NSColor.blackColor;
  textField.stringValue = description;
  textField.bezeled = NO;
  textField.editable = NO;
}

// Positions and stylizes textfields.
- (void)setUpTextfields {
  statusDescription_ =
      [[NSTextField alloc] initWithFrame:NSMakeRect(20, 95, 300, 20)];
  [self stylizeTextField:statusDescription_
         withDescription:@"Working on it! While you're waiting..."];

  downloadProgressDescription_ =
      [[NSTextField alloc] initWithFrame:NSMakeRect(20, 160, 300, 20)];
  [self stylizeTextField:downloadProgressDescription_
         withDescription:@"Downloading... "];
}

// Positions and stylizes the progressbar for download and install.
- (void)setUpProgressBar {
  progressBar_ =
      [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(15, 125, 400, 50)];
  progressBar_.indeterminate = NO;
  progressBar_.style = NSProgressIndicatorBarStyle;
  progressBar_.maxValue = 100.0;
  progressBar_.minValue = 0.0;
  progressBar_.doubleValue = 0.0;
}

// Positions the main window and adds the rest of the UI elements to it.
// Prevents resizing the window so that the absolute position will look the same
// on all computers. Window is hidden until all positioning is finished.
- (id)initWithWindow:(NSWindow*)window {
  if (self = [super initWithWindow:window]) {
    [window setFrame:NSMakeRect(0, 0, 430, 220) display:YES];
    [window center];
    [window setStyleMask:[window styleMask] & ~NSResizableWindowMask];

    [self setUpButtons];
    [self setUpProgressBar];
    [self setUpTextfields];

    [window.contentView addSubview:importButton_];
    [window.contentView addSubview:defaultBrowserButton_];
    [window.contentView addSubview:optInButton_];
    [window.contentView addSubview:launchButton_];
    [window.contentView addSubview:progressBar_];
    [window.contentView addSubview:statusDescription_];
    [window.contentView addSubview:downloadProgressDescription_];
    [NSApp activateIgnoringOtherApps:YES];
    [window makeKeyAndOrderFront:self];
  }
  return self;
}

- (void)updateStatusDescription:(NSString*)text {
  // TODO: This method somehow causes ghosting of the previous string's contents
  // after a redraw. The below line of code is a temporary hack to clear the
  // ghosting behavior, but it should be replaced with a legitimate bug fix.
  downloadProgressDescription_.stringValue = @"";
  downloadProgressDescription_.stringValue = text;
}

- (void)updateDownloadProgress:(double)progressPercent {
  if (progressPercent > 0.0) {
    progressBar_.doubleValue = progressPercent;
  } else {
    // After the progress bar is made indeterminate, it will not need to track
    // determinate progress any more. Therefore, there is nothing implemented to
    // set indeterminate to NO.
    progressBar_.doubleValue = 0.0;
    progressBar_.indeterminate = YES;
    [progressBar_ startAnimation:nil];
  }
}

- (void)enableLaunchButton {
  [launchButton_ setEnabled:YES];
}

- (void)launchButtonClicked {
  // TODO: Launch the app and start ejecting disk.
  [NSApp terminate:nil];
}

- (BOOL)isUserMetricsChecked {
  return optInButton_.state == NSOnState;
}

- (BOOL)isDefaultBrowserChecked {
  return defaultBrowserButton_.state == NSOnState;
}

@end
