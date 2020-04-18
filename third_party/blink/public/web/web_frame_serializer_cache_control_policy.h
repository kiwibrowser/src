// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FRAME_SERIALIZER_CACHE_CONTROL_POLICY_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FRAME_SERIALIZER_CACHE_CONTROL_POLICY_H_

namespace blink {

// WebFrameSerializerCacheControlPolicy configures how WebFrameSerializer
// processes resources with different Cache-Control headers. By default, frame
// serialization encompasses all resources in a page.  If it is desirable to
// serialize only those resources that could be stored by a http cache, the
// other options may be used.
// TODO(dewittj): Add more policies for subframes and subresources.
enum class WebFrameSerializerCacheControlPolicy {
  kNone = 0,
  kFailForNoStoreMainFrame,
  kSkipAnyFrameOrResourceMarkedNoStore,
  kLast = kSkipAnyFrameOrResourceMarkedNoStore,
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FRAME_SERIALIZER_CACHE_CONTROL_POLICY_H_
