// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CATALOG_MANIFEST_PROVIDER_H_
#define SERVICES_CATALOG_MANIFEST_PROVIDER_H_

#include <string>

#include "base/component_export.h"

namespace base {
class Value;
}

namespace catalog {

// An interface which can be implemented by a catalog embedder to override
// manifest fetching behavior.
class COMPONENT_EXPORT(CATALOG) ManifestProvider {
 public:
  virtual ~ManifestProvider() {}

  // Retrieves the raw contents of the manifest for application named |name|.
  // Returns true if |name| is known and |*manifest_contents| is populated.
  // returns false otherwise.
  virtual std::unique_ptr<base::Value> GetManifest(const std::string& name) = 0;
};

}  // namespace catalog

#endif  // SERVICES_CATALOG_MANIFEST_PROVIDER_H_
