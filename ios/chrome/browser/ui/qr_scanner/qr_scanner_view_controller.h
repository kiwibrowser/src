// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_QR_SCANNER_QR_SCANNER_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_QR_SCANNER_QR_SCANNER_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include "ios/chrome/browser/ui/qr_scanner/camera_controller.h"

@protocol QRScannerPresenting;
@protocol QRScannerResultLoading;

@interface QRScannerViewController : UIViewController<CameraControllerDelegate>

- (instancetype)
initWithPresentationProvider:(id<QRScannerPresenting>)presentationProvider
                loadProvider:(id<QRScannerResultLoading>)loadProvider
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithNibName:(NSString*)name
                         bundle:(NSBundle*)bundle NS_UNAVAILABLE;

- (instancetype)initWithCoder:(NSCoder*)coder NS_UNAVAILABLE;

// Returns a view controller to be presented based on the camera state. Returns
// |self| if the camera is available or an appropriate UIAlertController if
// there was an error loading the camera.
- (UIViewController*)getViewControllerToPresent;

@end

#endif  // IOS_CHROME_BROWSER_UI_QR_SCANNER_QR_SCANNER_VIEW_CONTROLLER_H_
