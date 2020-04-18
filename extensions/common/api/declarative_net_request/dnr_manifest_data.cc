// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/dnr_manifest_data.h"

#include "extensions/common/manifest_constants.h"

namespace extensions {
namespace declarative_net_request {

DNRManifestData::DNRManifestData(const ExtensionResource& resource)
    : resource(resource) {}
DNRManifestData::~DNRManifestData() = default;

// static
const ExtensionResource* DNRManifestData::GetRulesetResource(
    const Extension* extension) {
  Extension::ManifestData* data =
      extension->GetManifestData(manifest_keys::kDeclarativeNetRequestKey);
  if (!data)
    return nullptr;
  return &(static_cast<DNRManifestData*>(data)->resource);
}

}  // namespace declarative_net_request
}  // namespace extensions
