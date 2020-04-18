// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_DNR_MANIFEST_DATA_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_DNR_MANIFEST_DATA_H_

#include "base/macros.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_resource.h"

namespace extensions {
namespace declarative_net_request {

// Manifest data required for the kDeclarativeNetRequestKey manifest
// key.
struct DNRManifestData : Extension::ManifestData {
  explicit DNRManifestData(const ExtensionResource& resource);
  ~DNRManifestData() override;

  // Returns ExtensionResource corresponding to the kDeclarativeNetRequestKey
  // manifest key for the |extension|. Returns null if the extension didn't
  // specify the manifest key.
  // TODO(karandeepb): Change this so that it accepts a const reference.
  static const ExtensionResource* GetRulesetResource(
      const Extension* extension);

  ExtensionResource resource;

  DISALLOW_COPY_AND_ASSIGN(DNRManifestData);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_DNR_MANIFEST_DATA_H_
