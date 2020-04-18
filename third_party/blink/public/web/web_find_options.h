/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FIND_OPTIONS_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FIND_OPTIONS_H_

#include "third_party/blink/public/platform/web_string.h"

namespace blink {

// Options used when performing a find-in-page query.
struct WebFindOptions {
  // Whether to search forward or backward within the page.
  bool forward;

  // Whether search should be case-sensitive.
  bool match_case;

  // Whether this operation is the first request or a follow-up.
  bool find_next;

  // Whether this operation should look for matches only at the start of words.
  bool word_start;

  // When combined with wordStart, accepts a match in the middle of a word if
  // the match begins with an uppercase letter followed by a lowercase or
  // non-letter. Accepts several other intra-word matches.
  bool medial_capital_as_word_start;

  // Force a re-search of the frame: typically used when forcing a re-search
  // after the frame navigates.
  bool force;

  WebFindOptions()
      : forward(true),
        match_case(false),
        find_next(false),
        word_start(false),
        medial_capital_as_word_start(false),
        force(false) {}
};

}  // namespace blink

#endif
