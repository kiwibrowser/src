// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/html_table_row_element.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/html/html_paragraph_element.h"
#include "third_party/blink/renderer/core/html/html_table_element.h"

namespace blink {

// rowIndex
// https://html.spec.whatwg.org/multipage/tables.html#dom-tr-rowindex

TEST(HTMLTableRowElementTest, rowIndex_notInTable) {
  Document* document = Document::CreateForTest();
  HTMLTableRowElement* row = HTMLTableRowElement::Create(*document);
  EXPECT_EQ(-1, row->rowIndex())
      << "rows not in tables should have row index -1";
}

TEST(HTMLTableRowElementTest, rowIndex_directChildOfTable) {
  Document* document = Document::CreateForTest();
  HTMLTableElement* table = HTMLTableElement::Create(*document);
  HTMLTableRowElement* row = HTMLTableRowElement::Create(*document);
  table->AppendChild(row);
  EXPECT_EQ(0, row->rowIndex())
      << "rows that are direct children of a table should have a row index";
}

TEST(HTMLTableRowElementTest, rowIndex_inUnrelatedElementInTable) {
  Document* document = Document::CreateForTest();
  HTMLTableElement* table = HTMLTableElement::Create(*document);
  // Almost any element will do; what's pertinent is that this is not
  // THEAD, TBODY or TFOOT.
  HTMLParagraphElement* paragraph = HTMLParagraphElement::Create(*document);
  HTMLTableRowElement* row = HTMLTableRowElement::Create(*document);
  table->AppendChild(paragraph);
  paragraph->AppendChild(row);
  EXPECT_EQ(-1, row->rowIndex())
      << "rows in a table, but within an unrelated element, should have "
      << "row index -1";
}

}  // namespace blink
