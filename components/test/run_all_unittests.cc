// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/test/components_test_suite.h"

#if defined(HAS_SERVICE_IN_UNIT_TEST)
#include "components/test/components_unittests_catalog_source.h"  // nogncheck
#include "services/catalog/catalog.h"                             // nogncheck
#endif

int main(int argc, char** argv) {
  // GetLaunchCallback() sets up the environment needed by Catalog.
  base::RunTestSuiteCallback callback = GetLaunchCallback(argc, argv);
#if defined(HAS_SERVICE_IN_UNIT_TEST)
  catalog::Catalog::SetDefaultCatalogManifest(
      components::CreateUnittestsCatalog());
#endif

  return base::LaunchUnitTests(argc, argv, std::move(callback));
}
