// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_QR_SCANNER_REQUIREMENTS_QR_SCANNER_RESULT_LOADING_H_
#define IOS_CHROME_BROWSER_UI_QR_SCANNER_REQUIREMENTS_QR_SCANNER_RESULT_LOADING_H_

// QRScannerResultLoading contains methods that allow the QR scanner to load
// pages when valid QR codes are detected.
@protocol QRScannerResultLoading

// Called when the scanner detects a valid code. Camera recording is stopped
// when a result is scanned and the QRScannerViewController is dismissed. This
// function is called when the dismissal completes. A valid code is any
// non-empty string. If |load| is YES, the scanned code was of a type which can
// only encode digits, and the delegate can load the result immediately, instead
// of prompting the user to confirm the result.
- (void)receiveQRScannerResult:(NSString*)qrScannerResult
               loadImmediately:(BOOL)load;

@end

#endif  // IOS_CHROME_BROWSER_UI_QR_SCANNER_REQUIREMENTS_QR_SCANNER_RESULT_LOADING_H_
