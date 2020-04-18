// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/clipboard_utils.h"

#include "base/strings/utf_string_conversions.h"
#include "net/base/escape.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"

namespace content {

std::string URLToImageMarkup(const blink::WebURL& url,
                             const blink::WebString& title) {
  std::string markup("<img src=\"");
  markup.append(net::EscapeForHTML(url.GetString().Utf8()));
  markup.append("\"");
  if (!title.IsEmpty()) {
    markup.append(" alt=\"");
    markup.append(net::EscapeForHTML(title.Utf8()));
    markup.append("\"");
  }
  markup.append("/>");
  return markup;
}

}  // namespace content
