// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_UI_SUPPRESSED_ELEMENT_H_
#define CHROME_BROWSER_VR_UI_SUPPRESSED_ELEMENT_H_

namespace vr {

// Ensure that this stays in sync with VRSuppressedElement in enums.xml
// These values are written to logs.  New enum values can be added, but existing
// enums must never be renumbered or deleted and reused.
enum class UiSuppressedElement : int {
  kFileChooser = 0,
  kBluetoothChooser,
  kJavascriptDialog,
  kMediaPermission,
  kPermissionRequest,
  kQuotaPermission,
  kHttpAuth,
  kDownloadPermission,
  kFileAccessPermission,
  kPasswordManager,
  kAutofill,
  kUsbChooser,
  kSslClientCertificate,
  kMediaRouterPresentationRequest,

  // This must be last.
  kCount,
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_UI_SUPPRESSED_ELEMENT_H_
