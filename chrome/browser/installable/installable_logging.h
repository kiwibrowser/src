// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALLABLE_INSTALLABLE_LOGGING_H_
#define CHROME_BROWSER_INSTALLABLE_INSTALLABLE_LOGGING_H_

#include <string>

namespace content {
class WebContents;
}

// These values are a central reference for installability errors. The
// InstallableManager will specify an InstallableStatusCode (or
// NO_ERROR_DETECTED) in its result. Clients may also add their own error codes,
// and utilise LogErrorToConsole to write a message to the devtools console.
// This enum backs an UMA histogram, so it must be treated as append-only.
enum InstallableStatusCode {
  NO_ERROR_DETECTED,
  RENDERER_EXITING,
  RENDERER_CANCELLED,
  USER_NAVIGATED,
  NOT_IN_MAIN_FRAME,
  NOT_FROM_SECURE_ORIGIN,
  NO_MANIFEST,
  MANIFEST_EMPTY,
  START_URL_NOT_VALID,
  MANIFEST_MISSING_NAME_OR_SHORT_NAME,
  MANIFEST_DISPLAY_NOT_SUPPORTED,
  MANIFEST_MISSING_SUITABLE_ICON,
  NO_MATCHING_SERVICE_WORKER,
  NO_ACCEPTABLE_ICON,
  CANNOT_DOWNLOAD_ICON,
  NO_ICON_AVAILABLE,
  PLATFORM_NOT_SUPPORTED_ON_ANDROID,
  NO_ID_SPECIFIED,
  IDS_DO_NOT_MATCH,
  ALREADY_INSTALLED,
  INSUFFICIENT_ENGAGEMENT,
  PACKAGE_NAME_OR_START_URL_EMPTY,
  PREVIOUSLY_BLOCKED,
  PREVIOUSLY_IGNORED,
  SHOWING_NATIVE_APP_BANNER,
  SHOWING_WEB_APP_BANNER,
  FAILED_TO_CREATE_BANNER,
  URL_NOT_SUPPORTED_FOR_WEBAPK,
  IN_INCOGNITO,
  NOT_OFFLINE_CAPABLE,
  WAITING_FOR_MANIFEST,
  WAITING_FOR_INSTALLABLE_CHECK,
  NO_GESTURE,
  WAITING_FOR_NATIVE_DATA,
  SHOWING_APP_INSTALLATION_DIALOG,
  MAX_ERROR_CODE,
};

// Logs a message associated with |code| to the devtools console attached to
// |web_contents|. Does nothing if |web_contents| is nullptr.
void LogErrorToConsole(content::WebContents* web_contents,
                       InstallableStatusCode code);

#endif  // CHROME_BROWSER_INSTALLABLE_INSTALLABLE_LOGGING_H_
