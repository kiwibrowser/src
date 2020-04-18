// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/manifest_handlers/incognito_info.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

namespace keys = manifest_keys;

IncognitoInfo::IncognitoInfo(Mode mode) : mode(mode) {}

IncognitoInfo::~IncognitoInfo() {
}

// static
bool IncognitoInfo::IsSplitMode(const Extension* extension) {
  IncognitoInfo* info = static_cast<IncognitoInfo*>(
      extension->GetManifestData(keys::kIncognito));
  return info ? info->mode == Mode::SPLIT : false;
}

// static
bool IncognitoInfo::IsIncognitoAllowed(const Extension* extension) {
  IncognitoInfo* info =
      static_cast<IncognitoInfo*>(extension->GetManifestData(keys::kIncognito));
  return info ? info->mode != Mode::NOT_ALLOWED : true;
}

IncognitoHandler::IncognitoHandler() {
}

IncognitoHandler::~IncognitoHandler() {
}

bool IncognitoHandler::Parse(Extension* extension, base::string16* error) {
  // Extensions and Chrome apps default to spanning mode.
  // Hosted and legacy packaged apps default to split mode.
  IncognitoInfo::Mode mode =
      extension->is_hosted_app() || extension->is_legacy_packaged_app()
          ? IncognitoInfo::Mode::SPLIT
          : IncognitoInfo::Mode::SPANNING;
  if (!extension->manifest()->HasKey(keys::kIncognito)) {
    extension->SetManifestData(keys::kIncognito,
                               std::make_unique<IncognitoInfo>(mode));
    return true;
  }

  std::string incognito_string;
  if (!extension->manifest()->GetString(keys::kIncognito, &incognito_string)) {
    *error = base::ASCIIToUTF16(manifest_errors::kInvalidIncognitoBehavior);
    return false;
  }

  if (incognito_string == manifest_values::kIncognitoSplit) {
    mode = IncognitoInfo::Mode::SPLIT;
  } else if (incognito_string == manifest_values::kIncognitoSpanning) {
    mode = IncognitoInfo::Mode::SPANNING;
  } else if (incognito_string == manifest_values::kIncognitoNotAllowed) {
    mode = IncognitoInfo::Mode::NOT_ALLOWED;
  } else {
    *error = base::ASCIIToUTF16(manifest_errors::kInvalidIncognitoBehavior);
    return false;
  }

  extension->SetManifestData(keys::kIncognito,
                             std::make_unique<IncognitoInfo>(mode));
  return true;
}

bool IncognitoHandler::AlwaysParseForType(Manifest::Type type) const {
  return true;
}

base::span<const char* const> IncognitoHandler::Keys() const {
  static constexpr const char* kKeys[] = {keys::kIncognito};
  return kKeys;
}

}  // namespace extensions
