// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/web_apps.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/web_application_info.h"
#include "third_party/blink/public/platform/web_icon_sizes_parser.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_node.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

using blink::WebDocument;
using blink::WebElement;
using blink::WebLocalFrame;
using blink::WebNode;
using blink::WebString;

namespace web_apps {
namespace {

// Sizes a single size (the width or height) from a 'sizes' attribute. A size
// matches must match the following regex: [1-9][0-9]*.
int ParseSingleIconSize(const base::StringPiece16& text) {
  // Size must not start with 0, and be between 0 and 9.
  if (text.empty() || !(text[0] >= L'1' && text[0] <= L'9'))
    return 0;

  // Make sure all chars are from 0-9.
  for (size_t i = 1; i < text.length(); ++i) {
    if (!(text[i] >= L'0' && text[i] <= L'9'))
      return 0;
  }
  int output;
  if (!base::StringToInt(text, &output))
    return 0;
  return output;
}

// Parses an icon size. An icon size must match the following regex:
// [1-9][0-9]*x[1-9][0-9]*.
// If the input couldn't be parsed, a size with a width/height == 0 is returned.
gfx::Size ParseIconSize(const base::string16& text) {
  std::vector<base::StringPiece16> sizes = base::SplitStringPiece(
      text, base::string16(1, 'x'),
      base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (sizes.size() != 2)
    return gfx::Size();

  return gfx::Size(ParseSingleIconSize(sizes[0]),
                   ParseSingleIconSize(sizes[1]));
}

void AddInstallIcon(const WebElement& link,
                    std::vector<WebApplicationInfo::IconInfo>* icons) {
  WebString href = link.GetAttribute("href");
  if (href.IsNull() || href.IsEmpty())
    return;

  // Get complete url.
  GURL url = link.GetDocument().CompleteURL(href);
  if (!url.is_valid())
    return;

  WebApplicationInfo::IconInfo icon_info;
  if (link.HasAttribute("sizes")) {
    blink::WebVector<blink::WebSize> icon_sizes =
        blink::WebIconSizesParser::ParseIconSizes(link.GetAttribute("sizes"));
    if (icon_sizes.size() == 1 &&
        icon_sizes[0].width != 0 &&
        icon_sizes[0].height != 0) {
      icon_info.width = icon_sizes[0].width;
      icon_info.height = icon_sizes[0].height;
    }
  }
  icon_info.url = url;
  icons->push_back(icon_info);
}

}  // namespace

bool ParseIconSizes(const base::string16& text,
                    std::vector<gfx::Size>* sizes,
                    bool* is_any) {
  *is_any = false;
  std::vector<base::string16> size_strings = base::SplitString(
      text, base::kWhitespaceASCIIAs16,
      base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (size_t i = 0; i < size_strings.size(); ++i) {
    if (base::EqualsASCII(size_strings[i], "any")) {
      *is_any = true;
    } else {
      gfx::Size size = ParseIconSize(size_strings[i]);
      if (size.width() <= 0 || size.height() <= 0)
        return false;  // Bogus size.
      sizes->push_back(size);
    }
  }
  if (*is_any && !sizes->empty()) {
    // If is_any is true, it must occur by itself.
    return false;
  }
  return (*is_any || !sizes->empty());
}

void ParseWebAppFromWebDocument(WebLocalFrame* frame,
                                WebApplicationInfo* app_info) {
  WebDocument document = frame->GetDocument();
  if (document.IsNull())
    return;

  WebElement head = document.Head();
  if (head.IsNull())
    return;

  GURL document_url = document.Url();
  for (WebNode child = head.FirstChild(); !child.IsNull();
       child = child.NextSibling()) {
    if (!child.IsElementNode())
      continue;
    WebElement elem = child.To<WebElement>();

    if (elem.HasHTMLTagName("link")) {
      std::string rel = elem.GetAttribute("rel").Utf8();
      // "rel" attribute may use either "icon" or "shortcut icon".
      // see also
      //   <http://en.wikipedia.org/wiki/Favicon>
      //   <http://dev.w3.org/html5/spec/Overview.html#rel-icon>
      //
      // Bookmark apps also support "apple-touch-icon" and
      // "apple-touch-icon-precomposed".
      if (base::LowerCaseEqualsASCII(rel, "icon") ||
          base::LowerCaseEqualsASCII(rel, "shortcut icon") ||
          base::LowerCaseEqualsASCII(rel, "apple-touch-icon") ||
          base::LowerCaseEqualsASCII(rel, "apple-touch-icon-precomposed")) {
        AddInstallIcon(elem, &app_info->icons);
      }
    } else if (elem.HasHTMLTagName("meta") && elem.HasAttribute("name")) {
      std::string name = elem.GetAttribute("name").Utf8();
      WebString content = elem.GetAttribute("content");
      if (name == "application-name") {
        app_info->title = content.Utf16();
      } else if (name == "description") {
        app_info->description = content.Utf16();
      } else if (name == "application-url") {
        std::string url = content.Utf8();
        app_info->app_url = document_url.is_valid() ?
            document_url.Resolve(url) : GURL(url);
        if (!app_info->app_url.is_valid())
          app_info->app_url = GURL();
      } else if (name == "mobile-web-app-capable" &&
                 base::LowerCaseEqualsASCII(content.Utf16(), "yes")) {
        app_info->mobile_capable = WebApplicationInfo::MOBILE_CAPABLE;
      } else if (name == "apple-mobile-web-app-capable" &&
                 base::LowerCaseEqualsASCII(content.Utf16(), "yes") &&
                 app_info->mobile_capable ==
                     WebApplicationInfo::MOBILE_CAPABLE_UNSPECIFIED) {
        app_info->mobile_capable = WebApplicationInfo::MOBILE_CAPABLE_APPLE;
      }
    }
  }
}

}  // namespace web_apps
