/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/editing/iterators/search_buffer.h"

#include "third_party/blink/renderer/core/dom/range.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"

namespace blink {

class SearchBufferTest : public EditingTestBase {
 protected:
  Range* GetBodyRange() const;
};

Range* SearchBufferTest::GetBodyRange() const {
  Range* range(Range::Create(GetDocument()));
  range->selectNode(GetDocument().body());
  return range;
}

TEST_F(SearchBufferTest, FindPlainTextInvalidTarget) {
  static const char* body_content = "<div>foo bar test</div>";
  SetBodyContent(body_content);
  Range* range = GetBodyRange();

  Range* expected_range = range->cloneRange();
  expected_range->collapse(false);

  // A lone lead surrogate (0xDA0A) example taken from fuzz-58.
  static const UChar kInvalid1[] = {0x1461u, 0x2130u, 0x129bu, 0xd711u, 0xd6feu,
                                    0xccadu, 0x7064u, 0xd6a0u, 0x4e3bu, 0x03abu,
                                    0x17dcu, 0xb8b7u, 0xbf55u, 0xfca0u, 0x07fau,
                                    0x0427u, 0xda0au, 0};

  // A lone trailing surrogate (U+DC01).
  static const UChar kInvalid2[] = {0x1461u, 0x2130u, 0x129bu, 0xdc01u,
                                    0xd6feu, 0xccadu, 0};
  // A trailing surrogate followed by a lead surrogate (U+DC03 U+D901).
  static const UChar kInvalid3[] = {0xd800u, 0xdc00u, 0x0061u, 0xdc03u,
                                    0xd901u, 0xccadu, 0};

  static const UChar* invalid_u_strings[] = {kInvalid1, kInvalid2, kInvalid3};

  for (size_t i = 0; i < arraysize(invalid_u_strings); ++i) {
    String invalid_target(invalid_u_strings[i]);
    EphemeralRange found_range =
        FindPlainText(EphemeralRange(range), invalid_target, 0);
    Range* actual_range = Range::Create(
        GetDocument(), found_range.StartPosition(), found_range.EndPosition());
    EXPECT_TRUE(AreRangesEqual(expected_range, actual_range));
  }
}

TEST_F(SearchBufferTest, DisplayInline) {
  SetBodyContent("<span>fi</span>nd");
  GetDocument().UpdateStyleAndLayout();
  auto match_range = FindPlainText(EphemeralRange(GetBodyRange()), "find", 0);
  EXPECT_FALSE(match_range.IsCollapsed());
}

TEST_F(SearchBufferTest, DisplayBlock) {
  SetBodyContent("<div>fi</div>nd");
  GetDocument().UpdateStyleAndLayout();
  auto match_range = FindPlainText(EphemeralRange(GetBodyRange()), "find", 0);
  EXPECT_TRUE(match_range.IsCollapsed());
}

TEST_F(SearchBufferTest, DisplayContents) {
  SetBodyContent("<div style='display: contents'>fi</div>nd");
  GetDocument().UpdateStyleAndLayout();
  auto match_range = FindPlainText(EphemeralRange(GetBodyRange()), "find", 0);
  EXPECT_FALSE(match_range.IsCollapsed());
}

}  // namespace blink
