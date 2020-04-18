// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/visible_units.h"

#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {

class VisibleUnitsWordTest : public EditingTestBase {
 protected:
  std::string DoStartOfWord(
      const std::string& selection_text,
      EWordSide word_side = EWordSide::kNextWordIfOnBoundary) {
    const Position position = SetSelectionTextToBody(selection_text).Base();
    return GetCaretTextFromBody(
        StartOfWord(CreateVisiblePosition(position), word_side)
            .DeepEquivalent());
  }

  std::string DoEndOfWord(
      const std::string& selection_text,
      EWordSide word_side = EWordSide::kNextWordIfOnBoundary) {
    const Position position = SetSelectionTextToBody(selection_text).Base();
    return GetCaretTextFromBody(
        EndOfWord(CreateVisiblePosition(position), word_side).DeepEquivalent());
  }

  std::string DoNextWord(const std::string& selection_text) {
    const Position position = SetSelectionTextToBody(selection_text).Base();
    return GetCaretTextFromBody(
        NextWordPosition(CreateVisiblePosition(position)).DeepEquivalent());
  }

  std::string DoPreviousWord(const std::string& selection_text) {
    const Position position = SetSelectionTextToBody(selection_text).Base();
    const Position result =
        PreviousWordPosition(CreateVisiblePosition(position)).DeepEquivalent();
    if (result.IsNull())
      return GetSelectionTextFromBody(SelectionInDOMTree());
    return GetCaretTextFromBody(result);
  }

  // To avoid name conflict in jumbo build, following functions should be here.
  static VisiblePosition CreateVisiblePositionInDOMTree(
      Node& anchor,
      int offset,
      TextAffinity affinity = TextAffinity::kDownstream) {
    return CreateVisiblePosition(Position(&anchor, offset), affinity);
  }

  static VisiblePositionInFlatTree CreateVisiblePositionInFlatTree(
      Node& anchor,
      int offset,
      TextAffinity affinity = TextAffinity::kDownstream) {
    return CreateVisiblePosition(PositionInFlatTree(&anchor, offset), affinity);
  }
};

class ParameterizedVisibleUnitsWordTest
    : public ::testing::WithParamInterface<bool>,
      private ScopedLayoutNGForTest,
      public VisibleUnitsWordTest {
 protected:
  ParameterizedVisibleUnitsWordTest() : ScopedLayoutNGForTest(GetParam()) {}

  bool LayoutNGEnabled() const { return GetParam(); }
};

INSTANTIATE_TEST_CASE_P(All,
                        ParameterizedVisibleUnitsWordTest,
                        ::testing::Bool());

TEST_F(VisibleUnitsWordTest, StartOfWordBasic) {
  EXPECT_EQ("<p> |(1) abc def</p>", DoStartOfWord("<p>| (1) abc def</p>"));
  EXPECT_EQ("<p> |(1) abc def</p>", DoStartOfWord("<p> |(1) abc def</p>"));
  EXPECT_EQ("<p> (|1) abc def</p>", DoStartOfWord("<p> (|1) abc def</p>"));
  EXPECT_EQ("<p> (1|) abc def</p>", DoStartOfWord("<p> (1|) abc def</p>"));
  EXPECT_EQ("<p> (1)| abc def</p>", DoStartOfWord("<p> (1)| abc def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoStartOfWord("<p> (1) |abc def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoStartOfWord("<p> (1) a|bc def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoStartOfWord("<p> (1) ab|c def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoStartOfWord("<p> (1) abc| def</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoStartOfWord("<p> (1) abc |def</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoStartOfWord("<p> (1) abc d|ef</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoStartOfWord("<p> (1) abc de|f</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoStartOfWord("<p> (1) abc def|</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoStartOfWord("<p> (1) abc def</p>|"));
}

TEST_F(VisibleUnitsWordTest, StartOfWordPreviousWordIfOnBoundaryBasic) {
  EXPECT_EQ("<p> |(1) abc def</p>",
            DoStartOfWord("<p>| (1) abc def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> |(1) abc def</p>",
            DoStartOfWord("<p> |(1) abc def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> |(1) abc def</p>",
            DoStartOfWord("<p> (|1) abc def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (|1) abc def</p>",
            DoStartOfWord("<p> (1|) abc def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1|) abc def</p>",
            DoStartOfWord("<p> (1)| abc def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1)| abc def</p>",
            DoStartOfWord("<p> (1) |abc def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) |abc def</p>",
            DoStartOfWord("<p> (1) a|bc def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) |abc def</p>",
            DoStartOfWord("<p> (1) ab|c def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) |abc def</p>",
            DoStartOfWord("<p> (1) abc| def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc| def</p>",
            DoStartOfWord("<p> (1) abc |def</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc |def</p>",
            DoStartOfWord("<p> (1) abc d|ef</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc |def</p>",
            DoStartOfWord("<p> (1) abc de|f</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc |def</p>",
            DoStartOfWord("<p> (1) abc def|</p>",
                          EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc |def</p>",
            DoStartOfWord("<p> (1) abc def</p>|",
                          EWordSide::kPreviousWordIfOnBoundary));
}

TEST_F(VisibleUnitsWordTest, StartOfWordCrossing) {
  EXPECT_EQ("<b>|abc</b><i>def</i>", DoStartOfWord("<b>abc</b><i>|def</i>"));
  EXPECT_EQ("<b>abc</b><i>def|</i>", DoStartOfWord("<b>abc</b><i>def</i>|"));
}

TEST_F(VisibleUnitsWordTest, StartOfWordFirstLetter) {
  InsertStyleElement("p::first-letter {font-size:200%;}");
  // Note: Expectations should match with |StartOfWordBasic|.
  EXPECT_EQ("<p> |(1) abc def</p>", DoStartOfWord("<p>| (1) abc def</p>"));
  EXPECT_EQ("<p> |(1) abc def</p>", DoStartOfWord("<p> |(1) abc def</p>"));
  EXPECT_EQ("<p> (|1) abc def</p>", DoStartOfWord("<p> (|1) abc def</p>"));
  EXPECT_EQ("<p> (1|) abc def</p>", DoStartOfWord("<p> (1|) abc def</p>"));
  EXPECT_EQ("<p> (1)| abc def</p>", DoStartOfWord("<p> (1)| abc def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoStartOfWord("<p> (1) |abc def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoStartOfWord("<p> (1) a|bc def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoStartOfWord("<p> (1) ab|c def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoStartOfWord("<p> (1) abc| def</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoStartOfWord("<p> (1) abc |def</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoStartOfWord("<p> (1) abc d|ef</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoStartOfWord("<p> (1) abc de|f</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoStartOfWord("<p> (1) abc def|</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoStartOfWord("<p> (1) abc def</p>|"));
}

TEST_F(VisibleUnitsWordTest, StartOfWordShadowDOM) {
  const char* body_content =
      "<a id=host><b id=one>1</b> <b id=two>22</b></a><i id=three>333</i>";
  const char* shadow_content =
      "<p><u id=four>44444</u><content select=#two></content><span id=space> "
      "</span><content select=#one></content><u id=five>55555</u></p>";
  SetBodyContent(body_content);
  ShadowRoot* shadow_root = SetShadowContent(shadow_content, "host");

  Node* one = GetDocument().getElementById("one")->firstChild();
  Node* two = GetDocument().getElementById("two")->firstChild();
  Node* three = GetDocument().getElementById("three")->firstChild();
  Node* four = shadow_root->getElementById("four")->firstChild();
  Node* five = shadow_root->getElementById("five")->firstChild();
  Node* space = shadow_root->getElementById("space")->firstChild();

  EXPECT_EQ(
      Position(one, 0),
      StartOfWord(CreateVisiblePositionInDOMTree(*one, 0)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(space, 1),
      StartOfWord(CreateVisiblePositionInFlatTree(*one, 0)).DeepEquivalent());

  EXPECT_EQ(
      Position(one, 0),
      StartOfWord(CreateVisiblePositionInDOMTree(*one, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(space, 1),
      StartOfWord(CreateVisiblePositionInFlatTree(*one, 1)).DeepEquivalent());

  EXPECT_EQ(
      Position(one, 0),
      StartOfWord(CreateVisiblePositionInDOMTree(*two, 0)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(four, 0),
      StartOfWord(CreateVisiblePositionInFlatTree(*two, 0)).DeepEquivalent());

  EXPECT_EQ(
      Position(one, 0),
      StartOfWord(CreateVisiblePositionInDOMTree(*two, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(four, 0),
      StartOfWord(CreateVisiblePositionInFlatTree(*two, 1)).DeepEquivalent());

  EXPECT_EQ(
      Position(one, 0),
      StartOfWord(CreateVisiblePositionInDOMTree(*three, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(space, 1),
      StartOfWord(CreateVisiblePositionInFlatTree(*three, 1)).DeepEquivalent());

  EXPECT_EQ(
      Position(four, 0),
      StartOfWord(CreateVisiblePositionInDOMTree(*four, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(four, 0),
      StartOfWord(CreateVisiblePositionInFlatTree(*four, 1)).DeepEquivalent());

  EXPECT_EQ(
      Position(space, 1),
      StartOfWord(CreateVisiblePositionInDOMTree(*five, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(space, 1),
      StartOfWord(CreateVisiblePositionInFlatTree(*five, 1)).DeepEquivalent());
}

TEST_F(VisibleUnitsWordTest, StartOfWordTextSecurity) {
  // Note: |StartOfWord()| considers security characters as a sequence "x".
  InsertStyleElement("s {-webkit-text-security:disc;}");
  EXPECT_EQ("|abc<s>foo bar</s>baz", DoStartOfWord("|abc<s>foo bar</s>baz"));
  EXPECT_EQ("|abc<s>foo bar</s>baz", DoStartOfWord("abc|<s>foo bar</s>baz"));
  EXPECT_EQ("|abc<s>foo bar</s>baz", DoStartOfWord("abc<s>|foo bar</s>baz"));
  EXPECT_EQ("|abc<s>foo bar</s>baz", DoStartOfWord("abc<s>f|oo bar</s>baz"));
  EXPECT_EQ("|abc<s>foo bar</s>baz", DoStartOfWord("abc<s>foo| bar</s>baz"));
  EXPECT_EQ("|abc<s>foo bar</s>baz", DoStartOfWord("abc<s>foo |bar</s>baz"));
  EXPECT_EQ("|abc<s>foo bar</s>baz", DoStartOfWord("abc<s>foo bar|</s>baz"));
  EXPECT_EQ("|abc<s>foo bar</s>baz", DoStartOfWord("abc<s>foo bar</s>|baz"));
  EXPECT_EQ("|abc<s>foo bar</s>baz", DoStartOfWord("abc<s>foo bar</s>b|az"));
}

TEST_F(VisibleUnitsWordTest, EndOfWordBasic) {
  EXPECT_EQ("<p> (|1) abc def</p>", DoEndOfWord("<p>| (1) abc def</p>"));
  EXPECT_EQ("<p> (|1) abc def</p>", DoEndOfWord("<p> |(1) abc def</p>"));
  EXPECT_EQ("<p> (1|) abc def</p>", DoEndOfWord("<p> (|1) abc def</p>"));
  EXPECT_EQ("<p> (1)| abc def</p>", DoEndOfWord("<p> (1|) abc def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoEndOfWord("<p> (1)| abc def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoEndOfWord("<p> (1) |abc def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoEndOfWord("<p> (1) a|bc def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoEndOfWord("<p> (1) ab|c def</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoEndOfWord("<p> (1) abc| def</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoEndOfWord("<p> (1) abc |def</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoEndOfWord("<p> (1) abc d|ef</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoEndOfWord("<p> (1) abc de|f</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoEndOfWord("<p> (1) abc def|</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoEndOfWord("<p> (1) abc def</p>|"));
}

TEST_F(VisibleUnitsWordTest, EndOfWordPreviousWordIfOnBoundaryBasic) {
  EXPECT_EQ("<p> |(1) abc def</p>",
            DoEndOfWord("<p>| (1) abc def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> |(1) abc def</p>",
            DoEndOfWord("<p> |(1) abc def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (|1) abc def</p>",
            DoEndOfWord("<p> (|1) abc def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1|) abc def</p>",
            DoEndOfWord("<p> (1|) abc def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1)| abc def</p>",
            DoEndOfWord("<p> (1)| abc def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) |abc def</p>",
            DoEndOfWord("<p> (1) |abc def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc| def</p>",
            DoEndOfWord("<p> (1) a|bc def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc| def</p>",
            DoEndOfWord("<p> (1) ab|c def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc| def</p>",
            DoEndOfWord("<p> (1) abc| def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc |def</p>",
            DoEndOfWord("<p> (1) abc |def</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc def|</p>",
            DoEndOfWord("<p> (1) abc d|ef</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc def|</p>",
            DoEndOfWord("<p> (1) abc de|f</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc def|</p>",
            DoEndOfWord("<p> (1) abc def|</p>",
                        EWordSide::kPreviousWordIfOnBoundary));
  EXPECT_EQ("<p> (1) abc def|</p>",
            DoEndOfWord("<p> (1) abc def</p>|",
                        EWordSide::kPreviousWordIfOnBoundary));
}

TEST_F(VisibleUnitsWordTest, EndOfWordShadowDOM) {
  const char* body_content =
      "<a id=host><b id=one>1</b> <b id=two>22</b></a><i id=three>333</i>";
  const char* shadow_content =
      "<p><u id=four>44444</u><content select=#two></content><span id=space> "
      "</span><content select=#one></content><u id=five>55555</u></p>";
  SetBodyContent(body_content);
  ShadowRoot* shadow_root = SetShadowContent(shadow_content, "host");

  Node* one = GetDocument().getElementById("one")->firstChild();
  Node* two = GetDocument().getElementById("two")->firstChild();
  Node* three = GetDocument().getElementById("three")->firstChild();
  Node* four = shadow_root->getElementById("four")->firstChild();
  Node* five = shadow_root->getElementById("five")->firstChild();

  EXPECT_EQ(
      Position(three, 3),
      EndOfWord(CreateVisiblePositionInDOMTree(*one, 0)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(five, 5),
      EndOfWord(CreateVisiblePositionInFlatTree(*one, 0)).DeepEquivalent());

  EXPECT_EQ(
      Position(three, 3),
      EndOfWord(CreateVisiblePositionInDOMTree(*one, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(five, 5),
      EndOfWord(CreateVisiblePositionInFlatTree(*one, 1)).DeepEquivalent());

  EXPECT_EQ(
      Position(three, 3),
      EndOfWord(CreateVisiblePositionInDOMTree(*two, 0)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(two, 2),
      EndOfWord(CreateVisiblePositionInFlatTree(*two, 0)).DeepEquivalent());

  EXPECT_EQ(
      Position(three, 3),
      EndOfWord(CreateVisiblePositionInDOMTree(*two, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(two, 2),
      EndOfWord(CreateVisiblePositionInFlatTree(*two, 1)).DeepEquivalent());

  EXPECT_EQ(
      Position(three, 3),
      EndOfWord(CreateVisiblePositionInDOMTree(*three, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(three, 3),
      EndOfWord(CreateVisiblePositionInFlatTree(*three, 1)).DeepEquivalent());

  EXPECT_EQ(
      Position(four, 5),
      EndOfWord(CreateVisiblePositionInDOMTree(*four, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(two, 2),
      EndOfWord(CreateVisiblePositionInFlatTree(*four, 1)).DeepEquivalent());

  EXPECT_EQ(
      Position(five, 5),
      EndOfWord(CreateVisiblePositionInDOMTree(*five, 1)).DeepEquivalent());
  EXPECT_EQ(
      PositionInFlatTree(five, 5),
      EndOfWord(CreateVisiblePositionInFlatTree(*five, 1)).DeepEquivalent());
}

TEST_F(VisibleUnitsWordTest, EndOfWordTextSecurity) {
  // Note: |EndOfWord()| considers security characters as a sequence "x".
  InsertStyleElement("s {-webkit-text-security:disc;}");
  EXPECT_EQ("abc<s>foo bar</s>baz|", DoEndOfWord("|abc<s>foo bar</s>baz"));
  EXPECT_EQ("abc<s>foo bar</s>baz|", DoEndOfWord("abc|<s>foo bar</s>baz"));
  EXPECT_EQ("abc<s>foo bar</s>baz|", DoEndOfWord("abc<s>|foo bar</s>baz"));
  EXPECT_EQ("abc<s>foo bar</s>baz|", DoEndOfWord("abc<s>f|oo bar</s>baz"));
  EXPECT_EQ("abc<s>foo bar</s>baz|", DoEndOfWord("abc<s>foo| bar</s>baz"));
  EXPECT_EQ("abc<s>foo bar</s>baz|", DoEndOfWord("abc<s>foo |bar</s>baz"));
  EXPECT_EQ("abc<s>foo bar</s>baz|", DoEndOfWord("abc<s>foo bar|</s>baz"));
  EXPECT_EQ("abc<s>foo bar</s>baz|", DoEndOfWord("abc<s>foo bar</s>|baz"));
  EXPECT_EQ("abc<s>foo bar</s>baz|", DoEndOfWord("abc<s>foo bar</s>b|az"));
}

TEST_P(ParameterizedVisibleUnitsWordTest, NextWordBasic) {
  EXPECT_EQ("<p> (1|) abc def</p>", DoNextWord("<p>| (1) abc def</p>"));
  EXPECT_EQ("<p> (1|) abc def</p>", DoNextWord("<p> |(1) abc def</p>"));
  EXPECT_EQ("<p> (1|) abc def</p>", DoNextWord("<p> (|1) abc def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoNextWord("<p> (1|) abc def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoNextWord("<p> (1)| abc def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoNextWord("<p> (1) |abc def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoNextWord("<p> (1) a|bc def</p>"));
  EXPECT_EQ("<p> (1) abc| def</p>", DoNextWord("<p> (1) ab|c def</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoNextWord("<p> (1) abc| def</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoNextWord("<p> (1) abc |def</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoNextWord("<p> (1) abc d|ef</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoNextWord("<p> (1) abc de|f</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoNextWord("<p> (1) abc def|</p>"));
  EXPECT_EQ("<p> (1) abc def|</p>", DoNextWord("<p> (1) abc def</p>|"));
}

TEST_P(ParameterizedVisibleUnitsWordTest, NextWordCrossingBlock) {
  EXPECT_EQ("<p>abc|</p><p>def</p>", DoNextWord("<p>|abc</p><p>def</p>"));
  EXPECT_EQ("<p>abc</p><p>def|</p>", DoNextWord("<p>abc|</p><p>def</p>"));
}

TEST_P(ParameterizedVisibleUnitsWordTest, NextWordMixedEditability) {
  EXPECT_EQ(
      "<p contenteditable>"
      "abc<b contenteditable=\"false\">def ghi</b>|jkl mno</p>",
      DoNextWord("<p contenteditable>"
                 "|abc<b contenteditable=false>def ghi</b>jkl mno</p>"));
  EXPECT_EQ(
      "<p contenteditable>"
      "abc<b contenteditable=\"false\">def| ghi</b>jkl mno</p>",
      DoNextWord("<p contenteditable>"
                 "abc<b contenteditable=false>|def ghi</b>jkl mno</p>"));
  EXPECT_EQ(
      "<p contenteditable>"
      "abc<b contenteditable=\"false\">def ghi|</b>jkl mno</p>",
      DoNextWord("<p contenteditable>"
                 "abc<b contenteditable=false>def |ghi</b>jkl mno</p>"));
  EXPECT_EQ(
      "<p contenteditable>"
      "abc<b contenteditable=\"false\">def ghi|</b>jkl mno</p>",
      DoNextWord("<p contenteditable>"
                 "abc<b contenteditable=false>def ghi|</b>jkl mno</p>"));
}

TEST_P(ParameterizedVisibleUnitsWordTest, NextWordPunctuation) {
  EXPECT_EQ("abc|.def", DoNextWord("|abc.def"));
  EXPECT_EQ("abc|.def", DoNextWord("a|bc.def"));
  EXPECT_EQ("abc|.def", DoNextWord("ab|c.def"));
  EXPECT_EQ("abc.def|", DoNextWord("abc|.def"));
  EXPECT_EQ("abc.def|", DoNextWord("abc.|def"));

  EXPECT_EQ("abc|...def", DoNextWord("|abc...def"));
  EXPECT_EQ("abc|...def", DoNextWord("a|bc...def"));
  EXPECT_EQ("abc|...def", DoNextWord("ab|c...def"));
  EXPECT_EQ("abc...def|", DoNextWord("abc|...def"));
  EXPECT_EQ("abc...def|", DoNextWord("abc.|..def"));
  EXPECT_EQ("abc...def|", DoNextWord("abc..|.def"));
  EXPECT_EQ("abc...def|", DoNextWord("abc...|def"));
}

TEST_P(ParameterizedVisibleUnitsWordTest, NextWordSkipTab) {
  InsertStyleElement("s { white-space: pre }");
  EXPECT_EQ("<p><s>\t</s>foo|</p>", DoNextWord("<p><s>\t|</s>foo</p>"));
}

//----

TEST_F(VisibleUnitsWordTest, PreviousWordBasic) {
  EXPECT_EQ("<p> |(1) abc def</p>", DoPreviousWord("<p>| (1) abc def</p>"));
  EXPECT_EQ("<p> |(1) abc def</p>", DoPreviousWord("<p> |(1) abc def</p>"));
  EXPECT_EQ("<p> |(1) abc def</p>", DoPreviousWord("<p> (|1) abc def</p>"));
  EXPECT_EQ("<p> (|1) abc def</p>", DoPreviousWord("<p> (1|) abc def</p>"));
  EXPECT_EQ("<p> (|1) abc def</p>", DoPreviousWord("<p> (1)| abc def</p>"));
  EXPECT_EQ("<p> (|1) abc def</p>", DoPreviousWord("<p> (1) |abc def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoPreviousWord("<p> (1) a|bc def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoPreviousWord("<p> (1) ab|c def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoPreviousWord("<p> (1) abc| def</p>"));
  EXPECT_EQ("<p> (1) |abc def</p>", DoPreviousWord("<p> (1) abc |def</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoPreviousWord("<p> (1) abc d|ef</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoPreviousWord("<p> (1) abc de|f</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoPreviousWord("<p> (1) abc def|</p>"));
  EXPECT_EQ("<p> (1) abc |def</p>", DoPreviousWord("<p> (1) abc def</p>|"));
}

}  // namespace blink
