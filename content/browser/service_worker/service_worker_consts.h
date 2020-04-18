// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONSTS_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONSTS_H_

namespace content {

struct ServiceWorkerConsts {
  static const char kBadMessageFromNonWindow[];
  static const char kBadMessageGetRegistrationForReadyDuplicated[];
  static const char kBadMessageImproperOrigins[];
  static const char kBadMessageInvalidURL[];
  static const char kBadNavigationPreloadHeaderValue[];
  static const char kDatabaseErrorMessage[];
  static const char kEnableNavigationPreloadErrorPrefix[];
  static const char kGetNavigationPreloadStateErrorPrefix[];
  static const char kInvalidStateErrorMessage[];
  static const char kNoActiveWorkerErrorMessage[];
  static const char kNoDocumentURLErrorMessage[];
  static const char kSetNavigationPreloadHeaderErrorPrefix[];
  static const char kShutdownErrorMessage[];
  static const char kUserDeniedPermissionMessage[];
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONSTS_H_
