// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_NAVIGATION_STATE_H_
#define CONTENT_PUBLIC_RENDERER_NAVIGATION_STATE_H_

#include <string>

#include "content/common/content_export.h"
#include "ui/base/page_transition_types.h"

namespace content {

// NavigationState is the portion of DocumentState that is affected by
// in-document navigation.
// TODO(simonjam): Move this to HistoryItem's ExtraData.
class CONTENT_EXPORT NavigationState {
 public:
  virtual ~NavigationState();

  // Contains the transition type that the browser specified when it
  // initiated the load.
  virtual ui::PageTransition GetTransitionType() = 0;

  // True iff the frame's navigation was within the same document.
  virtual bool WasWithinSameDocument() = 0;

  // True if this navigation was not initiated via WebFrame::LoadRequest.
  virtual bool IsContentInitiated() = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_NAVIGATION_STATE_H
