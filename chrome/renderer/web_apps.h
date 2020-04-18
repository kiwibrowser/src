// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_WEB_APPS_H_
#define CHROME_RENDERER_WEB_APPS_H_

#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "ui/gfx/geometry/size.h"

namespace blink {
class WebLocalFrame;
}

struct WebApplicationInfo;

namespace web_apps {

// Parses the icon's size attribute as defined in the HTML 5 spec. Returns true
// on success, false on errors. On success either all the sizes specified in
// the attribute are added to sizes, or is_any is set to true.
//
// You shouldn't have a need to invoke this directly, it's public for testing.
bool ParseIconSizes(const base::string16& text, std::vector<gfx::Size>* sizes,
                    bool* is_any);

// Parses |app_info| information out of the document in WebFrame. Note that the
// document may contain no web application information, in which case |app_info|
// is unchanged.
void ParseWebAppFromWebDocument(blink::WebLocalFrame* frame,
                                WebApplicationInfo* app_info);

}  // namespace web_apps

#endif  // CHROME_RENDERER_WEB_APPS_H_
