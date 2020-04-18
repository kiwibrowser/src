// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_TESTING_INTERNALS_NAVIGATOR_CONTENT_UTILS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_TESTING_INTERNALS_NAVIGATOR_CONTENT_UTILS_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class Document;
class Internals;

class InternalsNavigatorContentUtils {
  STATIC_ONLY(InternalsNavigatorContentUtils);

 public:
  static void setNavigatorContentUtilsClientMock(Internals&, Document*);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_NAVIGATORCONTENTUTILS_TESTING_INTERNALS_NAVIGATOR_CONTENT_UTILS_H_
