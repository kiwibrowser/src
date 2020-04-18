// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_ISOLATED_WORLD_IDS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_ISOLATED_WORLD_IDS_H_

namespace blink {

enum IsolatedWorldId {
  // Embedder isolated worlds can use IDs in [1, 1<<29).
  kEmbedderWorldIdLimit = (1 << 29),
  kDocumentXMLTreeViewerWorldId,
  kDevToolsFirstIsolatedWorldId,
  kDevToolsLastIsolatedWorldId = kDevToolsFirstIsolatedWorldId + 100,
  kIsolatedWorldIdLimit,
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_ISOLATED_WORLD_IDS_H_
