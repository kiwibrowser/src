// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_CONSTANTS_H_
#define CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_CONSTANTS_H_

namespace cloud_print {

extern const char kPrivetKeyError[];
extern const char kPrivetInfoKeyToken[];
extern const char kPrivetInfoKeyAPIList[];
extern const char kPrivetInfoKeyID[];
extern const char kPrivetKeyDeviceID[];
extern const char kPrivetKeyClaimURL[];
extern const char kPrivetKeyClaimToken[];
extern const char kPrivetKeyTimeout[];

extern const char kPrivetActionNameInfo[];

extern const char kPrivetInfoPath[];
extern const char kPrivetRegisterPath[];
extern const char kPrivetCapabilitiesPath[];
extern const char kPrivetSubmitdocPath[];
extern const char kPrivetCreatejobPath[];

extern const char kPrivetErrorDeviceBusy[];
extern const char kPrivetErrorPrinterBusy[];
extern const char kPrivetErrorInvalidPrintJob[];
extern const char kPrivetErrorInvalidDocumentType[];
extern const char kPrivetErrorPendingUserAction[];
extern const char kPrivetErrorInvalidXPrivetToken[];
extern const char kPrivetErrorTimeout[];
extern const char kPrivetErrorCancel[];

extern const char kPrivetActionStart[];
extern const char kPrivetActionGetClaimToken[];
extern const char kPrivetActionComplete[];
extern const char kPrivetActionCancel[];

extern const char kPrivetDefaultDeviceType[];

extern const char kPrivetTypePrinter[];

extern const char kPrivetTxtKeyName[];
extern const char kPrivetTxtKeyDescription[];
extern const char kPrivetTxtKeyVersion[];
extern const char kPrivetTxtKeyType[];
extern const char kPrivetTxtKeyID[];
extern const char kPrivetTxtKeyGcdID[];
extern const char kPrivetTxtKeyDevicesClass[];

const int kPrivetDefaultTimeout = 15;

const double kPrivetMaximumTimeRandomAddition = 0.2;

const int kPrivetMinimumTimeout = 2;

}  // namespace cloud_print

#endif  // CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_CONSTANTS_H_
