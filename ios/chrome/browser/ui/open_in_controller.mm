// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/open_in_controller.h"

#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/task_scheduler/post_task.h"

#include "base/sequenced_task_runner.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/alert_coordinator/alert_coordinator.h"
#import "ios/chrome/browser/ui/open_in_controller_testing.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/web/public/web_thread.h"
#import "ios/web/web_state/ui/crw_web_controller.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "ui/base/l10n/l10n_util_mac.h"
#import "ui/gfx/ios/NSString+CrStringDrawing.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// The path in the temp directory containing documents that are to be opened in
// other applications.
static NSString* const kDocumentsTempPath = @"OpenIn";

static const int kHTTPResponseCodeSucceeded = 200;

// Duration of the show/hide animation for the |openInToolbar_|.
const NSTimeInterval kOpenInToolbarAnimationDuration = 0.2;

// Duration to show or hide the |overlayedView_|.
const NSTimeInterval kOverlayViewAnimationDuration = 0.3;

// Time interval after which the |openInToolbar_| is automatically hidden.
const NSTimeInterval kOpenInToolbarDisplayDuration = 2.0;

// Text size used for the label indicating a download in progress.
const CGFloat kLabelTextSize = 22.0;

// Alpha value for the background view of |overlayedView_|.
const CGFloat kOverlayedViewBackgroundAlpha = 0.6;

// Width of the label displayed on the |overlayedView_| as a percentage of the
// |overlayedView_|'s width.
const CGFloat kOverlayedViewLabelWidthPercentage = 0.7;

// Bottom margin for the label displayed on the |overlayedView_|.
const CGFloat kOverlayedViewLabelBottomMargin = 60;

}  // anonymous namespace

@interface OpenInController () {
  // AlertCoordinator for showing an alert if no applications were found to open
  // the current document.
  AlertCoordinator* _alertCoordinator;
}

// URLFetcher delegate method called when |fetcher_| completes a request.
- (void)urlFetchDidComplete:(const net::URLFetcher*)source;
// Ensures the destination directory is created and any contained obsolete files
// are deleted. Returns YES if the directory is created successfully.
+ (BOOL)createDestinationDirectoryAndRemoveObsoleteFiles;
// Starts downloading the file at path |kDocumentsTempPath| with the name
// |suggestedFilename_|.
- (void)startDownload;
// Shows the overlayed toolbar |openInToolbar_|.
- (void)showOpenInToolbar;
// Hides the overlayed toolbar |openInToolbar_|.
- (void)hideOpenInToolbar;
// Called when there is a tap on the |webController_|'s view to display the
// overlayed toolbar |openInToolbar_| if necessary and (re)schedule the
// |openInTimer_|.
- (void)handleTapFrom:(UIGestureRecognizer*)gestureRecognizer;
// Downloads the file at |documentURL_| and presents the OpenIn menu for opening
// it in other applications.
- (void)exportFileWithOpenInMenuAnchoredAt:(id)sender;
// Called when there is a tap on the |overlayedView_| to cancel the file
// download.
- (void)handleTapOnOverlayedView:(UIGestureRecognizer*)gestureRecognizer;
// Removes |overlayedView_| from the top view of the application.
- (void)removeOverlayedView;
// Shows an alert with the given error message.
- (void)showErrorWithMessage:(NSString*)message;
// Presents the OpenIn menu for the file at |fileURL|.
- (void)presentOpenInMenuForFileAtURL:(NSURL*)fileURL;
// Removes the file at path |path|.
- (void)removeDocumentAtPath:(NSString*)path;
// Removes all the stored files at path |path|.
+ (void)removeAllStoredDocumentsAtPath:(NSString*)path;
// Shows an overlayed spinner on the top view to indicate that a file download
// is in progress.
- (void)showDownloadOverlayView;
// Returns a toolbar with an "Open in..." button to be overlayed on a document
// on tap.
- (UIToolbar*)openInToolbar;
@end

// Bridge to deliver method calls from C++ to the |OpenInController| class.
class OpenInControllerBridge
    : public net::URLFetcherDelegate,
      public base::RefCountedThreadSafe<OpenInControllerBridge> {
 public:
  explicit OpenInControllerBridge(OpenInController* owner) : owner_(owner) {}

  void OnURLFetchComplete(const net::URLFetcher* source) override {
    DCHECK(owner_);
    [owner_ urlFetchDidComplete:source];
  }

  BOOL CreateDestinationDirectoryAndRemoveObsoleteFiles(void) {
    return [OpenInController createDestinationDirectoryAndRemoveObsoleteFiles];
  }

  void OnDestinationDirectoryCreated(BOOL success) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    if (!success)
      [owner_ hideOpenInToolbar];
    else
      [owner_ startDownload];
  }

  void OnOwnerDisabled() {
    // When the owner is disabled:
    // - if there is a task in flight posted via |PostTaskAndReplyWithResult|
    // then dereferencing |bridge_| will not release it as |bridge_| is also
    // referenced by the task posting; setting |owner_| to nil makes sure that
    // no methods are called on it, and it works since |owner_| is only used on
    // the main thread.
    // - if there is a task in flight posted by the URLFetcher then
    // |OpenInController| destroys the fetcher and cancels the callback. This is
    // why |OnURLFetchComplete| will neved be called after |owner_| is disabled.
    owner_ = nil;
  }

 protected:
  friend base::RefCountedThreadSafe<OpenInControllerBridge>;
  ~OpenInControllerBridge() override {}

 private:
  __weak OpenInController* owner_;
};

@implementation OpenInController {
  // Bridge from C++ to Obj-C class.
  scoped_refptr<OpenInControllerBridge> bridge_;

  // URL of the document.
  GURL documentURL_;

  // Controller for opening documents in other applications.
  UIDocumentInteractionController* documentController_;

  // Toolbar overlay to be displayed on tap.
  OpenInToolbar* openInToolbar_;

  // Timer used to automatically hide the |openInToolbar_| after a period.
  NSTimer* openInTimer_;

  // Gesture recognizer to catch taps on the document.
  UITapGestureRecognizer* tapRecognizer_;

  // Suggested filename for the document.
  NSString* suggestedFilename_;

  // Fetcher used to redownload the document and save it in the sandbox.
  std::unique_ptr<net::URLFetcher> fetcher_;

  // CRWWebController used to check if the tap is not on a link and the
  // |openInToolbar_| should be displayed.
  CRWWebController* webController_;

  // URLRequestContextGetter needed for the URLFetcher.
  scoped_refptr<net::URLRequestContextGetter> requestContext_;

  // Spinner view displayed while the file is downloading.
  UIView* overlayedView_;

  // The location where the "Open in..." menu is anchored.
  CGRect anchorLocation_;

  // YES if the file download was canceled.
  BOOL downloadCanceled_;

  // YES if the OpenIn menu is displayed.
  BOOL isOpenInMenuDisplayed_;

  // YES if the toolbar is displayed.
  BOOL isOpenInToolbarDisplayed_;

  // Task runner on which file operations should happen.
  scoped_refptr<base::SequencedTaskRunner> sequencedTaskRunner_;
}

- (id)initWithRequestContext:(net::URLRequestContextGetter*)requestContext
               webController:(CRWWebController*)webController {
  self = [super init];
  if (self) {
    requestContext_ = requestContext;
    webController_ = webController;
    tapRecognizer_ = [[UITapGestureRecognizer alloc]
        initWithTarget:self
                action:@selector(handleTapFrom:)];
    [tapRecognizer_ setDelegate:self];
    sequencedTaskRunner_ = base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BACKGROUND});
    isOpenInMenuDisplayed_ = NO;
  }
  return self;
}

- (void)enableWithDocumentURL:(const GURL&)documentURL
            suggestedFilename:(NSString*)suggestedFilename {
  documentURL_ = GURL(documentURL);
  suggestedFilename_ = suggestedFilename;
  [webController_ addGestureRecognizerToWebView:tapRecognizer_];
  [self openInToolbar].alpha = 0.0f;
  [webController_ addToolbarViewToWebView:[self openInToolbar]];
  [self showOpenInToolbar];
}

- (void)disable {
  [self openInToolbar].alpha = 0.0f;
  [openInTimer_ invalidate];
  if (bridge_.get())
    bridge_->OnOwnerDisabled();
  bridge_ = nil;
  [webController_ removeGestureRecognizerFromWebView:tapRecognizer_];
  [webController_ removeToolbarViewFromWebView:[self openInToolbar]];
  [documentController_ dismissMenuAnimated:NO];
  [documentController_ setDelegate:nil];
  documentURL_ = GURL();
  suggestedFilename_ = nil;
  fetcher_.reset();
}

- (void)detachFromWebController {
  [self disable];
  // Animation blocks may be keeping this object alive; don't extend the
  // lifetime of CRWWebController.
  webController_ = nil;
}

- (void)dealloc {
  [self disable];
}

- (void)handleTapFrom:(UIGestureRecognizer*)gestureRecognizer {
  if ([gestureRecognizer state] == UIGestureRecognizerStateEnded) {
    if (isOpenInToolbarDisplayed_) {
      [self hideOpenInToolbar];
    } else {
      [self showOpenInToolbar];
    }
  }
}

- (void)showOpenInToolbar {
  if ([openInTimer_ isValid]) {
    [openInTimer_ setFireDate:([NSDate dateWithTimeIntervalSinceNow:
                                           kOpenInToolbarDisplayDuration])];
  } else {
    openInTimer_ =
        [NSTimer scheduledTimerWithTimeInterval:kOpenInToolbarDisplayDuration
                                         target:self
                                       selector:@selector(hideOpenInToolbar)
                                       userInfo:nil
                                        repeats:NO];
    UIView* openInToolbar = [self openInToolbar];
    [UIView animateWithDuration:kOpenInToolbarAnimationDuration
                     animations:^{
                       [openInToolbar setAlpha:1.0];
                     }];
  }
  isOpenInToolbarDisplayed_ = YES;
}

- (void)hideOpenInToolbar {
  if (!openInToolbar_)
    return;
  [openInTimer_ invalidate];
  UIView* openInToolbar = [self openInToolbar];
  [UIView animateWithDuration:kOpenInToolbarAnimationDuration
                   animations:^{
                     [openInToolbar setAlpha:0.0];
                   }];
  isOpenInToolbarDisplayed_ = NO;
}

- (void)exportFileWithOpenInMenuAnchoredAt:(UIView*)view {
  DCHECK([view isKindOfClass:[UIView class]]);
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if (!webController_)
    return;

  anchorLocation_ = [[self openInToolbar] convertRect:view.frame
                                               toView:[webController_ view]];
  [openInTimer_ invalidate];
  if (!bridge_.get())
    bridge_ = new OpenInControllerBridge(self);

  // This needs to be done in two steps, on two separate threads. The
  // first task needs to be done on the worker pool and returns a BOOL which is
  // then used in the second function, |OnDestinationDirectoryCreated|, which
  // runs on the UI thread.
  base::Callback<BOOL(void)> task = base::Bind(
      &OpenInControllerBridge::CreateDestinationDirectoryAndRemoveObsoleteFiles,
      bridge_);
  base::Callback<void(BOOL)> reply = base::Bind(
      &OpenInControllerBridge::OnDestinationDirectoryCreated, bridge_);
  base::PostTaskAndReplyWithResult(sequencedTaskRunner_.get(), FROM_HERE, task,
                                   reply);
}

- (void)startDownload {
  NSString* tempDirPath = [NSTemporaryDirectory()
      stringByAppendingPathComponent:kDocumentsTempPath];
  NSString* filePath =
      [tempDirPath stringByAppendingPathComponent:suggestedFilename_];

  // In iPad the toolbar has to be displayed to anchor the "Open in" menu.
  if (!IsIPadIdiom())
    [self hideOpenInToolbar];

  // Show an overlayed view to indicate a download is in progress. On tap this
  // view can be dismissed and the download canceled.
  [self showDownloadOverlayView];
  downloadCanceled_ = NO;

  // Ensure |bridge_| is set in case this function is called from a unittest.
  if (!bridge_.get())
    bridge_ = new OpenInControllerBridge(self);

  // Download the document and save it at |filePath|.
  fetcher_ = net::URLFetcher::Create(0, documentURL_, net::URLFetcher::GET,
                                     bridge_.get());
  fetcher_->SetRequestContext(requestContext_.get());
  fetcher_->SetLoadFlags(net::LOAD_SKIP_CACHE_VALIDATION);
  fetcher_->SaveResponseToFileAtPath(
      base::FilePath(base::SysNSStringToUTF8(filePath)), sequencedTaskRunner_);
  fetcher_->Start();
}

- (void)handleTapOnOverlayedView:(UIGestureRecognizer*)gestureRecognizer {
  if ([gestureRecognizer state] != UIGestureRecognizerStateEnded)
    return;

  [self removeOverlayedView];
  if (IsIPadIdiom())
    [self hideOpenInToolbar];
  downloadCanceled_ = YES;
}

- (void)removeOverlayedView {
  if (!overlayedView_)
    return;

  UIView* overlayedView = overlayedView_;
  [UIView animateWithDuration:kOverlayViewAnimationDuration
      animations:^{
        [overlayedView setAlpha:0.0];
      }
      completion:^(BOOL finished) {
        [overlayedView removeFromSuperview];
      }];
  overlayedView_ = nil;
}

- (void)showErrorWithMessage:(NSString*)message {
  UIViewController* topViewController =
      [[[UIApplication sharedApplication] keyWindow] rootViewController];

  _alertCoordinator =
      [[AlertCoordinator alloc] initWithBaseViewController:topViewController
                                                     title:nil
                                                   message:message];

  [_alertCoordinator addItemWithTitle:l10n_util::GetNSString(IDS_OK)
                               action:nil
                                style:UIAlertActionStyleDefault];

  [_alertCoordinator start];
}

- (void)presentOpenInMenuForFileAtURL:(NSURL*)fileURL {
  if (!webController_)
    return;

  if (requestContext_.get()) {
    // |requestContext_| is nil only if this is called from a unit test, in
    // which case the |documentController_| was set already.
    documentController_ =
        [UIDocumentInteractionController interactionControllerWithURL:fileURL];
  }

  // TODO(cgrigoruta): The UTI is hardcoded for now, change this when we add
  // support for other file types as well.
  [documentController_ setUTI:@"com.adobe.pdf"];
  [documentController_ setDelegate:self];
  BOOL success =
      [documentController_ presentOpenInMenuFromRect:anchorLocation_
                                              inView:[webController_ view]
                                            animated:YES];
  if (requestContext_.get()) {
    [self removeOverlayedView];
    if (!success) {
      if (IsIPadIdiom())
        [self hideOpenInToolbar];
      NSString* errorMessage =
          l10n_util::GetNSStringWithFixup(IDS_IOS_OPEN_IN_NO_APPS_REGISTERED);
      [self showErrorWithMessage:errorMessage];
    } else {
      isOpenInMenuDisplayed_ = YES;
    }
  }
}

- (void)showDownloadOverlayView {
  UIViewController* topViewController =
      [[[UIApplication sharedApplication] keyWindow] rootViewController];
  UIView* topView = topViewController.view;
  overlayedView_ = [[UIView alloc] initWithFrame:[topView bounds]];
  [overlayedView_ setAutoresizingMask:(UIViewAutoresizingFlexibleWidth |
                                       UIViewAutoresizingFlexibleHeight)];
  UIView* grayBackgroundView =
      [[UIView alloc] initWithFrame:[overlayedView_ frame]];
  [grayBackgroundView setBackgroundColor:[UIColor darkGrayColor]];
  [grayBackgroundView setAlpha:kOverlayedViewBackgroundAlpha];
  [grayBackgroundView setAutoresizingMask:(UIViewAutoresizingFlexibleWidth |
                                           UIViewAutoresizingFlexibleHeight)];
  [overlayedView_ addSubview:grayBackgroundView];

  UIActivityIndicatorView* spinner = [[UIActivityIndicatorView alloc]
      initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
  [spinner setFrame:[overlayedView_ frame]];
  [spinner setHidesWhenStopped:YES];
  [spinner setUserInteractionEnabled:NO];
  [spinner startAnimating];
  [spinner setAutoresizingMask:(UIViewAutoresizingFlexibleWidth |
                                UIViewAutoresizingFlexibleHeight)];
  [overlayedView_ addSubview:spinner];

  UILabel* label = [[UILabel alloc] init];
  [label setTextColor:[UIColor whiteColor]];
  [label setFont:GetUIFont(FONT_HELVETICA, true, kLabelTextSize)];
  [label setNumberOfLines:0];
  [label setShadowColor:[UIColor blackColor]];
  [label setShadowOffset:CGSizeMake(0.0, 1.0)];
  [label setBackgroundColor:[UIColor clearColor]];
  [label setText:l10n_util::GetNSString(IDS_IOS_OPEN_IN_FILE_DOWNLOAD_CANCEL)];
  [label setLineBreakMode:NSLineBreakByWordWrapping];
  [label setTextAlignment:NSTextAlignmentCenter];
  CGFloat labelWidth =
      [overlayedView_ frame].size.width * kOverlayedViewLabelWidthPercentage;
  CGFloat originX = ([overlayedView_ frame].size.width - labelWidth) / 2;

  CGFloat labelHeight =
      [[label text] cr_boundingSizeWithSize:CGSizeMake(labelWidth, CGFLOAT_MAX)
                                       font:[label font]]
          .height;
  CGFloat originY =
      [overlayedView_ center].y - labelHeight - kOverlayedViewLabelBottomMargin;
  [label setFrame:CGRectMake(originX, originY, labelWidth, labelHeight)];
  [overlayedView_ addSubview:label];

  UITapGestureRecognizer* tapRecognizer = [[UITapGestureRecognizer alloc]
      initWithTarget:self
              action:@selector(handleTapOnOverlayedView:)];
  [tapRecognizer setDelegate:self];
  [overlayedView_ addGestureRecognizer:tapRecognizer];

  [overlayedView_ setAlpha:0.0];
  [topView addSubview:overlayedView_];
  UIView* overlayedView = overlayedView_;
  [UIView animateWithDuration:kOverlayViewAnimationDuration
                   animations:^{
                     [overlayedView setAlpha:1.0];
                   }];
}

- (UIView*)openInToolbar {
  if (!openInToolbar_) {
    openInToolbar_ = [[OpenInToolbar alloc]
        initWithTarget:self
                action:@selector(exportFileWithOpenInMenuAnchoredAt:)];
  }
  return openInToolbar_;
}

#pragma mark -
#pragma mark File management

- (void)removeDocumentAtPath:(NSString*)path {
  base::AssertBlockingAllowed();
  NSFileManager* fileManager = [NSFileManager defaultManager];
  NSError* error = nil;
  if (![fileManager removeItemAtPath:path error:&error]) {
    DLOG(ERROR) << "Failed to remove file: "
                << base::SysNSStringToUTF8([error description]);
  }
}

+ (void)removeAllStoredDocumentsAtPath:(NSString*)tempDirPath {
  base::AssertBlockingAllowed();
  NSFileManager* fileManager = [NSFileManager defaultManager];
  NSError* error = nil;
  NSArray* documentFiles =
      [fileManager contentsOfDirectoryAtPath:tempDirPath error:&error];
  if (!documentFiles) {
    DLOG(ERROR) << "Failed to get content of directory at path: "
                << base::SysNSStringToUTF8([error description]);
    return;
  }

  for (NSString* filename in documentFiles) {
    NSString* filePath = [tempDirPath stringByAppendingPathComponent:filename];
    if (![fileManager removeItemAtPath:filePath error:&error]) {
      DLOG(ERROR) << "Failed to remove file: "
                  << base::SysNSStringToUTF8([error description]);
    }
  }
}

+ (BOOL)createDestinationDirectoryAndRemoveObsoleteFiles {
  base::AssertBlockingAllowed();
  NSString* tempDirPath = [NSTemporaryDirectory()
      stringByAppendingPathComponent:kDocumentsTempPath];
  NSFileManager* fileManager = [NSFileManager defaultManager];
  BOOL isDirectory;
  NSError* error = nil;
  if (![fileManager fileExistsAtPath:tempDirPath isDirectory:&isDirectory]) {
    BOOL created = [fileManager createDirectoryAtPath:tempDirPath
                          withIntermediateDirectories:YES
                                           attributes:nil
                                                error:&error];
    DCHECK(created);
    if (!created) {
      DLOG(ERROR) << "Error creating destination dir: "
                  << base::SysNSStringToUTF8([error description]);
      return NO;
    }
  } else {
    DCHECK(isDirectory);
    if (!isDirectory) {
      DLOG(ERROR) << "Destination Directory already exists and is a file.";
      return NO;
    }
    // Remove all documents that might be still on temporary storage.
    [self removeAllStoredDocumentsAtPath:(NSString*)tempDirPath];
  }
  return YES;
}

#pragma mark -
#pragma mark URLFetcher delegate method

- (void)urlFetchDidComplete:(const net::URLFetcher*)fetcher {
  DCHECK(fetcher);
  if (requestContext_.get())
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
  base::FilePath filePath;
  if (fetcher->GetResponseCode() == kHTTPResponseCodeSucceeded &&
      fetcher->GetResponseAsFilePath(true, &filePath)) {
    NSURL* fileURL =
        [NSURL fileURLWithPath:base::SysUTF8ToNSString(filePath.value())];
    if (downloadCanceled_) {
      sequencedTaskRunner_->PostTask(FROM_HERE, base::BindBlockArc(^{
                                       [self
                                           removeDocumentAtPath:[fileURL path]];
                                     }));
    } else {
      [self presentOpenInMenuForFileAtURL:fileURL];
    }
  } else if (!downloadCanceled_) {
    if (IsIPadIdiom())
      [self hideOpenInToolbar];
    [self removeOverlayedView];
    [self showErrorWithMessage:l10n_util::GetNSStringWithFixup(
                                   IDS_IOS_OPEN_IN_FILE_DOWNLOAD_FAILED)];
  }
}

#pragma mark -
#pragma mark UIDocumentInteractionControllerDelegate Methods

- (void)documentInteractionController:(UIDocumentInteractionController*)contr
           didEndSendingToApplication:(NSString*)application {
  sequencedTaskRunner_->PostTask(FROM_HERE, base::BindBlockArc(^{
                                   [self
                                       removeDocumentAtPath:[[contr URL] path]];
                                 }));
  if (IsIPadIdiom()) {
    // Call the |documentInteractionControllerDidDismissOpenInMenu:| method
    // as this is not called on the iPad after the document has been opened
    // in another application.
    [self documentInteractionControllerDidDismissOpenInMenu:contr];
  }
}

- (void)documentInteractionControllerDidDismissOpenInMenu:
    (UIDocumentInteractionController*)controller {
  if (!IsIPadIdiom()) {
    isOpenInMenuDisplayed_ = NO;
    // On the iPhone the |openInToolber_| is hidden already.
    return;
  }

  // On iPad this method is called whenever the device changes orientation,
  // even thought the OpenIn menu is not displayed. To distinguish the cases
  // when this method is called after the OpenIn menu is dismissed, we
  // check the BOOL |isOpenInMenuDisplayed|.
  if (isOpenInMenuDisplayed_) {
    openInTimer_ =
        [NSTimer scheduledTimerWithTimeInterval:kOpenInToolbarDisplayDuration
                                         target:self
                                       selector:@selector(hideOpenInToolbar)
                                       userInfo:nil
                                        repeats:NO];
  }
  isOpenInMenuDisplayed_ = NO;
}

#pragma mark -
#pragma mark UIGestureRecognizerDelegate Methods

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer*)gestureRecognizer {
  if ([gestureRecognizer.view isEqual:overlayedView_])
    return YES;

  CGPoint location = [gestureRecognizer locationInView:[self openInToolbar]];
  return ![[self openInToolbar] pointInside:location withEvent:nil];
}

#pragma mark - TestingAditions

- (void)setDocumentInteractionController:
    (UIDocumentInteractionController*)controller {
  documentController_ = controller;
}

- (NSString*)suggestedFilename {
  return suggestedFilename_;
}

@end
