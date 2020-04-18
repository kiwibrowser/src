// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/resource_type_struct_traits.h"

namespace mojo {

// static
content::mojom::ResourceType
EnumTraits<content::mojom::ResourceType, content::ResourceType>::ToMojom(
    content::ResourceType input) {
  switch (input) {
    case content::RESOURCE_TYPE_MAIN_FRAME:
      return content::mojom::ResourceType::RESOURCE_TYPE_MAIN_FRAME;
    case content::RESOURCE_TYPE_SUB_FRAME:
      return content::mojom::ResourceType::RESOURCE_TYPE_SUB_FRAME;
    case content::RESOURCE_TYPE_STYLESHEET:
      return content::mojom::ResourceType::RESOURCE_TYPE_STYLESHEET;
    case content::RESOURCE_TYPE_SCRIPT:
      return content::mojom::ResourceType::RESOURCE_TYPE_SCRIPT;
    case content::RESOURCE_TYPE_IMAGE:
      return content::mojom::ResourceType::RESOURCE_TYPE_IMAGE;
    case content::RESOURCE_TYPE_FONT_RESOURCE:
      return content::mojom::ResourceType::RESOURCE_TYPE_FONT_RESOURCE;
    case content::RESOURCE_TYPE_SUB_RESOURCE:
      return content::mojom::ResourceType::RESOURCE_TYPE_SUB_RESOURCE;
    case content::RESOURCE_TYPE_OBJECT:
      return content::mojom::ResourceType::RESOURCE_TYPE_OBJECT;
    case content::RESOURCE_TYPE_MEDIA:
      return content::mojom::ResourceType::RESOURCE_TYPE_MEDIA;
    case content::RESOURCE_TYPE_WORKER:
      return content::mojom::ResourceType::RESOURCE_TYPE_WORKER;
    case content::RESOURCE_TYPE_SHARED_WORKER:
      return content::mojom::ResourceType::RESOURCE_TYPE_SHARED_WORKER;
    case content::RESOURCE_TYPE_PREFETCH:
      return content::mojom::ResourceType::RESOURCE_TYPE_PREFETCH;
    case content::RESOURCE_TYPE_FAVICON:
      return content::mojom::ResourceType::RESOURCE_TYPE_FAVICON;
    case content::RESOURCE_TYPE_XHR:
      return content::mojom::ResourceType::RESOURCE_TYPE_XHR;
    case content::RESOURCE_TYPE_PING:
      return content::mojom::ResourceType::RESOURCE_TYPE_PING;
    case content::RESOURCE_TYPE_SERVICE_WORKER:
      return content::mojom::ResourceType::RESOURCE_TYPE_SERVICE_WORKER;
    case content::RESOURCE_TYPE_CSP_REPORT:
      return content::mojom::ResourceType::RESOURCE_TYPE_CSP_REPORT;
    case content::RESOURCE_TYPE_PLUGIN_RESOURCE:
      return content::mojom::ResourceType::RESOURCE_TYPE_PLUGIN_RESOURCE;
    case content::RESOURCE_TYPE_LAST_TYPE:
      return content::mojom::ResourceType::RESOURCE_TYPE_LAST_TYPE;
  }

  NOTREACHED();
  return content::mojom::ResourceType::RESOURCE_TYPE_MAIN_FRAME;
}
// static
bool EnumTraits<content::mojom::ResourceType, content::ResourceType>::FromMojom(

    content::mojom::ResourceType input,
    content::ResourceType* output) {
  switch (input) {
    case content::mojom::ResourceType::RESOURCE_TYPE_MAIN_FRAME:
      *output = content::RESOURCE_TYPE_MAIN_FRAME;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_SUB_FRAME:
      *output = content::RESOURCE_TYPE_SUB_FRAME;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_STYLESHEET:
      *output = content::RESOURCE_TYPE_STYLESHEET;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_SCRIPT:
      *output = content::RESOURCE_TYPE_SCRIPT;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_IMAGE:
      *output = content::RESOURCE_TYPE_IMAGE;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_FONT_RESOURCE:
      *output = content::RESOURCE_TYPE_FONT_RESOURCE;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_SUB_RESOURCE:
      *output = content::RESOURCE_TYPE_SUB_RESOURCE;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_OBJECT:
      *output = content::RESOURCE_TYPE_OBJECT;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_MEDIA:
      *output = content::RESOURCE_TYPE_MEDIA;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_WORKER:
      *output = content::RESOURCE_TYPE_WORKER;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_SHARED_WORKER:
      *output = content::RESOURCE_TYPE_SHARED_WORKER;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_PREFETCH:
      *output = content::RESOURCE_TYPE_PREFETCH;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_FAVICON:
      *output = content::RESOURCE_TYPE_FAVICON;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_XHR:
      *output = content::RESOURCE_TYPE_XHR;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_PING:
      *output = content::RESOURCE_TYPE_PING;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_SERVICE_WORKER:
      *output = content::RESOURCE_TYPE_SERVICE_WORKER;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_CSP_REPORT:
      *output = content::RESOURCE_TYPE_CSP_REPORT;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_PLUGIN_RESOURCE:
      *output = content::RESOURCE_TYPE_PLUGIN_RESOURCE;
      return true;
    case content::mojom::ResourceType::RESOURCE_TYPE_LAST_TYPE:
      *output = content::RESOURCE_TYPE_LAST_TYPE;
      return true;
  }
  return false;
}

}  // namespace mojo
