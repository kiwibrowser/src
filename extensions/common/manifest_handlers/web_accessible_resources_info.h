// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_MANIFEST_HANDLERS_WEB_ACCESSIBLE_RESOURCES_INFO_H_
#define EXTENSIONS_COMMON_MANIFEST_HANDLERS_WEB_ACCESSIBLE_RESOURCES_INFO_H_

#include <string>

#include "base/macros.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handler.h"

namespace extensions {

// A structure to hold the web accessible extension resources
// that may be specified in the manifest of an extension using
// "web_accessible_resources" key.
struct WebAccessibleResourcesInfo : public Extension::ManifestData {
  // Define out of line constructor/destructor to please Clang.
  WebAccessibleResourcesInfo();
  ~WebAccessibleResourcesInfo() override;

  // Returns true if the specified resource is web accessible.
  static bool IsResourceWebAccessible(const Extension* extension,
                                      const std::string& relative_path);

  // Returns true when 'web_accessible_resources' are defined for the extension.
  static bool HasWebAccessibleResources(const Extension* extension);

  // Optional list of web accessible extension resources.
  URLPatternSet web_accessible_resources_;
};

// Parses the "web_accessible_resources" manifest key.
class WebAccessibleResourcesHandler : public ManifestHandler {
 public:
  WebAccessibleResourcesHandler();
  ~WebAccessibleResourcesHandler() override;

  bool Parse(Extension* extension, base::string16* error) override;

 private:
  base::span<const char* const> Keys() const override;

  DISALLOW_COPY_AND_ASSIGN(WebAccessibleResourcesHandler);
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_MANIFEST_HANDLERS_WEB_ACCESSIBLE_RESOURCES_INFO_H_
