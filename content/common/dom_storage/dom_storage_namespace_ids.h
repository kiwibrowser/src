// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CONTENT_COMMON_DOM_STORAGE_DOM_STORAGE_NAMESPACE_IDS_H_
#define CONTENT_COMMON_DOM_STORAGE_DOM_STORAGE_NAMESPACE_IDS_H_

#include <string>

namespace content {

// The length of session storage namespace ids.
constexpr const size_t kSessionStorageNamespaceIdLength = 36;

// Allocates a unique session storage namespace id.
std::string AllocateSessionStorageNamespaceId();

}  // namespace content

#endif  // CONTENT_COMMON_DOM_STORAGE_DOM_STORAGE_NAMESPACE_IDS_H_
