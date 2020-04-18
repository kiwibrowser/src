// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/features.h"

namespace web {
namespace features {

const base::Feature kContextMenuElementPostMessage{
    "ContextMenuElementPostMessage", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kSlimNavigationManager{"SlimNavigationManager",
                                           base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kNewFileDownload{"NewFileDownload",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kWebErrorPages{"WebErrorPages",
                                   base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kWKHTTPSystemCookieStore{"WKHTTPSystemCookieStore",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kCrashOnUnexpectedURLChange{
    "CrashOnUnexpectedURLChange", base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kBrowserContainerFullscreen{
    "BrowserContainerFullscreen", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace web
