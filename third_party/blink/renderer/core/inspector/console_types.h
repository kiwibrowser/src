// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_CONSOLE_TYPES_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_CONSOLE_TYPES_H_

namespace blink {

enum MessageSource {
  kXMLMessageSource,
  kJSMessageSource,
  kNetworkMessageSource,
  kConsoleAPIMessageSource,
  kStorageMessageSource,
  kAppCacheMessageSource,
  kRenderingMessageSource,
  kSecurityMessageSource,
  kOtherMessageSource,
  kDeprecationMessageSource,
  kWorkerMessageSource,
  kViolationMessageSource,
  kInterventionMessageSource,
  kRecommendationMessageSource
};

enum MessageLevel {
  kVerboseMessageLevel,
  kInfoMessageLevel,
  kWarningMessageLevel,
  kErrorMessageLevel
};
}

#endif  // !defined(ConsoleTypes_h)
