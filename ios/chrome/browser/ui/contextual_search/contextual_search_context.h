// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_CONTEXT_H_
#define IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_CONTEXT_H_

#include <string>

#include "base/macros.h"
#include "url/gurl.h"

// Encapsulates key parts of a Contextual Search Context, including surrounding
// text.
struct ContextualSearchContext {
  ContextualSearchContext(const std::string& selected_text,
                          const bool use_resolved_search_term,
                          const GURL& page_url,
                          const std::string& encoding);
  ~ContextualSearchContext();

  // Text that was tapped on.
  const std::string selected_text;
  // If the resolved term (rather than the selected text) should be used.
  const bool use_resolved_search_term;
  // URL of the page that was tapped in.
  const GURL page_url;
  // Encoding of the page.
  const std::string encoding;

  // Text surrounding the tapped text.
  base::string16 surrounding_text;
  // Starting offset of |selected_text| within |surrounding_text|.
  int start_offset;
  // End offset of |selected_text| within |surrounding_text|.
  int end_offset;

  // Returns whether the selection needs context resolution. If no,
  // |selected_text| is the correct query, both for searching and displaying and
  // |surrounding_text|, |start_offset|, |end_offset| are irrelevant.
  bool HasSurroundingText();

  DISALLOW_COPY_AND_ASSIGN(ContextualSearchContext);
};

#endif  // IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_CONTEXT_H_
