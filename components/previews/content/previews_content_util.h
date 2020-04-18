// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CONTENT_PREVIEWS_CONTENT_UTIL_H_
#define COMPONENTS_PREVIEWS_CONTENT_PREVIEWS_CONTENT_UTIL_H_

#include "components/previews/core/previews_decider.h"
#include "content/public/common/previews_state.h"

namespace previews {

// Returns whether |previews_state| has any enabled previews.
bool HasEnabledPreviews(content::PreviewsState previews_state);

// Returns the bitmask of enabled client-side previews for |url_request| and
// the current effective network connection given |previews_decider|.
// This handles the mapping of previews::PreviewsType enum values to bitmask
// definitions for content::PreviewsState.
content::PreviewsState DetermineEnabledClientPreviewsState(
    const net::URLRequest& url_request,
    const previews::PreviewsDecider* previews_decider);

// Returns an updated PreviewsState given |previews_state| that has already
// been updated wrt server previews. This should be called at Navigation Commit
// time. It will defer to any server preview set, otherwise it chooses which
// client preview bits to retain for processing the main frame response.
content::PreviewsState DetermineCommittedClientPreviewsState(
    const net::URLRequest& url_request,
    content::PreviewsState previews_state,
    const previews::PreviewsDecider* previews_decider);

// Returns the effective PreviewsType known on a main frame basis given the
// |previews_state| bitmask for the committed main frame. This uses the same
// previews precendence consideration as |DetermineCommittedClientPreviewsState|
// in case it is called on a PreviewsState value that has not been filtered
// through that method.
previews::PreviewsType GetMainFramePreviewsType(
    content::PreviewsState previews_state);

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CONTENT_PREVIEWS_CONTENT_UTIL_H_
