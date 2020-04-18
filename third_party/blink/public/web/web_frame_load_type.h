// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FRAME_LOAD_TYPE_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FRAME_LOAD_TYPE_H_

namespace blink {

// The type of load for a navigation.
// TODO(clamy, toyoshim): Currently WebFrameLoadType represents multiple
// concepts that should be orthogonal and could be represented by multiple enum
// classes. We should consider what WebFrameLoadType should represent and what
// shouldn't.
// See https://crbug.com/707715 for further discussion.
//
// kStandard:
//   Follows network and cache protocols, e.g. using cached entries unless
//   they are expired. Used in usual navigations.
// kBackForward:
//   Uses cached entries even if the entries are stale. Used in history back and
//   forward navigations.
// kReload:
//   Revalidates a cached entry for the main resource if one exists, but follows
//   protocols for other subresources. Blink internally uses this for the same
//   page navigation. Also used in optimized reload for mobiles in a field
//   trial.
// kReplaceCurrentItem:
//   Same as Standard, but replaces the current navigation entry in the history.
// kInitialInChildFrame:
//   Used in the first load for a subframe.
// kInitialHistoryLoad:
//   Used in history navigation in a newly created frame.
// kReloadBypassingCache:
//   Bypasses any caches, memory and disk cache in the browser, and caches in
//   proxy servers, to fetch fresh contents directly from the end server.
//   Used in Shift-Reload.
//
// Note: kInitialInChildFrame and kInitialHistoryLoad are used to determine
// the WebHistoryCommitType, but in terms of cache policy, it should work in the
// same manner as Standard and kBackForward respectively.
// kReplaceCurrentItem is used to determine if the current navigation should
// replace the current history item, but in terms of cache policy, it should
// work in the same manner as Standard.
enum class WebFrameLoadType {
  kStandard,
  kBackForward,
  kReload,
  kReplaceCurrentItem,
  kInitialInChildFrame,
  kInitialHistoryLoad,
  kReloadBypassingCache,
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FRAME_LOAD_TYPE_H_
