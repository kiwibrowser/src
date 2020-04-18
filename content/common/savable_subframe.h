// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SAVABLE_SUBFRAME_H_
#define CONTENT_COMMON_SAVABLE_SUBFRAME_H_

#include "url/gurl.h"

namespace content {

// Information about a subframe being saved as "complete html".
struct SavableSubframe {
  // Original url of the subframe (i.e. based the parent's html sources).
  GURL original_url;

  // Routing ID of the RenderFrame or RenderFrameProxy for the subframe.
  int routing_id;
};

}  // namespace content

#endif  // CONTENT_COMMON_SAVABLE_SUBFRAME_H_
