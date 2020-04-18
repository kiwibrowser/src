// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/legacy_download_manager_controller.h"

#include <stdint.h>
#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/mac/bind_objc_block.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/download/download_directory_util.h"
#import "ios/chrome/browser/installation_notifier.h"
#import "ios/chrome/browser/store_kit/store_kit_tab_helper.h"
#import "ios/chrome/browser/ui/alert_coordinator/alert_coordinator.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/chrome/browser/ui/network_activity_indicator_manager.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/ActivityIndicator/src/MaterialActivityIndicator.h"
#import "ios/third_party/material_components_ios/src/components/Buttons/src/MaterialButtons.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/web_state/web_state.h"
#include "ios/web/public/web_thread.h"
#include "net/base/filename_util.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "ui/base/l10n/l10n_util_mac.h"
#import "ui/gfx/ios/uikit_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;
using net::HttpResponseHeaders;
using net::URLFetcher;
using net::URLFetcherDelegate;
using net::URLRequestContextGetter;
using net::URLRequestStatus;

@interface LegacyDownloadManagerController ()

// Creates the portrait and landscape mode constraints that are switched every
// time the interface is rotated.
- (void)initConstraints;

// Runs every time the interface switches between portrait and landscape mode,
// and applies the appropriate UI constraints for |orientation|.
- (void)updateConstraints:(UIInterfaceOrientation)orientation;

// Modifies (only) the constraints that anchor the action bar in portrait
// mode (there are different constraints for when the Google Drive
// button is showing versus not showing). This should only be called when
// the interface orientation is portrait.
- (void)updatePortraitActionBarConstraints;

// Removes all constraints that position the action bar in portrait mode.
- (void)removePortraitActionBarConstraints;

// Adds in the correct constraints to position the action bar in portrait mode,
// based on whether the Google Drive button is showing or not. This should only
// be called when the interface orientation is portrait.
- (void)addPortraitActionBarConstraints;

// Updates the file type label's vertical constraint constant to position the
// file type label depending on the device orientation and whether or not the
// error icon is displayed.
- (void)updateFileTypeLabelConstraint;

// Called every time the device orientation is about to change.
- (void)interfaceOrientationWillChangeNotification:
    (NSNotification*)notification;

// Called every time the device orientation has changed.
- (void)deviceOrientationDidChangeNotification:(NSNotification*)notification;

// If |down| is YES, animates the file type label down. Otherwise animates
// the file type label up. Runs |completion| at the end of the animation.
- (void)animateFileTypeLabelDown:(BOOL)down
                  withCompletion:(void (^)(BOOL))completion;

// Initializes the URL fetcher to make a HEAD request to get information about
// the file to download and starts the network activity indicator.
- (void)startHeadFetch;

// Called when the HEAD request for the downloaded file returns and there is
// information about the file to display (size, type, etc.).
- (void)onHeadFetchComplete;

// Creates the activity indicator to animate while download a file with an
// unknown file size.
- (void)initializeActivityIndicator;

// Displays a dialog informing the user that no application on the device can
// open the file. The dialog contains a button for downloading Google Drive.
- (void)displayUnableToOpenFileDialog;

// Displays the error UI to the user, notifying them that their download
// failed. This method only ensures that the file type label is in the right
// place, then calls |finishDisplayingError| to do all the other work.
- (void)displayError;

// Updates the views to display the error UI.
- (void)finishDisplayingError;

// Handles clicks on the download button - also the only way to get out of the
// error flow.
- (IBAction)downloadButtonTapped:(id)sender;

// Undoes all the changes to the UI caused by calling |displayError|, and
// then calls |finishDownloadButtonTapped|.
- (void)hideError;

// In practice this will almost always call |beginStartingContentDownload|, but
// it will call |startHeadFetch| if the head fetch never completed successfully
// and led to the error flow.
- (void)finishDownloadButtonTapped;

// Begins the work start a download, including creating the downloads directory
- (void)beginStartingContentDownload;

// Called after an attempt to create the downloads directory on another
// thread has completed. |directoryCreated| is true if the creation was
// successful, or false otherwise.
- (void)finishStartingContentDownload:(bool)directoryCreated;

// Called when another chunk of the file has successfully been downloaded.
// |bytesDownloaded| gives the number of total bytes downloaded so far.
- (void)onContentFetchProgress:(long long)bytesDownloaded;

// Changes the height of the progress bar to be |self.fractionDownloaded| the
// height of the document icon, and adjusts its y-coordinate so its bottom
// always aligns with the bottom of the document icon. If |withAnimation| is
// YES, it will animate the change (as long as the change is > 1 point). If
// |withCompletionAnimation| is YES, the file download complete animation will
// run once the progress bar has finished being set.
- (void)setProgressBarHeightWithAnimation:(BOOL)animated
                  withCompletionAnimation:(BOOL)completionAnimation;

// Sets title from |messageId| on |button| label for both Normal and Highlighted
// states. Material design uses all caps for button labels.
- (void)setTitle:(int)messageId forButton:(UIButton*)button;

// Makes the file icon "pop" and fades the image in the fold icon to the
// completed fold icon. This will also call -showGoogleDriveButton if the user
// doesn't have Google Drive installed on their device.
- (void)runDownloadCompleteAnimation;

// Handles clicks on the cancel button.
- (IBAction)cancelButtonTapped:(id)sender;

// Sets the time left label to the given text. If |animated| is YES, it will
// animate the text change.
- (void)setTimeLeft:(NSString*)text withAnimation:(BOOL)animated;

// Called when the request for downloading the file has completed (successfully
// or not).
- (void)onContentFetchComplete;

// Animates the Google Drive button onto the screen after |delay| time has
// passed.
- (void)showGoogleDriveButton;

// Animates the Google Drive button off the screen.
- (void)hideGoogleDriveButton;

// Handles clicks on the open in button.
- (IBAction)openInButtonTapped:(id)sender;

// Opens a view controller that allows the user to install Google Drive.
- (void)openGoogleDriveInAppStore;

// Calls -openGoogleDriveInAppStore to allow the user to install Google Drive.
- (IBAction)googleDriveButtonTapped:(id)sender;

// Cleans up this LegacyDownloadManagerController, and deletes its file from the
// downloads directory if it has been created there.
- (void)dealloc;

// Returns an NSString* unique to |self|, to use with the
// NetworkActivityIndicatorManager.
- (NSString*)getNetworkActivityKey;

@end

namespace {

NSString* kGoogleDriveAppStoreId = @"507874739";
NSString* kGoogleDriveURL = @"googledrive://";
NSString* kGoogleDriveBundleId = @"com.google.Drive";

// Key of the UMA Download.IOSDownloadFileResult histogram.
const char* const kUMADownloadFileResult = "Download.IOSDownloadFileResult";
// Values of the UMA Download.IOSDownloadFileResult histogram.
enum DownloadFileResult {
  DOWNLOAD_COMPLETED,
  DOWNLOAD_CANCELED,
  DOWNLOAD_FAILURE,
  DOWNLOAD_OTHER,
  DOWNLOAD_FILE_RESULT_COUNT
};

// Key of the UMA Download.IOSDownloadedFileAction histogram.
const char* const kUMADownloadedFileAction = "Download.IOSDownloadedFileAction";
// Values of the UMA Download.IOSDownloadedFileAction histogram.
enum DownloadedFileAction {
  OPENED_IN_DRIVE,
  OPENED_IN_OTHER_APP,
  NO_ACTION,
  DOWNLOADED_FILE_ACTION_COUNT
};

// Key of the UMA Download.IOSDownloadedFileStatusCode histogram.
const char kUMADownloadedFileStatusCode[] =
    "Download.IOSDownloadedFileStatusCode";

// Key of the UMA.Download.IOSDownloadedFileNetError histogram.
const char kUMADownloadFileNetError[] = "Download.IOSDownloadedFileNetError";

int g_download_manager_id = 0;

const int kNoFileSizeGiven = -1;

const NSTimeInterval kFileTypeLabelAnimationDuration = 0.5;
const NSTimeInterval kErrorAnimationDuration = 0.5;

const NSTimeInterval kProgressBarAnimationDuration = 1.0;

const NSTimeInterval kDownloadCompleteAnimationDuration = 0.4;
NSString* const kDocumentPopAnimationKey = @"DocumentPopAnimationKey";

const NSTimeInterval kTimeLeftAnimationDuration = 0.25;
NSString* const kTimeLeftAnimationKey = @"TimeLeftAnimationKey";

const NSTimeInterval kGoogleDriveButtonAnimationDuration = 0.5;

const int kUndownloadedDocumentColor = 0x7BAAF8;
const int kDownloadedDocumentColor = 0x4285F5;
const int kErrorDocumentColor = 0xDB4437;
const int kIndeterminateFileSizeColor = 0xEAEAEA;

const CGFloat kFileTypeLabelWhiteAlpha = 0.87;
const CGFloat kFileTypeLabelBlackAlpha = 0.56;

const CGFloat kDocumentContainerWidthPortrait = 136;
const CGFloat kDocumentContainerHeightPortrait = 180;
const CGFloat kDocumentContainerCenterYOneButtonOffsetPortrait = -34;
const CGFloat kDocumentContainerCenterYTwoButtonOffsetPortrait = -52;

const CGFloat kDocumentContainerWidthLandscape = 82;
const CGFloat kDocumentContainerHeightLandscape = 109;
const CGFloat kDocumentContainerCenterYOffsetLandscape = -48;

const CGFloat kFoldIconWidthPortrait = 56;
const CGFloat kFoldIconHeightPortrait = 106;

const CGFloat kFoldIconWidthLandscape = 34;
const CGFloat kFoldIconHeightLandscape = 64;

const CGFloat kErrorIconWidthPortrait = 40;
const CGFloat kErrorIconHeightPortrait = 40;

const CGFloat kErrorIconWidthLandscape = 20;
const CGFloat kErrorIconHeightLandscape = 20;

const CGFloat kFileTypeLabelWidthDeltaPortrait = -24;
const CGFloat kFileTypeLabelHeightPortrait = 42;

const CGFloat kFileTypeLabelWidthDeltaLandscape = -16;
const CGFloat kFileTypeLabelHeightLandscape = 24;

const CGFloat kFileTypeLabelVerticalOffset = 10;
const CGFloat kFileTypeLabelAnimationDeltaYPortrait = 44;
const CGFloat kFileTypeLabelAnimationDeltaYLandscape = 27;

const CGFloat kTimeLeftLabelWidthDeltaPortrait = -24;
const CGFloat kTimeLeftLabelHeightPortrait = 16;
const CGFloat kTimeLeftLabelBottomSpacingOffsetPortrait = -12;

const CGFloat kTimeLeftLabelWidthDeltaLandscape = -16;
const CGFloat kTimeLeftLabelHeightLandscape = 14;
const CGFloat kTimeLeftLabelBottomSpacingOffsetLandscape = -8;

const CGFloat kFileNameLabelHeightPortrait = 20;
const CGFloat kFileNameLabelTopSpacingOffsetPortrait = 12;

const CGFloat kFileNameLabelHeightLandscape = 16;
const CGFloat kFileNameLabelTopSpacingOffsetLandscape = 8;

const CGFloat kErrorOrSizeLabelHeightPortrait = 16;
const CGFloat kErrorOrSizeLabelHeightLandscape = 14;

const CGFloat kActionBarHeightOneButtonPortrait = 68;
const CGFloat kActionBarHeightTwoButtonPortrait = 104;

const CGFloat kActionBarHeightLandscape = 52;

const CGFloat kActionBarBorderHeight = 0.5;

const CGFloat kActionBarButtonTopSpacingOffsetPortrait = 16;

const CGFloat kActionBarButtonTopSpacingOffsetLandscape = 8;

// This value is the same in portrait and landscape.
const CGFloat kActionBarButtonTrailingSpacingOffset = -16;

const CGFloat kActivityIndicatorWidth = 30;

// URLFetcher delegate that bridges from C++ to an Obj-C class for the HEAD
// request to get information about the file.
class DownloadHeadDelegate : public URLFetcherDelegate {
 public:
  explicit DownloadHeadDelegate(LegacyDownloadManagerController* owner)
      : owner_(owner) {}
  void OnURLFetchComplete(const URLFetcher* source) override {
    [owner_ onHeadFetchComplete];
  };

 private:
  __weak LegacyDownloadManagerController* owner_;
  DISALLOW_COPY_AND_ASSIGN(DownloadHeadDelegate);
};

// URLFetcher delegate that bridges from C++ to this Obj-C class for the
// request to download the contents of the file.
class DownloadContentDelegate : public URLFetcherDelegate {
 public:
  explicit DownloadContentDelegate(LegacyDownloadManagerController* owner)
      : owner_(owner) {}
  void OnURLFetchDownloadProgress(const URLFetcher* source,
                                  int64_t current,
                                  int64_t total,
                                  int64_t current_network_bytes) override {
    [owner_ onContentFetchProgress:current];
  }
  void OnURLFetchComplete(const URLFetcher* source) override {
    [owner_ onContentFetchComplete];
  };

 private:
  __weak LegacyDownloadManagerController* owner_;
  DISALLOW_COPY_AND_ASSIGN(DownloadContentDelegate);
};

}  // namespace

@interface LegacyDownloadManagerController () {
  int _downloadManagerId;

  // Coordinator for displaying the alert informing the user that no application
  // on the device can open the file.
  AlertCoordinator* _alertCoordinator;

  // The size of the file to be downloaded, as determined by the Content-Length
  // header field in the initial HEAD request. This is set to |kNoFileSizeGiven|
  // if the Content-Length header is not given.
  long long _totalFileSize;

  // YES if |_fileTypeLabel| is vertically centered in |_documentContainer|. NO
  // if |_fileTypeLabel| is lower to account for another view in
  // |_documentContainer|.
  BOOL _isFileTypeLabelCentered;
  BOOL _isDisplayingError;
  BOOL _didSuccessfullyFinishHeadFetch;
  // WebState provides access to the *TabHelper objects.
  web::WebState* _webState;
  std::unique_ptr<URLFetcher> _fetcher;
  std::unique_ptr<DownloadHeadDelegate> _headFetcherDelegate;
  std::unique_ptr<DownloadContentDelegate> _contentFetcherDelegate;
  base::FilePath _downloadFilePath;
  MDCActivityIndicator* _activityIndicator;
  // Set to YES when a download begins and is used to determine if the
  // DownloadFileResult histogram needs to be recorded on -dealloc.
  BOOL _recordDownloadResultHistogram;
  // Set to YES when a file is downloaded and is used to determine if the
  // DownloadedFileAction histogram needs to be recorded on -dealloc.
  BOOL _recordFileActionHistogram;
}

// The container that holds the |documentIcon|, the |progressBar|, the
// |foldIcon|, the |fileTypeLabel|, and the |timeLeftLabel|.
@property(nonatomic, strong) IBOutlet UIView* documentContainer;

// The progress bar that displays download progress.
@property(nonatomic, strong) IBOutlet UIView* progressBar;

// The image of the document.
@property(nonatomic, strong) IBOutlet UIImageView* documentIcon;

// The image of the document fold.
@property(nonatomic, strong) IBOutlet UIImageView* foldIcon;

// The error image displayed inside the document.
@property(nonatomic, strong) IBOutlet UIImageView* errorIcon;

// The label that displays the file type of the file to be downloaded.
@property(nonatomic, strong) IBOutlet UILabel* fileTypeLabel;

// The label that displays the estimate of how much time is still needed to
// finish the file download.
@property(nonatomic, strong) IBOutlet UILabel* timeLeftLabel;

// The label that displays the name of the file to be downloaded, as it will
// be saved on the user's device.
@property(nonatomic, strong) IBOutlet UILabel* fileNameLabel;

// The label that displays the size of the file to be downloaded or the error
// message.
@property(nonatomic, strong) IBOutlet UILabel* errorOrSizeLabel;

// The label that displays error messages when errors occur.
@property(nonatomic, strong) IBOutlet UILabel* errorLabel;

// The container that holds the |downloadButton|, |cancelButton|,
// |openInButton|, and |googleDriveButton|.
@property(nonatomic, strong) IBOutlet UIView* actionBar;

// View that appears at the top of the action bar and acts as a border.
@property(nonatomic, strong) IBOutlet UIView* actionBarBorder;

// The button which starts the file download.
@property(nonatomic, strong) IBOutlet MDCButton* downloadButton;

// The button which switches with the |downloadButton| during a download.
// Pressing it cancels the download.
@property(nonatomic, strong) IBOutlet MDCButton* cancelButton;

// The button that switches with the |cancelButton| when a file download
// completes. Pressing it opens the UIDocumentInteractionController, letting
// the user select another app in which to open the downloaded file.
@property(nonatomic, strong) IBOutlet MDCButton* openInButton;

// The button that opens a view controller to allow the user to install
// Google Drive.
@property(nonatomic, strong) IBOutlet MDCButton* googleDriveButton;

// The controller that displays the list of other apps that the downloaded file
// can be opened in.
@property(nonatomic, strong)
    UIDocumentInteractionController* docInteractionController;

// Contains all the constraints that should be applied only in portrait mode.
@property(nonatomic, strong) NSArray* portraitConstraintsArray;

// Contains all the constraints that should be applied only in landscape mode.
@property(nonatomic, strong) NSArray* landscapeConstraintsArray;

// Contains all the constraints that should be applied only in portrait mode
// when there is only one button showing in the action bar (i.e. the Google
// Drive button is NOT showing).
@property(nonatomic, strong)
    NSArray* portraitActionBarOneButtonConstraintsArray;

// Contains all the constraints that should be applied only in portrait mode
// when there are two buttons showing in the action bar (i.e. the Google Drive
// button IS showing).
@property(nonatomic, strong)
    NSArray* portraitActionBarTwoButtonConstraintsArray;

// Constraint that positions the file type label vertically in the center of the
// document with an additional offset.
@property(nonatomic, strong) NSLayoutConstraint* fileTypeLabelCentered;

// Records the time the download started, to display an estimate of how much
// time is required to finish the download.
@property(nonatomic, strong) NSDate* downloadStartedTime;

// Records the fraction (from 0.0 to 1.0) of the file that has been
// downloaded.
@property(nonatomic) double fractionDownloaded;

// The base view controller from which to present UI.
@property(nonatomic, readonly, weak) UIViewController* baseViewController;

@end

@implementation LegacyDownloadManagerController

@synthesize documentContainer = _documentContainer;
@synthesize progressBar = _progressBar;
@synthesize documentIcon = _documentIcon;
@synthesize foldIcon = _foldIcon;
@synthesize errorIcon = _errorIcon;
@synthesize fileTypeLabel = _fileTypeLabel;
@synthesize timeLeftLabel = _timeLeftLabel;
@synthesize fileNameLabel = _fileNameLabel;
@synthesize errorOrSizeLabel = _errorOrSizeLabel;
@synthesize errorLabel = _errorLabel;
@synthesize actionBar = _actionBar;
@synthesize actionBarBorder = _actionBarBorder;
@synthesize downloadButton = _downloadButton;
@synthesize cancelButton = _cancelButton;
@synthesize openInButton = _openInButton;
@synthesize googleDriveButton = _googleDriveButton;
@synthesize docInteractionController = _docInteractionController;
@synthesize portraitConstraintsArray = _portraitConstraintsArray;
@synthesize landscapeConstraintsArray = _landscapeConstraintsArray;
@synthesize portraitActionBarOneButtonConstraintsArray =
    _portraitActionBarOneButtonConstraintsArray;
@synthesize portraitActionBarTwoButtonConstraintsArray =
    _portraitActionBarTwoButtonConstraintsArray;
@synthesize fileTypeLabelCentered = _fileTypeLabelCentered;
@synthesize downloadStartedTime = _downloadStartedTime;
@synthesize fractionDownloaded = _fractionDownloaded;
@synthesize baseViewController = _baseViewController;

- (instancetype)initWithWebState:(web::WebState*)webState
                     downloadURL:(const GURL&)url
              baseViewController:(UIViewController*)baseViewController {
  self = [super initWithNibName:@"DownloadManagerController" url:url];
  if (self) {
    _downloadManagerId = g_download_manager_id++;

    DCHECK(webState);
    _webState = webState;
    _headFetcherDelegate.reset(new DownloadHeadDelegate(self));
    _contentFetcherDelegate.reset(new DownloadContentDelegate(self));
    _downloadFilePath = base::FilePath();
    _baseViewController = baseViewController;

    [_documentContainer
        setBackgroundColor:UIColorFromRGB(kUndownloadedDocumentColor)];

    _isFileTypeLabelCentered = YES;
    _isDisplayingError = NO;
    _didSuccessfullyFinishHeadFetch = NO;

    [self setTitle:IDS_IOS_DOWNLOAD_MANAGER_DOWNLOAD forButton:_downloadButton];
    [self setTitle:IDS_CANCEL forButton:_cancelButton];
    [self setTitle:IDS_IOS_OPEN_IN forButton:_openInButton];
    [self setTitle:IDS_IOS_DOWNLOAD_MANAGER_INSTALL_GOOGLE_DRIVE
         forButton:_googleDriveButton];

    [_documentContainer setIsAccessibilityElement:YES];
    self.fractionDownloaded = 0;

    // Constraints not in xib:
    self.fileTypeLabelCentered = [_fileTypeLabel.centerYAnchor
        constraintEqualToAnchor:_documentContainer.centerYAnchor
                       constant:kFileTypeLabelVerticalOffset];
    self.fileTypeLabelCentered.active = YES;

    // Action Bar Border
    [_actionBarBorder.heightAnchor
        constraintEqualToConstant:ui::AlignValueToUpperPixel(
                                      kActionBarBorderHeight)]
        .active = YES;

    [self initConstraints];
    UIInterfaceOrientation orientation =
        [[UIApplication sharedApplication] statusBarOrientation];
    [self updateConstraints:orientation];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(interfaceOrientationWillChangeNotification:)
               name:UIApplicationWillChangeStatusBarOrientationNotification
             object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(deviceOrientationDidChangeNotification:)
               name:UIDeviceOrientationDidChangeNotification
             object:nil];
    base::RecordAction(UserMetricsAction("MobileDownloadFileUIShown"));
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:UIApplicationWillChangeStatusBarOrientationNotification
              object:nil];
  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:UIDeviceOrientationDidChangeNotification
              object:nil];
  [[InstallationNotifier sharedInstance] unregisterForNotifications:self];
  [[NetworkActivityIndicatorManager sharedInstance]
      clearNetworkTasksForGroup:[self getNetworkActivityKey]];
  [_docInteractionController setDelegate:nil];
  [_docInteractionController dismissMenuAnimated:NO];
  if (!_downloadFilePath.empty()) {
    // A local copy of _downloadFilePath must be made: the instance variable
    // will be cleaned up during dealloc, but a local copy will be retained by
    // the block and won't be deleted until the block completes.
    base::FilePath downloadPathCopy = _downloadFilePath;
    base::PostTaskWithTraits(FROM_HERE,
                             {base::MayBlock(), base::TaskPriority::BACKGROUND},
                             base::BindBlockArc(^{
                               DeleteFile(downloadPathCopy, false);
                             }));
  }
  if (_recordDownloadResultHistogram) {
    UMA_HISTOGRAM_ENUMERATION(kUMADownloadFileResult, DOWNLOAD_OTHER,
                              DOWNLOAD_FILE_RESULT_COUNT);
  }
  if (_recordFileActionHistogram) {
    UMA_HISTOGRAM_ENUMERATION(kUMADownloadedFileAction, NO_ACTION,
                              DOWNLOADED_FILE_ACTION_COUNT);
  }
}

#pragma mark - Layout constraints

- (void)initConstraints {
  // Document Container
  NSLayoutConstraint* portraitDocumentContainerWidth =
      [_documentContainer.widthAnchor
          constraintEqualToConstant:kDocumentContainerWidthPortrait];
  NSLayoutConstraint* portraitDocumentContainerHeight =
      [_documentContainer.heightAnchor
          constraintEqualToConstant:kDocumentContainerHeightPortrait];

  // This constraint varies in portrait mode depending on whether the action
  // bar is showing one button or two.
  NSLayoutConstraint* portraitDocumentContainerCenterYOneButton =
      [_documentContainer.centerYAnchor
          constraintEqualToAnchor:self.view.centerYAnchor
                         constant:
                             kDocumentContainerCenterYOneButtonOffsetPortrait];
  NSLayoutConstraint* portraitDocumentContainerCenterYTwoButton =
      [_documentContainer.centerYAnchor
          constraintEqualToAnchor:self.view.centerYAnchor
                         constant:
                             kDocumentContainerCenterYTwoButtonOffsetPortrait];

  NSLayoutConstraint* landscapeDocumentContainerWidth =
      [_documentContainer.widthAnchor
          constraintEqualToConstant:kDocumentContainerWidthLandscape];
  NSLayoutConstraint* landscapeDocumentContainerHeight =
      [_documentContainer.heightAnchor
          constraintEqualToConstant:kDocumentContainerHeightLandscape];
  NSLayoutConstraint* landscapeDocumentContainerCenterY =
      [_documentContainer.centerYAnchor
          constraintEqualToAnchor:self.view.centerYAnchor
                         constant:kDocumentContainerCenterYOffsetLandscape];

  // Fold Icon
  NSLayoutConstraint* portraitFoldIconWidth =
      [_foldIcon.widthAnchor constraintEqualToConstant:kFoldIconWidthPortrait];
  NSLayoutConstraint* portraitFoldIconHeight = [_foldIcon.heightAnchor
      constraintEqualToConstant:kFoldIconHeightPortrait];
  NSLayoutConstraint* landscapeFoldIconWidth =
      [_foldIcon.widthAnchor constraintEqualToConstant:kFoldIconWidthLandscape];
  NSLayoutConstraint* landscapeFoldIconHeight = [_foldIcon.heightAnchor
      constraintEqualToConstant:kFoldIconHeightLandscape];

  // Error Icon
  NSLayoutConstraint* portraitErrorIconWidth = [_errorIcon.widthAnchor
      constraintEqualToConstant:kErrorIconWidthPortrait];
  NSLayoutConstraint* portraitErrorIconHeight = [_errorIcon.heightAnchor
      constraintEqualToConstant:kErrorIconHeightPortrait];

  NSLayoutConstraint* landscapeErrorIconWidth = [_errorIcon.widthAnchor
      constraintEqualToConstant:kErrorIconWidthLandscape];
  NSLayoutConstraint* landscapeErrorIconHeight = [_errorIcon.heightAnchor
      constraintEqualToConstant:kErrorIconHeightLandscape];

  // File Type Label
  NSLayoutConstraint* portraitFileTypeLabelWidth = [_fileTypeLabel.widthAnchor
      constraintLessThanOrEqualToAnchor:_documentContainer.widthAnchor
                               constant:kFileTypeLabelWidthDeltaPortrait];
  NSLayoutConstraint* portraitFileTypeLabelHeight = [_fileTypeLabel.heightAnchor
      constraintEqualToConstant:kFileTypeLabelHeightPortrait];

  NSLayoutConstraint* landscapeFileTypeLabelWidth = [_fileTypeLabel.widthAnchor
      constraintLessThanOrEqualToAnchor:_documentContainer.widthAnchor
                               constant:kFileTypeLabelWidthDeltaLandscape];
  NSLayoutConstraint* landscapeFileTypeLabelHeight =
      [_fileTypeLabel.heightAnchor
          constraintEqualToConstant:kFileTypeLabelHeightLandscape];

  // Time Left Label
  NSLayoutConstraint* portraitTimeLeftLabelWidth = [_timeLeftLabel.widthAnchor
      constraintLessThanOrEqualToAnchor:_documentContainer.widthAnchor
                               constant:kTimeLeftLabelWidthDeltaPortrait];
  NSLayoutConstraint* portraitTimeLeftLabelHeight = [_timeLeftLabel.heightAnchor
      constraintEqualToConstant:kTimeLeftLabelHeightPortrait];
  NSLayoutConstraint* portraitTimeLeftLabelBottomSpacing =
      [_timeLeftLabel.bottomAnchor
          constraintEqualToAnchor:_documentContainer.bottomAnchor
                         constant:kTimeLeftLabelBottomSpacingOffsetPortrait];

  NSLayoutConstraint* landscapeTimeLeftLabelWidth = [_timeLeftLabel.widthAnchor
      constraintLessThanOrEqualToAnchor:_documentContainer.widthAnchor
                               constant:kTimeLeftLabelWidthDeltaLandscape];
  NSLayoutConstraint* landscapeTimeLeftLabelHeight =
      [_timeLeftLabel.heightAnchor
          constraintEqualToConstant:kTimeLeftLabelHeightLandscape];
  NSLayoutConstraint* landscapeTimeLeftLabelBottomSpacing =
      [_timeLeftLabel.bottomAnchor
          constraintEqualToAnchor:_documentContainer.bottomAnchor
                         constant:kTimeLeftLabelBottomSpacingOffsetLandscape];

  // File Name Label
  NSLayoutConstraint* portraitFileNameLabelHeight = [_fileNameLabel.heightAnchor
      constraintEqualToConstant:kFileNameLabelHeightPortrait];
  NSLayoutConstraint* portraitFileNameLabelTopSpacing =
      [_fileNameLabel.topAnchor
          constraintEqualToAnchor:_documentContainer.bottomAnchor
                         constant:kFileNameLabelTopSpacingOffsetPortrait];

  NSLayoutConstraint* landscapeFileNameLabelHeight =
      [_fileNameLabel.heightAnchor
          constraintEqualToConstant:kFileNameLabelHeightLandscape];
  NSLayoutConstraint* landscapeFileNameLabelTopSpacing =
      [_fileNameLabel.topAnchor
          constraintEqualToAnchor:_documentContainer.bottomAnchor
                         constant:kFileNameLabelTopSpacingOffsetLandscape];

  // File Size Label
  NSLayoutConstraint* portraitFileSizeLabelHeight =
      [_errorOrSizeLabel.heightAnchor
          constraintEqualToConstant:kErrorOrSizeLabelHeightPortrait];
  NSLayoutConstraint* landscapeFileSizeLabelHeight =
      [_errorOrSizeLabel.heightAnchor
          constraintEqualToConstant:kErrorOrSizeLabelHeightLandscape];

  // Action Bar
  // This constraint varies in portrait mode depending on whether the action
  // bar is showing one button or two.
  NSLayoutConstraint* portraitActionBarHeightOneButton =
      [_actionBar.heightAnchor
          constraintEqualToConstant:kActionBarHeightOneButtonPortrait];
  NSLayoutConstraint* portraitActionBarHeightTwoButton =
      [_actionBar.heightAnchor
          constraintEqualToConstant:kActionBarHeightTwoButtonPortrait];

  NSLayoutConstraint* landscapeActionBarHeight = [_actionBar.heightAnchor
      constraintEqualToConstant:kActionBarHeightLandscape];

  // Download Button
  NSLayoutConstraint* portraitDownloadButtonTopSpacing =
      [_downloadButton.topAnchor
          constraintEqualToAnchor:_actionBar.topAnchor
                         constant:kActionBarButtonTopSpacingOffsetPortrait];
  NSLayoutConstraint* landscapeDownloadButtonTopSpacing =
      [_downloadButton.topAnchor
          constraintEqualToAnchor:_actionBar.topAnchor
                         constant:kActionBarButtonTopSpacingOffsetLandscape];

  // Cancel Button
  NSLayoutConstraint* portraitCancelButtonTopSpacing = [_cancelButton.topAnchor
      constraintEqualToAnchor:_actionBar.topAnchor
                     constant:kActionBarButtonTopSpacingOffsetPortrait];
  NSLayoutConstraint* landscapeCancelButtonTopSpacing = [_cancelButton.topAnchor
      constraintEqualToAnchor:_actionBar.topAnchor
                     constant:kActionBarButtonTopSpacingOffsetLandscape];

  // Open In Button
  NSLayoutConstraint* portraitOpenInButtonTopSpacing = [_openInButton.topAnchor
      constraintEqualToAnchor:_actionBar.topAnchor
                     constant:kActionBarButtonTopSpacingOffsetPortrait];
  NSLayoutConstraint* landscapeOpenInButtonTopSpacing = [_openInButton.topAnchor
      constraintEqualToAnchor:_actionBar.topAnchor
                     constant:kActionBarButtonTopSpacingOffsetLandscape];

  // Google Drive Button
  NSLayoutConstraint* portraitGoogleDriveButtonTopAlignment =
      [_googleDriveButton.topAnchor
          constraintEqualToAnchor:_openInButton.bottomAnchor];
  NSLayoutConstraint* portraitGoogleDriveButtonTrailingSpacing =
      [_googleDriveButton.trailingAnchor
          constraintEqualToAnchor:_actionBar.trailingAnchor
                         constant:kActionBarButtonTrailingSpacingOffset];

  NSLayoutConstraint* landscapeGoogleDriveButtonTopSpacing =
      [_googleDriveButton.topAnchor
          constraintEqualToAnchor:_actionBar.topAnchor
                         constant:kActionBarButtonTopSpacingOffsetLandscape];
  NSLayoutConstraint* landscapeGoogleDriveButtonTrailingAlignment =
      [_googleDriveButton.trailingAnchor
          constraintEqualToAnchor:_openInButton.leadingAnchor];

  self.portraitConstraintsArray = @[
    portraitDocumentContainerWidth, portraitDocumentContainerHeight,
    portraitFoldIconWidth, portraitFoldIconHeight, portraitErrorIconWidth,
    portraitErrorIconHeight, portraitFileTypeLabelWidth,
    portraitFileTypeLabelHeight, portraitTimeLeftLabelWidth,
    portraitTimeLeftLabelHeight, portraitTimeLeftLabelBottomSpacing,
    portraitFileNameLabelHeight, portraitFileNameLabelTopSpacing,
    portraitFileSizeLabelHeight, portraitDownloadButtonTopSpacing,
    portraitCancelButtonTopSpacing, portraitOpenInButtonTopSpacing,
    portraitGoogleDriveButtonTopAlignment,
    portraitGoogleDriveButtonTrailingSpacing
  ];
  self.landscapeConstraintsArray = @[
    landscapeDocumentContainerWidth,
    landscapeDocumentContainerHeight,
    landscapeDocumentContainerCenterY,
    landscapeFoldIconWidth,
    landscapeFoldIconHeight,
    landscapeErrorIconWidth,
    landscapeErrorIconHeight,
    landscapeFileTypeLabelWidth,
    landscapeFileTypeLabelHeight,
    landscapeTimeLeftLabelWidth,
    landscapeTimeLeftLabelHeight,
    landscapeTimeLeftLabelBottomSpacing,
    landscapeFileNameLabelHeight,
    landscapeFileNameLabelTopSpacing,
    landscapeFileSizeLabelHeight,
    landscapeActionBarHeight,
    landscapeDownloadButtonTopSpacing,
    landscapeCancelButtonTopSpacing,
    landscapeOpenInButtonTopSpacing,
    landscapeGoogleDriveButtonTopSpacing,
    landscapeGoogleDriveButtonTrailingAlignment
  ];

  self.portraitActionBarOneButtonConstraintsArray = @[
    portraitDocumentContainerCenterYOneButton, portraitActionBarHeightOneButton
  ];

  self.portraitActionBarTwoButtonConstraintsArray = @[
    portraitDocumentContainerCenterYTwoButton, portraitActionBarHeightTwoButton
  ];
}

- (void)updateConstraints:(UIInterfaceOrientation)orientation {
  [self.view removeConstraints:_portraitConstraintsArray];
  [self.view removeConstraints:_landscapeConstraintsArray];
  [self removePortraitActionBarConstraints];
  if (UIInterfaceOrientationIsPortrait(orientation)) {
    [self.view addConstraints:_portraitConstraintsArray];
    [self addPortraitActionBarConstraints];

    [_fileTypeLabel setFont:[MDCTypography display2Font]];
    [_timeLeftLabel setFont:[MDCTypography body1Font]];
    [_fileNameLabel setFont:[MDCTypography subheadFont]];
    [_errorOrSizeLabel setFont:[MDCTypography captionFont]];
  } else {
    [self.view addConstraints:_landscapeConstraintsArray];

    [_fileTypeLabel setFont:[MDCTypography display1Font]];
    [_timeLeftLabel setFont:[MDCTypography captionFont]];
    [_fileNameLabel setFont:[MDCTypography body1Font]];
    [_errorOrSizeLabel
        setFont:[[MDCTypography fontLoader] regularFontOfSize:10]];
  }
}

- (void)updatePortraitActionBarConstraints {
  [self removePortraitActionBarConstraints];
  [self addPortraitActionBarConstraints];
}

- (void)removePortraitActionBarConstraints {
  [self.view removeConstraints:_portraitActionBarOneButtonConstraintsArray];
  [self.view removeConstraints:_portraitActionBarTwoButtonConstraintsArray];
}

- (void)addPortraitActionBarConstraints {
  if ([_googleDriveButton isHidden] || _googleDriveButton.alpha == 0) {
    [self.view addConstraints:_portraitActionBarOneButtonConstraintsArray];
  } else {
    [self.view addConstraints:_portraitActionBarTwoButtonConstraintsArray];
  }
}

- (void)updateFileTypeLabelConstraint {
  CGFloat constant = kFileTypeLabelVerticalOffset;
  if (UIInterfaceOrientationIsPortrait(
          [[UIApplication sharedApplication] statusBarOrientation])) {
    if (!_isFileTypeLabelCentered) {
      constant += kFileTypeLabelAnimationDeltaYPortrait;
    }
  } else {
    if (!_isFileTypeLabelCentered) {
      constant += kFileTypeLabelAnimationDeltaYLandscape;
    }
  }
  self.fileTypeLabelCentered.constant = constant;
}

- (void)interfaceOrientationWillChangeNotification:
    (NSNotification*)notification {
  NSNumber* orientationNumber = [[notification userInfo]
      objectForKey:UIApplicationStatusBarOrientationUserInfoKey];
  UIInterfaceOrientation orientation =
      static_cast<UIInterfaceOrientation>([orientationNumber integerValue]);
  [self updateConstraints:orientation];
}

- (void)deviceOrientationDidChangeNotification:(NSNotification*)notification {
  [self setProgressBarHeightWithAnimation:NO withCompletionAnimation:NO];
  [self updateFileTypeLabelConstraint];
}

- (void)animateFileTypeLabelDown:(BOOL)down
                  withCompletion:(void (^)(BOOL))completion {
  if (_isFileTypeLabelCentered == !down) {
    if (completion) {
      completion(YES);
    }
    return;
  }

  _isFileTypeLabelCentered = !down;
  [UIView animateWithDuration:kFileTypeLabelAnimationDuration
      delay:0
      options:UIViewAnimationOptionCurveEaseInOut
      animations:^{
        [self updateFileTypeLabelConstraint];
        [self.view layoutIfNeeded];
      }
      completion:^(BOOL finished) {
        if (completion) {
          completion(finished);
        }
      }];
}

#pragma mark - Getting metadata

// TODO(fulbright): Investigate moving this code into -wasShown instead
// (-wasShown is currently not being called on this class even when it's
// implemented).
- (void)start {
  [self startHeadFetch];
}

- (void)startHeadFetch {
  _fetcher = URLFetcher::Create([self url], URLFetcher::HEAD,
                                _headFetcherDelegate.get());
  _fetcher->SetRequestContext(
      _webState->GetBrowserState()->GetRequestContext());
  [[NetworkActivityIndicatorManager sharedInstance]
      startNetworkTaskForGroup:[self getNetworkActivityKey]];
  _fetcher->Start();
}

- (void)onHeadFetchComplete {
  [[NetworkActivityIndicatorManager sharedInstance]
      stopNetworkTaskForGroup:[self getNetworkActivityKey]];
  int responseCode = _fetcher->GetResponseCode();
  base::UmaHistogramSparse(
      kUMADownloadedFileStatusCode,
      net::HttpUtil::MapStatusCodeForHistogram(responseCode));

  if (!_fetcher->GetStatus().is_success() || responseCode != 200) {
    [self displayError];
    return;
  }

  _didSuccessfullyFinishHeadFetch = YES;
  HttpResponseHeaders* headers = _fetcher->GetResponseHeaders();

  std::string contentDisposition;
  headers->GetNormalizedHeader("Content-Disposition", &contentDisposition);
  // TODO(fulbright): It's possible that the filename returned from
  // GetSuggestedFilename will already be in use (especially if "document" was
  // used in a previous download). Find a way to avoid filename collisions.
  // TODO(fulbright): Investigate why the comment on GetSuggestedFilename says
  // that |mime_type| should only be passed if it's called on a thread that
  // allows IO.
  base::string16 utilGeneratedFilename =
      net::GetSuggestedFilename(_fetcher->GetURL(), contentDisposition,
                                "",  // charset
                                "",  // "suggested" filename
                                "",  // mime type
                                "document");

  NSString* filename = base::SysUTF16ToNSString(utilGeneratedFilename);
  if (filename != nil && ![filename isEqualToString:@""]) {
    [_fileNameLabel setText:filename];
    [_fileNameLabel setHidden:NO];
  }

  NSString* fileType = [filename pathExtension];
  if (fileType != nil && ![fileType isEqualToString:@""]) {
    [_fileTypeLabel setText:[fileType uppercaseString]];
    [_fileTypeLabel setHidden:NO];
  }

  int64_t length = headers->GetContentLength();
  if (length > 0) {
    _totalFileSize = length;
    NSString* fileSizeText = [NSByteCountFormatter
        stringFromByteCount:length
                 countStyle:NSByteCountFormatterCountStyleFile];
    [_errorOrSizeLabel setText:fileSizeText];
  } else {
    [_errorOrSizeLabel
        setText:l10n_util::GetNSString(
                    IDS_IOS_DOWNLOAD_MANAGER_CANNOT_DETERMINE_FILE_SIZE)];
    _totalFileSize = kNoFileSizeGiven;
    [self initializeActivityIndicator];
    _documentContainer.backgroundColor =
        UIColorFromRGB(kIndeterminateFileSizeColor);
    _fileTypeLabel.textColor =
        [UIColor colorWithWhite:0 alpha:kFileTypeLabelBlackAlpha];
  }
  [_errorOrSizeLabel setHidden:NO];

  [_downloadButton setHidden:NO];
}

- (void)initializeActivityIndicator {
  _activityIndicator = [[MDCActivityIndicator alloc]
      initWithFrame:CGRectMake(0, 0, kActivityIndicatorWidth,
                               kActivityIndicatorWidth)];
  [_activityIndicator setRadius:AlignValueToPixel(kActivityIndicatorWidth / 2)];
  [_activityIndicator setStrokeWidth:4];
  [_activityIndicator
      setCycleColors:@[ [[MDCPalette cr_bluePalette] tint500] ]];
  [_activityIndicator setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_documentContainer addSubview:_activityIndicator];
  _activityIndicator.center = _documentContainer.center;
  [NSLayoutConstraint activateConstraints:@[
    [[_activityIndicator centerYAnchor]
        constraintEqualToAnchor:_documentContainer.centerYAnchor],
    [[_activityIndicator centerXAnchor]
        constraintEqualToAnchor:_documentContainer.centerXAnchor],
    [[_activityIndicator heightAnchor]
        constraintEqualToConstant:kActivityIndicatorWidth],
    [[_activityIndicator widthAnchor]
        constraintEqualToConstant:kActivityIndicatorWidth]
  ]];
}

#pragma mark - Errors

- (void)displayUnableToOpenFileDialog {
  NSString* title =
      l10n_util::GetNSString(IDS_IOS_DOWNLOAD_MANAGER_UNABLE_TO_OPEN_FILE);
  NSString* message =
      l10n_util::GetNSString(IDS_IOS_DOWNLOAD_MANAGER_NO_APP_MESSAGE);

  _alertCoordinator = [[AlertCoordinator alloc]
      initWithBaseViewController:self.baseViewController
                           title:title
                         message:message];

  StoreKitTabHelper* tabHelper = StoreKitTabHelper::FromWebState(_webState);
  if (tabHelper) {
    NSString* googleDriveButtonTitle =
        l10n_util::GetNSString(IDS_IOS_DOWNLOAD_MANAGER_UPLOAD_TO_GOOGLE_DRIVE);
    __weak LegacyDownloadManagerController* weakSelf = self;
    [_alertCoordinator addItemWithTitle:googleDriveButtonTitle
                                 action:^{
                                   [weakSelf openGoogleDriveInAppStore];
                                 }
                                  style:UIAlertActionStyleDefault];
  }
  [_alertCoordinator addItemWithTitle:l10n_util::GetNSString(IDS_CANCEL)
                               action:nil
                                style:UIAlertActionStyleCancel];

  [_alertCoordinator start];
}

- (void)displayError {
  if (_recordDownloadResultHistogram) {
    _recordDownloadResultHistogram = NO;
    UMA_HISTOGRAM_ENUMERATION(kUMADownloadFileResult, DOWNLOAD_FAILURE,
                              DOWNLOAD_FILE_RESULT_COUNT);
  }
  [_activityIndicator stopAnimating];
  _isDisplayingError = YES;
  self.fractionDownloaded = 0;
  if ([_fileTypeLabel isHidden]) {
    [self finishDisplayingError];
  } else {
    [self animateFileTypeLabelDown:YES
                    withCompletion:^(BOOL finished) {
                      [self finishDisplayingError];
                    }];
  }
}

- (void)finishDisplayingError {
  _timeLeftLabel.hidden = YES;
  _progressBar.hidden = YES;
  _foldIcon.image = [UIImage imageNamed:@"file_icon_fold"];

  // Prepare the error icon for the fading in animation.
  [_errorIcon setAlpha:0.0];
  [_errorIcon setHidden:NO];

  // Prepare the "Download failed." label text for the fading in animation.
  NSString* errorText =
      l10n_util::GetNSString(IDS_IOS_DOWNLOAD_MANAGER_DOWNLOAD_FAILED);
  CATransition* errorTextAnimation = [CATransition animation];
  errorTextAnimation.duration = kErrorAnimationDuration;
  [_documentContainer setAccessibilityLabel:errorText];

  // Prepare the "RETRY DOWNLOAD" button text for the fading in animation.
  [self setTitle:IDS_IOS_DOWNLOAD_MANAGER_RETRY_DOWNLOAD
       forButton:_downloadButton];
  _downloadButton.alpha = 0.0;
  _downloadButton.hidden = NO;

  [UIView
      animateWithDuration:kErrorAnimationDuration
               animations:^{
                 [_errorIcon setAlpha:1.0];
                 [_documentContainer
                     setBackgroundColor:UIColorFromRGB(kErrorDocumentColor)];
                 [_downloadButton setAlpha:1.0];
                 if (!_cancelButton.hidden) {
                   [_cancelButton setAlpha:0.0];
                 }
               }];

  _errorOrSizeLabel.hidden = NO;
  [_errorOrSizeLabel.layer addAnimation:errorTextAnimation
                                 forKey:@"displayError"];
  [_errorOrSizeLabel setText:errorText];
}

- (IBAction)downloadButtonTapped:(id)sender {
  _recordDownloadResultHistogram = YES;
  [_downloadButton setHidden:YES];
  NSString* retryDownloadText =
      l10n_util::GetNSString(IDS_IOS_DOWNLOAD_MANAGER_RETRY_DOWNLOAD);
  if ([_downloadButton.titleLabel.text
          caseInsensitiveCompare:retryDownloadText] == NSOrderedSame) {
    base::RecordAction(UserMetricsAction("MobileDownloadRetryDownload"));
  }
  // Hide any error message that may have been shown in a previous attempt to
  // download the file.
  if (_isDisplayingError) {
    [self hideError];
  } else {
    [self finishDownloadButtonTapped];
  }
}

- (void)hideError {
  // Get the file size string to display.
  NSString* fileSizeText;
  if (!_didSuccessfullyFinishHeadFetch) {
    // The file size hasn't been fetched yet, so just show a blank string.
    fileSizeText = @"";
  } else if (_totalFileSize == kNoFileSizeGiven) {
    fileSizeText = l10n_util::GetNSString(
        IDS_IOS_DOWNLOAD_MANAGER_CANNOT_DETERMINE_FILE_SIZE);
  } else {
    fileSizeText = [NSByteCountFormatter
        stringFromByteCount:_totalFileSize
                 countStyle:NSByteCountFormatterCountStyleFile];
  }
  CATransition* fileSizeTextAnimation = [CATransition animation];
  fileSizeTextAnimation.duration = kErrorAnimationDuration;

  // Prepare to change the download button back to its original text.
  [_errorOrSizeLabel.layer addAnimation:fileSizeTextAnimation
                                 forKey:@"hideError"];
  [_errorOrSizeLabel setText:fileSizeText];

  if (![_fileTypeLabel isHidden] && _totalFileSize != kNoFileSizeGiven) {
    [self animateFileTypeLabelDown:NO withCompletion:nil];
  }

  [UIView animateWithDuration:kErrorAnimationDuration
      animations:^{
        _documentContainer.backgroundColor =
            UIColorFromRGB(kUndownloadedDocumentColor);
        _errorIcon.alpha = 0.0;
        _downloadButton.alpha = 0.0;
      }
      completion:^(BOOL finished) {
        _errorIcon.hidden = YES;
        _errorIcon.alpha = 1.0;
        _downloadButton.hidden = YES;
        _downloadButton.alpha = 1.0;
        [self setTitle:IDS_IOS_DOWNLOAD_MANAGER_DOWNLOAD
             forButton:_downloadButton];
        _isDisplayingError = NO;
        [self finishDownloadButtonTapped];
      }];
}

- (void)finishDownloadButtonTapped {
  if (_didSuccessfullyFinishHeadFetch) {
    [self beginStartingContentDownload];
  } else {
    [self startHeadFetch];
  }
}

#pragma mark - Downloading content

- (void)beginStartingContentDownload {
  // Ensure the directory that downloaded files are saved to exists.
  base::FilePath downloadsDirectoryPath;
  if (!GetDownloadsDirectory(&downloadsDirectoryPath)) {
    [self displayError];
    return;
  }
  __weak LegacyDownloadManagerController* weakSelf = self;
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::BindBlockArc(^{
        return CreateDirectory(downloadsDirectoryPath);
      }),
      base::BindBlockArc(^(bool directoryCreated) {
        [weakSelf finishStartingContentDownload:directoryCreated];
      }));
}

- (void)finishStartingContentDownload:(bool)directoryCreated {
  if (!directoryCreated) {
    [self displayError];
    return;
  }
  base::FilePath downloadsDirectoryPath;
  if (!GetDownloadsDirectory(&downloadsDirectoryPath)) {
    [self displayError];
    return;
  }
  _downloadFilePath = downloadsDirectoryPath.Append(
      base::SysNSStringToUTF8([_fileNameLabel text]));

  _fetcher = URLFetcher::Create([self url], URLFetcher::GET,
                                _contentFetcherDelegate.get());
  _fetcher->SetRequestContext(
      _webState->GetBrowserState()->GetRequestContext());
  _fetcher->SaveResponseToFileAtPath(
      _downloadFilePath,
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND}));
  [[NetworkActivityIndicatorManager sharedInstance]
      startNetworkTaskForGroup:[self getNetworkActivityKey]];
  _fetcher->Start();

  self.downloadStartedTime = [NSDate date];
  self.fractionDownloaded = 0.0;
  [self setProgressBarHeightWithAnimation:NO withCompletionAnimation:NO];
  [self setTimeLeft:@"" withAnimation:NO];
  [_progressBar setHidden:NO];
  [_cancelButton setHidden:NO];
  [_cancelButton setAlpha:1];
}

- (void)setProgressBarHeightWithAnimation:(BOOL)animated
                  withCompletionAnimation:(BOOL)completionAnimation {
  CGRect documentIconFrame = [_documentIcon frame];
  CGRect oldProgressFrame = [_progressBar frame];
  CGRect newProgressFrame = oldProgressFrame;
  newProgressFrame.size.height =
      self.fractionDownloaded * documentIconFrame.size.height;
  newProgressFrame.origin.y =
      CGRectGetMaxY(documentIconFrame) - newProgressFrame.size.height;
  if (animated &&
      newProgressFrame.size.height - oldProgressFrame.size.height > 1) {
    __weak UIView* weakProgressBar = _progressBar;
    if (completionAnimation) {
      __weak LegacyDownloadManagerController* weakSelf = self;
      [UIView animateWithDuration:kProgressBarAnimationDuration
          animations:^{
            [weakProgressBar setFrame:newProgressFrame];
          }
          completion:^(BOOL) {
            [weakSelf runDownloadCompleteAnimation];
          }];
    } else {
      [UIView animateWithDuration:kProgressBarAnimationDuration
                       animations:^{
                         [weakProgressBar setFrame:newProgressFrame];
                       }];
    }
  } else {
    [_progressBar setFrame:newProgressFrame];
    if (completionAnimation) {
      [self runDownloadCompleteAnimation];
    }
  }
}

- (void)setTitle:(int)messageId forButton:(UIButton*)button {
  NSString* title = l10n_util::GetNSString(messageId);
  NSString* capsTitle =
      [title uppercaseStringWithLocale:[NSLocale currentLocale]];
  [button setTitle:capsTitle forState:UIControlStateNormal];
  [button setTitle:capsTitle forState:UIControlStateHighlighted];
}

- (void)runDownloadCompleteAnimation {
  [_documentContainer
      setBackgroundColor:UIColorFromRGB(kDownloadedDocumentColor)];

  CAKeyframeAnimation* animation =
      [CAKeyframeAnimation animationWithKeyPath:@"transform"];
  NSArray* vals = @[
    [NSValue valueWithCATransform3D:CATransform3DMakeScale(1.0, 1.0, 1.0)],
    [NSValue valueWithCATransform3D:CATransform3DMakeScale(0.95, 0.95, 1.0)],
    [NSValue valueWithCATransform3D:CATransform3DMakeScale(1.1, 1.1, 1.0)],
    [NSValue valueWithCATransform3D:CATransform3DMakeScale(1.0, 1.0, 1.0)]
  ];
  NSArray* times = @[ @0.0, @0.33, @0.66, @1.0 ];
  [animation setValues:vals];
  [animation setKeyTimes:times];
  [animation setDuration:kDownloadCompleteAnimationDuration];
  [_documentContainer.layer addAnimation:animation
                                  forKey:kDocumentPopAnimationKey];

  // Displays the "Install Google Drive" button if it is not installed.
  NSURL* googleDriveURL = [NSURL URLWithString:kGoogleDriveURL];
  BOOL showGoogleDriveButton =
      ![[UIApplication sharedApplication] canOpenURL:googleDriveURL];
  __weak UIImageView* weakFoldIcon = _foldIcon;
  [UIView transitionWithView:_foldIcon
      duration:kDownloadCompleteAnimationDuration
      options:UIViewAnimationOptionTransitionCrossDissolve
      animations:^{
        [weakFoldIcon setImage:[UIImage imageNamed:@"file_icon_fold_complete"]];
        if (_totalFileSize == kNoFileSizeGiven) {
          _fileTypeLabel.textColor =
              [UIColor colorWithWhite:1 alpha:kFileTypeLabelWhiteAlpha];
        }
      }
      completion:^(BOOL finished) {
        if (showGoogleDriveButton) {
          [self showGoogleDriveButton];
        }
      }];
}

- (void)setFractionDownloaded:(double)fractionDownloaded {
  _fractionDownloaded = fractionDownloaded;
  if (_fractionDownloaded == 1) {
    [_documentContainer
        setAccessibilityLabel:l10n_util::GetNSString(
                                  IDS_IOS_DOWNLOAD_MANAGER_DOWNLOAD_COMPLETE)];
  } else {
    base::string16 percentDownloaded = base::SysNSStringToUTF16([NSString
        stringWithFormat:@"%i", static_cast<int>(100 * _fractionDownloaded)]);
    [_documentContainer
        setAccessibilityLabel:l10n_util::GetNSStringF(
                                  IDS_IOS_DOWNLOAD_MANAGER_PERCENT_DOWNLOADED,
                                  percentDownloaded)];
  }
}

- (IBAction)cancelButtonTapped:(id)sender {
  // No-op if the file has finished downloading, which occurs if the download
  // completes while the user is pressing down on the Cancel button.
  if (self.fractionDownloaded == 1)
    return;

  self.fractionDownloaded = 0;
  [_cancelButton setHidden:YES];
  [[NetworkActivityIndicatorManager sharedInstance]
      stopNetworkTaskForGroup:[self getNetworkActivityKey]];
  if (_totalFileSize == kNoFileSizeGiven) {
    [_activityIndicator stopAnimating];
    [self animateFileTypeLabelDown:NO withCompletion:nil];
  }
  _fetcher.reset();
  [_progressBar setHidden:YES];
  [_timeLeftLabel setHidden:YES];
  [_downloadButton setHidden:NO];
  if (_recordDownloadResultHistogram) {
    _recordDownloadResultHistogram = NO;
    UMA_HISTOGRAM_ENUMERATION(kUMADownloadFileResult, DOWNLOAD_CANCELED,
                              DOWNLOAD_FILE_RESULT_COUNT);
  }
}

- (void)onContentFetchProgress:(long long)bytesDownloaded {
  if (_totalFileSize != kNoFileSizeGiven) {
    self.fractionDownloaded =
        static_cast<double>(bytesDownloaded) / _totalFileSize;
    BOOL showFinishingAnimation = (bytesDownloaded == _totalFileSize);
    [self setProgressBarHeightWithAnimation:YES
                    withCompletionAnimation:showFinishingAnimation];

    NSTimeInterval elapsedSeconds =
        -[self.downloadStartedTime timeIntervalSinceNow];
    long long bytesRemaining = _totalFileSize - bytesDownloaded;
    DCHECK(bytesDownloaded > 0);
    double secondsRemaining =
        static_cast<double>(bytesRemaining) / bytesDownloaded * elapsedSeconds;
    double minutesRemaining = secondsRemaining / 60;
    int minutesRemainingInt = static_cast<int>(ceil(minutesRemaining));
    base::string16 minutesRemainingString = base::SysNSStringToUTF16(
        [NSString stringWithFormat:@"%d", minutesRemainingInt]);
    NSString* minutesRemainingText = l10n_util::GetNSStringF(
        IDS_IOS_DOWNLOAD_MANAGER_TIME_LEFT, minutesRemainingString);
    [self setTimeLeft:minutesRemainingText withAnimation:YES];
    [_timeLeftLabel setHidden:NO];
  } else {
    [_documentContainer
        setAccessibilityLabel:l10n_util::GetNSString(
                                  IDS_IOS_DOWNLOAD_MANAGER_DOWNLOADING)];
    [_activityIndicator startAnimating];
    if (_isFileTypeLabelCentered) {
      [self animateFileTypeLabelDown:YES withCompletion:nil];
    }
  }
}

- (void)setTimeLeft:(NSString*)text withAnimation:(BOOL)animated {
  if (![text isEqualToString:[_timeLeftLabel text]]) {
    if (animated) {
      CATransition* animation = [CATransition animation];
      animation.duration = kTimeLeftAnimationDuration;
      animation.type = kCATransitionPush;
      animation.subtype = kCATransitionFromBottom;
      animation.timingFunction = [CAMediaTimingFunction
          functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
      [_timeLeftLabel.layer addAnimation:animation
                                  forKey:kTimeLeftAnimationKey];
    }
    [_timeLeftLabel setText:text];
  }
}

- (void)onContentFetchComplete {
  [_cancelButton setHidden:YES];
  [_timeLeftLabel setHidden:YES];
  [[NetworkActivityIndicatorManager sharedInstance]
      stopNetworkTaskForGroup:[self getNetworkActivityKey]];

  if (!_fetcher->GetStatus().is_success() ||
      _fetcher->GetResponseCode() != 200) {
    [self displayError];
    // Log the Net Error code.
    base::UmaHistogramSparse(kUMADownloadFileNetError,
                             -_fetcher->GetStatus().error());
    return;
  }

  base::FilePath filePath;
  bool success = _fetcher->GetResponseAsFilePath(YES, &filePath);
  DCHECK(success);
  NSString* filePathString = base::SysUTF8ToNSString(filePath.value());

  if (_totalFileSize == kNoFileSizeGiven) {
    [_activityIndicator stopAnimating];
    _activityIndicator = nil;

    // Display the file size.
    NSError* error = nil;
    NSDictionary* attributes =
        [[NSFileManager defaultManager] attributesOfItemAtPath:filePathString
                                                         error:&error];
    if (!error) {
      NSNumber* fileSizeNumber = [attributes objectForKey:NSFileSize];
      NSString* fileSizeText = [NSByteCountFormatter
          stringFromByteCount:[fileSizeNumber longLongValue]
                   countStyle:NSByteCountFormatterCountStyleFile];
      [_errorOrSizeLabel setText:fileSizeText];
    }

    [self animateFileTypeLabelDown:NO withCompletion:nil];
    // Set the progress bar to 100% to give the document the filled in blue
    // background.
    self.fractionDownloaded = 1.0;
    [self setProgressBarHeightWithAnimation:YES withCompletionAnimation:YES];
  }

  NSURL* fileURL = [NSURL fileURLWithPath:filePathString];
  self.docInteractionController =
      [UIDocumentInteractionController interactionControllerWithURL:fileURL];
  self.docInteractionController.delegate = self;

  [_openInButton setHidden:NO];

  _recordFileActionHistogram = YES;
  _recordDownloadResultHistogram = NO;
  UMA_HISTOGRAM_ENUMERATION(kUMADownloadFileResult, DOWNLOAD_COMPLETED,
                            DOWNLOAD_FILE_RESULT_COUNT);
}

#pragma mark - Completed download actions

- (void)showGoogleDriveButton {
  _googleDriveButton.alpha = 0.0;
  _googleDriveButton.hidden = NO;
  UIInterfaceOrientation orientation =
      [[UIApplication sharedApplication] statusBarOrientation];
  if (UIInterfaceOrientationIsPortrait(orientation)) {
    [UIView animateWithDuration:kGoogleDriveButtonAnimationDuration
                     animations:^{
                       _googleDriveButton.alpha = 1.0;
                       [self updatePortraitActionBarConstraints];
                       [self.view layoutIfNeeded];
                     }];
  } else {
    [UIView animateWithDuration:kGoogleDriveButtonAnimationDuration
                     animations:^{
                       _googleDriveButton.alpha = 1.0;
                     }];
  }
}

- (void)hideGoogleDriveButton {
  base::RecordAction(
      UserMetricsAction("MobileDownloadFileUIInstallGoogleDrive"));
  UIInterfaceOrientation orientation =
      [[UIApplication sharedApplication] statusBarOrientation];
  if (UIInterfaceOrientationIsPortrait(orientation)) {
    [UIView animateWithDuration:kDownloadCompleteAnimationDuration
                     animations:^{
                       _googleDriveButton.alpha = 0.0;
                       [self updatePortraitActionBarConstraints];
                       [self.view layoutIfNeeded];
                     }];
  } else {
    [UIView animateWithDuration:kDownloadCompleteAnimationDuration
                     animations:^{
                       _googleDriveButton.alpha = 0.0;
                     }];
  }
}

- (IBAction)openInButtonTapped:(id)sender {
  BOOL showedMenu =
      [_docInteractionController presentOpenInMenuFromRect:_openInButton.frame
                                                    inView:_actionBar
                                                  animated:YES];
  if (!showedMenu) {
    [self displayUnableToOpenFileDialog];
  }
}

- (IBAction)googleDriveButtonTapped:(id)sender {
  [self openGoogleDriveInAppStore];
}

- (void)openGoogleDriveInAppStore {
  StoreKitTabHelper* helper = StoreKitTabHelper::FromWebState(_webState);
  if (helper) {
    NSString* scheme = [[NSURL URLWithString:kGoogleDriveURL] scheme];
    [[InstallationNotifier sharedInstance]
        registerForInstallationNotifications:self
                                withSelector:@selector(hideGoogleDriveButton)
                                   forScheme:scheme];
    helper->OpenAppStore(kGoogleDriveAppStoreId);
  }
}

- (NSString*)getNetworkActivityKey {
  return [NSString
      stringWithFormat:
          @"LegacyDownloadManagerController.NetworkActivityIndicatorKey.%d",
          _downloadManagerId];
}

#pragma mark - UIDocumentInteractionControllerDelegate

- (void)documentInteractionController:
            (UIDocumentInteractionController*)controller
        willBeginSendingToApplication:(NSString*)application {
  if (_recordFileActionHistogram) {
    if ([application isEqualToString:kGoogleDriveBundleId]) {
      UMA_HISTOGRAM_ENUMERATION(kUMADownloadedFileAction, OPENED_IN_DRIVE,
                                DOWNLOADED_FILE_ACTION_COUNT);
    } else {
      UMA_HISTOGRAM_ENUMERATION(kUMADownloadedFileAction, OPENED_IN_OTHER_APP,
                                DOWNLOADED_FILE_ACTION_COUNT);
    }
    _recordFileActionHistogram = NO;
  }
}

#pragma mark - Methods for testing

- (long long)totalFileSize {
  return _totalFileSize;
}

#pragma mark - CRWNativeContent

- (void)close {
  // Makes sure that all outstanding network requests are shut down before
  // this controller is closed.
  _fetcher.reset();
}

@end
