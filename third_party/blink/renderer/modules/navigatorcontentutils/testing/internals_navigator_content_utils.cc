// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/navigatorcontentutils/testing/internals_navigator_content_utils.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/testing/internals.h"
#include "third_party/blink/renderer/modules/navigatorcontentutils/navigator_content_utils.h"
#include "third_party/blink/renderer/modules/navigatorcontentutils/testing/navigator_content_utils_client_mock.h"

namespace blink {

void InternalsNavigatorContentUtils::setNavigatorContentUtilsClientMock(
    Internals&,
    Document* document) {
  DCHECK(document);
  DCHECK(document->GetPage());
  NavigatorContentUtils* navigator_content_utils =
      NavigatorContentUtils::From(*document->domWindow()->navigator());
  navigator_content_utils->SetClientForTest(
      NavigatorContentUtilsClientMock::Create());
}

}  // namespace blink
