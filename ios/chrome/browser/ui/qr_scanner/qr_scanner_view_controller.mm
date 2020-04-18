// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/qr_scanner/qr_scanner_view_controller.h"

#import <AVFoundation/AVFoundation.h>

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "ios/chrome/browser/ui/qr_scanner/qr_scanner_alerts.h"
#include "ios/chrome/browser/ui/qr_scanner/qr_scanner_transitioning_delegate.h"
#include "ios/chrome/browser/ui/qr_scanner/qr_scanner_view.h"
#include "ios/chrome/browser/ui/qr_scanner/requirements/qr_scanner_presenting.h"
#include "ios/chrome/browser/ui/qr_scanner/requirements/qr_scanner_result_loading.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;

namespace {

// The reason why the QRScannerViewController was dismissed. Used for collecting
// metrics.
enum DismissalReason {
  CLOSE_BUTTON,
  ERROR_DIALOG,
  SCANNED_CODE,
  // Not reported. Should be kept last of enum.
  IMPOSSIBLY_UNLIKELY_AUTHORIZATION_CHANGE
};

}  // namespace

@interface QRScannerViewController ()<QRScannerViewDelegate> {
  // The CameraController managing the camera connection.
  CameraController* _cameraController;
  // The view displaying the QR scanner.
  QRScannerView* _qrScannerView;
  // The scanned result.
  NSString* _result;
  // Whether the scanned result should be immediately loaded.
  BOOL _loadResultImmediately;
  // The transitioning delegate used for presenting and dismissing the QR
  // scanner.
  QRScannerTransitioningDelegate* _transitioningDelegate;
}

@property(nonatomic, readwrite, weak) id<QRScannerPresenting>
    presentationProvider;
@property(nonatomic, readwrite, weak) id<QRScannerResultLoading> loadProvider;

// Dismisses the QRScannerViewController and runs |completion| on completion.
// Logs metrics according to the |reason| for dismissal.
- (void)dismissForReason:(DismissalReason)reason
          withCompletion:(void (^)(void))completion;
// Starts receiving notifications about the UIApplication going to background.
- (void)startReceivingNotifications;
// Stops receiving all notificatins.
- (void)stopReceivingNotifications;
// Requests the torch mode to be set to |mode| by the |_cameraController| and
// the icon of the torch button to be changed by the |_qrScannerView|.
- (void)setTorchMode:(AVCaptureTorchMode)mode;

// Stops recording when the application resigns active.
- (void)handleUIApplicationWillResignActiveNotification;
// Dismisses the QR scanner and passes the scanned result to the delegate when
// the accessibility announcement for scanned QR code finishes.
- (void)handleUIAccessibilityAnnouncementDidFinishNotification:
    (NSNotification*)notification;

@end

@implementation QRScannerViewController

@synthesize loadProvider = _loadProvider;
@synthesize presentationProvider = _presentationProvider;

#pragma mark lifecycle

- (instancetype)
initWithPresentationProvider:(id<QRScannerPresenting>)presentationProvider
                loadProvider:(id<QRScannerResultLoading>)loadProvider {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _loadProvider = loadProvider;
    _presentationProvider = presentationProvider;
    _cameraController = [CameraController cameraControllerWithDelegate:self];
  }
  return self;
}

- (instancetype)initWithNibName:(NSString*)name bundle:(NSBundle*)bundle {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithCoder:(NSCoder*)coder {
  NOTREACHED();
  return nil;
}

#pragma mark UIAccessibilityAction

- (BOOL)accessibilityPerformEscape {
  [self dismissForReason:CLOSE_BUTTON withCompletion:nil];
  return YES;
}

#pragma mark UIViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  DCHECK(_cameraController);

  _qrScannerView =
      [[QRScannerView alloc] initWithFrame:self.view.frame delegate:self];
  [self.view addSubview:_qrScannerView];

  // Constraints for |_qrScannerView|.
  [_qrScannerView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [NSLayoutConstraint activateConstraints:@[
    [[_qrScannerView leadingAnchor]
        constraintEqualToAnchor:[self.view leadingAnchor]],
    [[_qrScannerView trailingAnchor]
        constraintEqualToAnchor:[self.view trailingAnchor]],
    [[_qrScannerView topAnchor] constraintEqualToAnchor:[self.view topAnchor]],
    [[_qrScannerView bottomAnchor]
        constraintEqualToAnchor:[self.view bottomAnchor]],
  ]];

  AVCaptureVideoPreviewLayer* previewLayer = [_qrScannerView getPreviewLayer];
  switch ([_cameraController getAuthorizationStatus]) {
    case AVAuthorizationStatusNotDetermined:
      [_cameraController
          requestAuthorizationAndLoadCaptureSession:previewLayer];
      break;
    case AVAuthorizationStatusAuthorized:
      [_cameraController loadCaptureSession:previewLayer];
      break;
    case AVAuthorizationStatusRestricted:
    case AVAuthorizationStatusDenied:
      // If this happens, then the user is really unlucky:
      // The authorization status changed in between the moment this VC was
      // instantiated and presented, and the moment viewDidLoad was called.
      [self dismissForReason:IMPOSSIBLY_UNLIKELY_AUTHORIZATION_CHANGE
              withCompletion:nil];
      break;
  }
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [self startReceivingNotifications];
  [_cameraController startRecording];

  // Reset torch.
  [self setTorchMode:AVCaptureTorchModeOff];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:
           (id<UIViewControllerTransitionCoordinator>)coordinator {
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  CGFloat epsilon = 0.0001;
  // Note: targetTransform is always either identity or a 90, -90, or 180 degree
  // rotation.
  CGAffineTransform targetTransform = coordinator.targetTransform;
  CGFloat angle = atan2f(targetTransform.b, targetTransform.a);
  if (fabs(angle) > epsilon) {
    // Rotate the preview in the opposite direction of the interface rotation
    // and add a small value to the angle to force the rotation to occur in the
    // correct direction when rotating by 180 degrees.
    void (^animationBlock)(id<UIViewControllerTransitionCoordinatorContext>) =
        ^void(id<UIViewControllerTransitionCoordinatorContext> context) {
          [_qrScannerView rotatePreviewByAngle:(epsilon - angle)];
        };
    // Note: The completion block is called even if the animation is
    // interrupted, for example by pressing the home button, with the same
    // target transform as the animation block.
    void (^completionBlock)(id<UIViewControllerTransitionCoordinatorContext>) =
        ^void(id<UIViewControllerTransitionCoordinatorContext> context) {
          [_qrScannerView finishPreviewRotation];
        };
    [coordinator animateAlongsideTransition:animationBlock
                                 completion:completionBlock];
  } else if (!CGSizeEqualToSize(self.view.frame.size, size)) {
    // Reset the size of the preview if the bounds of the view controller
    // changed. This can happen if entering or leaving Split View mode on iPad.
    [_qrScannerView resetPreviewFrame:size];
    [_cameraController resetVideoOrientation:[_qrScannerView getPreviewLayer]];
  }
}

- (void)viewDidDisappear:(BOOL)animated {
  [super viewDidDisappear:animated];
  [_cameraController stopRecording];
  [self stopReceivingNotifications];

  // Reset torch.
  [self setTorchMode:AVCaptureTorchModeOff];
}

- (BOOL)prefersStatusBarHidden {
  return YES;
}

#pragma mark public methods

- (UIViewController*)getViewControllerToPresent {
  DCHECK(_cameraController);
  switch ([_cameraController getAuthorizationStatus]) {
    case AVAuthorizationStatusNotDetermined:
    case AVAuthorizationStatusAuthorized:
      _transitioningDelegate = [[QRScannerTransitioningDelegate alloc] init];
      [self setTransitioningDelegate:_transitioningDelegate];
      return self;
    case AVAuthorizationStatusRestricted:
    case AVAuthorizationStatusDenied:
      return qr_scanner::DialogForCameraState(
          qr_scanner::CAMERA_PERMISSION_DENIED, nil);
  }
}

#pragma mark private methods

- (void)dismissForReason:(DismissalReason)reason
          withCompletion:(void (^)(void))completion {
  switch (reason) {
    case CLOSE_BUTTON:
      base::RecordAction(UserMetricsAction("MobileQRScannerClose"));
      break;
    case ERROR_DIALOG:
      base::RecordAction(UserMetricsAction("MobileQRScannerError"));
      break;
    case SCANNED_CODE:
      base::RecordAction(UserMetricsAction("MobileQRScannerScannedCode"));
      break;
    case IMPOSSIBLY_UNLIKELY_AUTHORIZATION_CHANGE:
      break;
  }

  [self.presentationProvider dismissQRScannerViewController:self
                                                 completion:completion];
}

- (void)startReceivingNotifications {
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(handleUIApplicationWillResignActiveNotification)
             name:UIApplicationWillResignActiveNotification
           object:nil];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(
                      handleUIAccessibilityAnnouncementDidFinishNotification:)
             name:UIAccessibilityAnnouncementDidFinishNotification
           object:nil];
}

- (void)stopReceivingNotifications {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)setTorchMode:(AVCaptureTorchMode)mode {
  [_cameraController setTorchMode:mode];
}

#pragma mark notification handlers

- (void)handleUIApplicationWillResignActiveNotification {
  [self setTorchMode:AVCaptureTorchModeOff];
}

- (void)handleUIAccessibilityAnnouncementDidFinishNotification:
    (NSNotification*)notification {
  NSString* announcement = [[notification userInfo]
      valueForKey:UIAccessibilityAnnouncementKeyStringValue];
  if ([announcement
          isEqualToString:
              l10n_util::GetNSString(
                  IDS_IOS_QR_SCANNER_CODE_SCANNED_ACCESSIBILITY_ANNOUNCEMENT)]) {
    DCHECK(_result);
    [self dismissForReason:SCANNED_CODE
            withCompletion:^{
              [self.loadProvider receiveQRScannerResult:_result
                                        loadImmediately:_loadResultImmediately];
            }];
  }
}

#pragma mark CameraControllerDelegate

- (void)captureSessionIsConnected {
  [_cameraController setViewport:[_qrScannerView viewportRectOfInterest]];
}

- (void)cameraStateChanged:(qr_scanner::CameraState)state {
  switch (state) {
    case qr_scanner::CAMERA_AVAILABLE:
      // Dismiss any presented alerts.
      if ([self presentedViewController]) {
        [self dismissViewControllerAnimated:YES completion:nil];
      }
      break;
    case qr_scanner::CAMERA_IN_USE_BY_ANOTHER_APPLICATION:
    case qr_scanner::MULTIPLE_FOREGROUND_APPS:
    case qr_scanner::CAMERA_PERMISSION_DENIED:
    case qr_scanner::CAMERA_UNAVAILABLE_DUE_TO_SYSTEM_PRESSURE:
    case qr_scanner::CAMERA_UNAVAILABLE: {
      // Dismiss any presented alerts.
      if ([self presentedViewController]) {
        [self dismissViewControllerAnimated:YES completion:nil];
      }
      [self presentViewController:qr_scanner::DialogForCameraState(
                                      state,
                                      ^(UIAlertAction*) {
                                        [self dismissForReason:ERROR_DIALOG
                                                withCompletion:nil];
                                      })
                         animated:YES
                       completion:nil];
      break;
    }
    case qr_scanner::CAMERA_NOT_LOADED:
      NOTREACHED();
      break;
  }
}

- (void)torchStateChanged:(BOOL)torchIsOn {
  [_qrScannerView setTorchButtonTo:torchIsOn];
}

- (void)torchAvailabilityChanged:(BOOL)torchIsAvailable {
  [_qrScannerView enableTorchButton:torchIsAvailable];
}

- (void)receiveQRScannerResult:(NSString*)result loadImmediately:(BOOL)load {
  if (UIAccessibilityIsVoiceOverRunning()) {
    // Post a notification announcing that a code was scanned. QR scanner will
    // be dismissed when the UIAccessibilityAnnouncementDidFinishNotification is
    // received.
    _result = [result copy];
    _loadResultImmediately = load;
    UIAccessibilityPostNotification(
        UIAccessibilityAnnouncementNotification,
        l10n_util::GetNSString(
            IDS_IOS_QR_SCANNER_CODE_SCANNED_ACCESSIBILITY_ANNOUNCEMENT));
  } else {
    [_qrScannerView animateScanningResultWithCompletion:^void(void) {
      [self dismissForReason:SCANNED_CODE
              withCompletion:^{
                [self.loadProvider receiveQRScannerResult:result
                                          loadImmediately:load];
              }];
    }];
  }
}

#pragma mark QRScannerViewDelegate

- (void)dismissQRScannerView:(id)sender {
  [self dismissForReason:CLOSE_BUTTON withCompletion:nil];
}

- (void)toggleTorch:(id)sender {
  if ([_cameraController isTorchActive]) {
    [self setTorchMode:AVCaptureTorchModeOff];
  } else {
    base::RecordAction(UserMetricsAction("MobileQRScannerTorchOn"));
    [self setTorchMode:AVCaptureTorchModeOn];
  }
}

@end
