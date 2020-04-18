// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/dom_storage/dom_storage_namespace_ids.h"

#include <algorithm>

#include "base/guid.h"

namespace content {

std::string AllocateSessionStorageNamespaceId() {
  std::string guid = base::GenerateGUID();
  std::replace(guid.begin(), guid.end(), '-', '_');
  // The database deserialization code makes assumptions based on this length.
  DCHECK_EQ(guid.size(), kSessionStorageNamespaceIdLength);
  return guid;
}

}  // namespace content
