// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_EMBEDDER_MANIFEST_UTILS_H_
#define SERVICES_SERVICE_MANAGER_EMBEDDER_MANIFEST_UTILS_H_

#include "base/values.h"
#include "services/service_manager/embedder/service_manager_embedder_export.h"

namespace service_manager {

// Merges |overlay| (if not null) into |manifest|.
// Uses a strategy similar to base::DictionaryValue::MergeDictionary(), except
// concatenates ListValue contents.
void SERVICE_MANAGER_EMBEDDER_EXPORT
MergeManifestWithOverlay(base::Value* manifest, base::Value* overlay);
}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_EMBEDDER_MANIFEST_UTILS_H_
