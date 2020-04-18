// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_GLOBAL_OBJECT_REUSE_POLICY_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_GLOBAL_OBJECT_REUSE_POLICY_H_

namespace blink {

// Indicates whether the global object (i.e. Window instance) associated with
// the previous document in a browsing context was replaced or reused for the
// new Document corresponding to the just-committed navigation; effective in the
// main world and all isolated worlds. WindowProxies are not affected.
//
// TODO(dcheng): Investigate removing the need for this by moving the
// InterfaceProvider plumbing from DidCommitProvisionalLoad to
// RenderFrameImpl::CommitNavigation.
enum class WebGlobalObjectReusePolicy { kCreateNew, kUseExisting };

}  // namespace blink

#endif
