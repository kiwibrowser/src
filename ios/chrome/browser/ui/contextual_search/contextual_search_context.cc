// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/contextual_search/contextual_search_context.h"

ContextualSearchContext::ContextualSearchContext(
    const std::string& selected_text,
    const bool use_resolved_search_term,
    const GURL& page_url,
    const std::string& encoding)
    : selected_text(selected_text),
      use_resolved_search_term(use_resolved_search_term),
      page_url(page_url),
      encoding(encoding),
      start_offset(-1),
      end_offset(-1) {}

ContextualSearchContext::~ContextualSearchContext() {}

bool ContextualSearchContext::HasSurroundingText() {
  return start_offset != -1 && end_offset != -1 && !surrounding_text.empty();
}
