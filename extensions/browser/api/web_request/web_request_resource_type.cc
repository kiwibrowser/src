// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/web_request/web_request_resource_type.h"

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/api/web_request/web_request_info.h"

namespace extensions {

namespace {

constexpr struct {
  const char* const name;
  const WebRequestResourceType type;
} kResourceTypes[] = {
    {"main_frame", WebRequestResourceType::MAIN_FRAME},
    {"sub_frame", WebRequestResourceType::SUB_FRAME},
    {"stylesheet", WebRequestResourceType::STYLESHEET},
    {"script", WebRequestResourceType::SCRIPT},
    {"image", WebRequestResourceType::IMAGE},
    {"font", WebRequestResourceType::FONT},
    {"object", WebRequestResourceType::OBJECT},
    {"xmlhttprequest", WebRequestResourceType::XHR},
    {"ping", WebRequestResourceType::PING},
    {"csp_report", WebRequestResourceType::CSP_REPORT},
    {"media", WebRequestResourceType::MEDIA},
    {"websocket", WebRequestResourceType::WEB_SOCKET},
    {"other", WebRequestResourceType::OTHER},
};

constexpr size_t kResourceTypesLength = arraysize(kResourceTypes);

static_assert(kResourceTypesLength ==
                  base::strict_cast<size_t>(WebRequestResourceType::OTHER) + 1,
              "Each WebRequestResourceType should have a string name.");

}  // namespace

WebRequestResourceType ToWebRequestResourceType(content::ResourceType type) {
  switch (type) {
    case content::RESOURCE_TYPE_MAIN_FRAME:
      return WebRequestResourceType::MAIN_FRAME;
    case content::RESOURCE_TYPE_SUB_FRAME:
      return WebRequestResourceType::SUB_FRAME;
    case content::RESOURCE_TYPE_STYLESHEET:
      return WebRequestResourceType::STYLESHEET;
    case content::RESOURCE_TYPE_SCRIPT:
      return WebRequestResourceType::SCRIPT;
    case content::RESOURCE_TYPE_IMAGE:
      return WebRequestResourceType::IMAGE;
    case content::RESOURCE_TYPE_FONT_RESOURCE:
      return WebRequestResourceType::FONT;
    case content::RESOURCE_TYPE_SUB_RESOURCE:
      return WebRequestResourceType::OTHER;
    case content::RESOURCE_TYPE_OBJECT:
      return WebRequestResourceType::OBJECT;
    case content::RESOURCE_TYPE_MEDIA:
      return WebRequestResourceType::MEDIA;
    case content::RESOURCE_TYPE_WORKER:
    case content::RESOURCE_TYPE_SHARED_WORKER:
      return WebRequestResourceType::SCRIPT;
    case content::RESOURCE_TYPE_PREFETCH:
      return WebRequestResourceType::OTHER;
    case content::RESOURCE_TYPE_FAVICON:
      return WebRequestResourceType::IMAGE;
    case content::RESOURCE_TYPE_XHR:
      return WebRequestResourceType::XHR;
    case content::RESOURCE_TYPE_PING:
      return WebRequestResourceType::PING;
    case content::RESOURCE_TYPE_SERVICE_WORKER:
      return WebRequestResourceType::SCRIPT;
    case content::RESOURCE_TYPE_CSP_REPORT:
      return WebRequestResourceType::CSP_REPORT;
    case content::RESOURCE_TYPE_PLUGIN_RESOURCE:
      return WebRequestResourceType::OBJECT;
    case content::RESOURCE_TYPE_LAST_TYPE:
      return WebRequestResourceType::OTHER;
  }
  NOTREACHED();
  return WebRequestResourceType::OTHER;
}

const char* WebRequestResourceTypeToString(WebRequestResourceType type) {
  size_t index = base::strict_cast<size_t>(type);
  DCHECK_LT(index, kResourceTypesLength);
  DCHECK_EQ(kResourceTypes[index].type, type);
  return kResourceTypes[index].name;
}

bool ParseWebRequestResourceType(base::StringPiece text,
                                 WebRequestResourceType* type) {
  for (size_t i = 0; i < kResourceTypesLength; ++i) {
    if (text == kResourceTypes[i].name) {
      *type = kResourceTypes[i].type;
      DCHECK_EQ(static_cast<WebRequestResourceType>(i), *type);
      return true;
    }
  }
  return false;
}

}  // namespace extensions
