// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/list_item_ordinal.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/shadow_root_init.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"

namespace blink {

class ListItemOrdinalTest : public EditingTestBase {};

TEST_F(ListItemOrdinalTest, ItemInsertedOrRemoved_ListItemsInSlot) {
  // Note: We should have more than |kLCSTableSizeLimit|(16) child nodes in
  // host to invoke |LazyReattachNodesNaive()|.
  SetBodyContent(
      "<div id=host>"
      "<li id=item1>1</li>"
      "<p>2</p>"
      "<p>3</p>"
      "<p>4</p>"
      "<p>5</p>"
      "<p>6</p>"
      "<p>7</p>"
      "<p>8</p>"
      "<p>9</p>"
      "<p>10</p>"
      "<p>11</p>"
      "<p>12</p>"
      "<p>13</p>"
      "<p>14</p>"
      "<p>15</p>"
      "<p>16</p>"
      "<li id=item2>17</li></div>");
  Element& host = *GetDocument().getElementById("host");
  ShadowRoot& shadow_root =
      host.AttachShadowRootInternal(ShadowRootType::kOpen);
  shadow_root.SetInnerHTMLFromString("<slot></slot>");
  GetDocument().View()->UpdateAllLifecyclePhases();
  ASSERT_FALSE(shadow_root.NeedsSlotAssignmentRecalc());
  Element& list_item1 = *GetDocument().getElementById("item1");
  Element& list_item2 = *GetDocument().getElementById("item2");

  EXPECT_EQ(1, ListItemOrdinal::Get(list_item1)->Value(list_item1));
  EXPECT_EQ(2, ListItemOrdinal::Get(list_item2)->Value(list_item2));
  LayoutObject* layout_object = list_item2.GetLayoutObject();

  // Invokes |ListItemOrdinal::ItemInsertedOrRemoved()|
  list_item1.remove();

  EXPECT_EQ(2, ListItemOrdinal::Get(list_item2)->Value(list_item2))
      << "We have existing number until layout clean.";
  EXPECT_EQ(layout_object, list_item2.GetLayoutObject())
      << "remove() doesn't change layout object for list_item2.";

  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(1, ListItemOrdinal::Get(list_item2)->Value(list_item2))
      << "Update layout should update list item ordinal too.";
}

}  // namespace blink
