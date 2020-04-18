// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "AppDelegate.h"

#include <Security/Security.h>

#include "chrome/common/chrome_switches.h"

#import "Downloader.h"
#import "InstallerWindowController.h"
#import "NSError+ChromeInstallerAdditions.h"
#import "NSAlert+ChromeInstallerAdditions.h"
#import "AuthorizedInstall.h"
#import "OmahaCommunication.h"
#import "Unpacker.h"

@interface NSAlert ()
- (void)beginSheetModalForWindow:(NSWindow*)sheetWindow
               completionHandler:
                   (void (^__nullable)(NSModalResponse returnCode))handler;
@end

@interface AppDelegate ()<NSWindowDelegate,
                          OmahaCommunicationDelegate,
                          DownloaderDelegate,
                          UnpackDelegate> {
  InstallerWindowController* installerWindowController_;
  AuthorizedInstall* authorizedInstall_;
  BOOL preventTermination_;
}
@property(strong) NSWindow* window;
- (void)exit;
@end

@implementation AppDelegate
@synthesize window = window_;

// Sets up the main window and begins the downloading process.
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  // TODO: Despite what the code implies -- when the installer is run, the main
  // window of the application is not visible until the user has taken action on
  // the Authorization modal.
  window_.delegate = self;
  installerWindowController_ =
      [[InstallerWindowController alloc] initWithWindow:window_];
  authorizedInstall_ = [[AuthorizedInstall alloc] init];
  if ([authorizedInstall_ loadInstallationTool]) {
    [self startDownload];
  } else {
    [self onLoadInstallationToolFailure];
  }
}

- (void)applicationWillTerminate:(NSNotification*)aNotification {
}

- (NSApplicationTerminateReply)applicationShouldTerminate:
    (NSApplication*)sender {
  return preventTermination_ ? NSTerminateCancel : NSTerminateNow;
}

// This function effectively takes the place of
// applicationShouldTerminateAfterLastWindowClosed: to make sure that the
// application does correctly terminate after closing the installer, but does
// not terminate when we call orderOut: to hide the installer during its
// tear-down steps. If the user quits the application, the below delegate
// method gets called. However, when orderOut is called, the below delegate
// method does not get called.
- (BOOL)windowShouldClose:(id)sender {
  [self exit];
  return YES;
}

- (void)exit {
  preventTermination_ = NO;
  [NSApp terminate:nil];
}

- (void)startDownload {
  [installerWindowController_ updateStatusDescription:@"Initializing..."];

  OmahaCommunication* omahaMessenger = [[OmahaCommunication alloc] init];
  omahaMessenger.delegate = self;
  [omahaMessenger fetchDownloadURLs];
}

- (void)onLoadInstallationToolFailure {
  NSError* loadToolError = [NSError
       errorForAlerts:@"Internal Error"
      withDescription:
          @"Your Chrome Installer may be corrupted. Download and try again."
        isRecoverable:NO];
  [self displayError:loadToolError];
}

- (void)omahaCommunication:(OmahaCommunication*)messenger
                 onSuccess:(NSArray*)URLs {
  [installerWindowController_ updateStatusDescription:@"Downloading..."];

  Downloader* download = [[Downloader alloc] init];
  download.delegate = self;
  [download downloadChromeImageFrom:[URLs firstObject]];
}

- (void)omahaCommunication:(OmahaCommunication*)messenger
                 onFailure:(NSError*)error {
  NSError* networkError =
      [NSError errorForAlerts:@"Network Error"
              withDescription:@"Could not connect to Chrome server."
                isRecoverable:YES];
  [self displayError:networkError];
}

// Bridge method from Downloader to InstallerWindowController. Allows Downloader
// to update the progressbar without having direct access to any UI obejcts.
- (void)downloader:(Downloader*)download percentProgress:(double)percentage {
  [installerWindowController_ updateDownloadProgress:(double)percentage];
}

- (void)downloader:(Downloader*)download onSuccess:(NSURL*)diskImageURL {
  [installerWindowController_ updateStatusDescription:@"Installing..."];
  [installerWindowController_ enableLaunchButton];

  Unpacker* unpacker = [[Unpacker alloc] init];
  unpacker.delegate = self;
  [unpacker mountDMGFromURL:diskImageURL];
}

- (void)downloader:(Downloader*)download onFailure:(NSError*)error {
  NSError* downloadError =
      [NSError errorForAlerts:@"Download Failure"
              withDescription:@"Unable to download Google Chrome."
                isRecoverable:NO];
  [self displayError:downloadError];
}

- (void)unpacker:(Unpacker*)unpacker onMountSuccess:(NSString*)tempAppPath {
  SecStaticCodeRef diskStaticCode;
  SecRequirementRef diskRequirement;
  // TODO: Include some better error handling below than NSLog
  OSStatus oserror;
  oserror = SecStaticCodeCreateWithPath(
      (__bridge CFURLRef)[NSURL fileURLWithPath:tempAppPath isDirectory:NO],
      kSecCSDefaultFlags, &diskStaticCode);
  if (oserror != errSecSuccess)
    NSLog(@"code %d", oserror);
  // TODO: The below requirement is too general as most signed entities have the
  // below requirement; replace it with something adequately specific.
  oserror =
      SecRequirementCreateWithString((CFStringRef) @"anchor apple generic",
                                     kSecCSDefaultFlags, &diskRequirement);
  if (oserror != errSecSuccess)
    NSLog(@"requirement %d", oserror);
  oserror = SecStaticCodeCheckValidity(diskStaticCode, kSecCSDefaultFlags,
                                       diskRequirement);
  if (oserror != errSecSuccess)
    NSLog(@"static code %d", oserror);

  // Calling this function will change the progress bar into an indeterminate
  // one. We won't need to update the progress bar any more after this point.
  [installerWindowController_ updateDownloadProgress:-1.0];
  // By disabling closing the window or quitting, we can tell the user that
  // closing the application at this point is not a good idea.
  window_.styleMask &= ~NSClosableWindowMask;
  preventTermination_ = YES;

  NSString* chromeInApplicationsFolder =
      [authorizedInstall_ startInstall:tempAppPath];

  NSMutableArray* installerSettings = [[NSMutableArray alloc] init];
  if ([installerWindowController_ isUserMetricsChecked])
    [installerSettings
        addObject:[NSString stringWithUTF8String:switches::kEnableUserMetrics]];
  if ([installerWindowController_ isDefaultBrowserChecked])
    [installerSettings
        addObject:[NSString
                      // NOTE: the |kMakeDefaultBrowser| constant used as a
                      // command-line switch here only will apply at a user
                      // level, since the application itself is not running with
                      // privileges. grt@ suggested this constant should be
                      // renamed |kMakeDefaultBrowserforUser|.
                      stringWithUTF8String:switches::kMakeDefaultBrowser]];

  NSError* error = nil;
  [[NSWorkspace sharedWorkspace]
      launchApplicationAtURL:[NSURL fileURLWithPath:chromeInApplicationsFolder
                                        isDirectory:NO]
                     options:NSWorkspaceLaunchDefault
               configuration:@{
                 NSWorkspaceLaunchConfigurationArguments : installerSettings
               }
                       error:&error];
  if (error) {
    NSLog(@"Chrome failed to launch: %@", error);
  }

  // Begin teardown step!
  dispatch_async(dispatch_get_main_queue(), ^{
    [window_ orderOut:nil];
  });

  [unpacker unmountDMG];
}

- (void)unpacker:(Unpacker*)unpacker onMountFailure:(NSError*)error {
  NSError* extractError =
      [NSError errorForAlerts:@"Install Error"
              withDescription:@"Unable to add Google Chrome to Applications."
                isRecoverable:NO];
  [self displayError:extractError];
}

- (void)unpacker:(Unpacker*)unpacker onUnmountSuccess:(NSString*)mountpath {
  NSLog(@"we're done here!");
  [self exit];
}

- (void)unpacker:(Unpacker*)unpacker onUnmountFailure:(NSError*)error {
  NSLog(@"error unmounting");
  // NOTE: Since we are not deleting the temporary folder if the unmount fails,
  // we'll just leave it up to the computer to delete the temporary folder on
  // its own time and to unmount the disk during a restart at some point. There
  // is no other work to be done in the mean time.
  [self exit];
}

// Displays an alert on the main window using the contents of the passed in
// error.
- (void)displayError:(NSError*)error {
  NSAlert* alertForUser = [NSAlert alertWithError:error];
  dispatch_async(dispatch_get_main_queue(), ^{
    [alertForUser beginSheetModalForWindow:window_
                         completionHandler:^(NSModalResponse returnCode) {
                           if (returnCode != [alertForUser quitResponse]) {
                             [self startDownload];
                           } else {
                             [NSApp terminate:nil];
                           }
                         }];
  });
}

@end
