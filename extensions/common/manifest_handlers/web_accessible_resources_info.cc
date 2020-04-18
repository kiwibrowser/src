// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/manifest_handlers/web_accessible_resources_info.h"

#include <stddef.h>

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

namespace keys = manifest_keys;
namespace errors = manifest_errors;

namespace {

const WebAccessibleResourcesInfo* GetResourcesInfo(const Extension* extension) {
  return static_cast<WebAccessibleResourcesInfo*>(
      extension->GetManifestData(keys::kWebAccessibleResources));
}

}  // namespace

WebAccessibleResourcesInfo::WebAccessibleResourcesInfo() {
}

WebAccessibleResourcesInfo::~WebAccessibleResourcesInfo() {
}

// static
bool WebAccessibleResourcesInfo::IsResourceWebAccessible(
    const Extension* extension,
    const std::string& relative_path) {
  const WebAccessibleResourcesInfo* info = GetResourcesInfo(extension);
  return info &&
         extension->ResourceMatches(
             info->web_accessible_resources_, relative_path);
}

// static
bool WebAccessibleResourcesInfo::HasWebAccessibleResources(
    const Extension* extension) {
  const WebAccessibleResourcesInfo* info = GetResourcesInfo(extension);
  return info && info->web_accessible_resources_.size() > 0;
}

WebAccessibleResourcesHandler::WebAccessibleResourcesHandler() {
}

WebAccessibleResourcesHandler::~WebAccessibleResourcesHandler() {
}

bool WebAccessibleResourcesHandler::Parse(Extension* extension,
                                          base::string16* error) {
  std::unique_ptr<WebAccessibleResourcesInfo> info(
      new WebAccessibleResourcesInfo);
  const base::ListValue* list_value = NULL;
  if (!extension->manifest()->GetList(keys::kWebAccessibleResources,
                                      &list_value)) {
    *error = base::ASCIIToUTF16(errors::kInvalidWebAccessibleResourcesList);
    return false;
  }
  for (size_t i = 0; i < list_value->GetSize(); ++i) {
    std::string relative_path;
    if (!list_value->GetString(i, &relative_path)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidWebAccessibleResource, base::NumberToString(i));
      return false;
    }
    URLPattern pattern(URLPattern::SCHEME_EXTENSION);
    if (pattern.Parse(extension->url().spec()) != URLPattern::PARSE_SUCCESS) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidURLPatternError, extension->url().spec());
      return false;
    }
    while (relative_path[0] == '/')
      relative_path = relative_path.substr(1, relative_path.length() - 1);
    pattern.SetPath(pattern.path() + relative_path);
    info->web_accessible_resources_.AddPattern(pattern);
  }
  extension->SetManifestData(keys::kWebAccessibleResources, std::move(info));
  return true;
}

base::span<const char* const> WebAccessibleResourcesHandler::Keys() const {
  static constexpr const char* kKeys[] = {keys::kWebAccessibleResources};
  return kKeys;
}

}  // namespace extensions
