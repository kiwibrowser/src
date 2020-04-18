// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var RoleType = chrome.automation.RoleType;

var allTests = [
  function boundsForRange() {
    function getNthListItemInlineTextBox(index) {
      var list = rootNode.find({role: RoleType.LIST});
      var listItem = list.children[index];
      assertEq(RoleType.LIST_ITEM, listItem.role);
      var staticText = listItem.children[1];
      assertEq(RoleType.STATIC_TEXT, staticText.role);
      var inlineTextBox = staticText.firstChild;
      assertEq(RoleType.INLINE_TEXT_BOX, inlineTextBox.role);
      return inlineTextBox;
    }

    // Left-to-right.
    var ltr = getNthListItemInlineTextBox(0);
    var bounds = ltr.location;
    var firstHalf = ltr.boundsForRange(0, 4);
    var secondHalf = ltr.boundsForRange(4, ltr.name.length);
    assertEq(bounds.top, firstHalf.top);
    assertEq(bounds.left, firstHalf.left);
    assertEq(bounds.height, firstHalf.height);
    assertEq(bounds.top, secondHalf.top);
    assertEq(bounds.height, secondHalf.height);
    assertTrue(secondHalf.left > bounds.left);
    assertTrue(firstHalf.width < bounds.width);
    assertTrue(secondHalf.width < bounds.width);
    assertTrue(Math.abs(bounds.width - firstHalf.width - secondHalf.width) < 3);

    // Right-to-left.
    var rtl = getNthListItemInlineTextBox(1);
    bounds = rtl.location;
    firstHalf = rtl.boundsForRange(0, 4);
    secondHalf = rtl.boundsForRange(4, rtl.name.length);
    assertEq(bounds.top, secondHalf.top);
    assertTrue(Math.abs(bounds.left - secondHalf.left) < 3);
    assertEq(bounds.height, secondHalf.height);
    assertEq(bounds.top, firstHalf.top);
    assertEq(bounds.height, firstHalf.height);
    assertTrue(firstHalf.left > bounds.left);
    assertTrue(secondHalf.width < bounds.width);
    assertTrue(firstHalf.width < bounds.width);
    assertTrue(Math.abs(bounds.width - secondHalf.width - firstHalf.width) < 3);

    // Top-to-bottom.
    var ttb = getNthListItemInlineTextBox(2);
    var bounds = ttb.location;
    var firstHalf = ttb.boundsForRange(0, 4);
    var secondHalf = ttb.boundsForRange(4, ttb.name.length);
    assertEq(bounds.left, firstHalf.left);
    assertEq(bounds.top, firstHalf.top);
    assertEq(bounds.width, firstHalf.width);
    assertEq(bounds.left, secondHalf.left);
    assertEq(bounds.width, secondHalf.width);
    assertTrue(secondHalf.top > bounds.top);
    assertTrue(firstHalf.height < bounds.height);
    assertTrue(secondHalf.height < bounds.height);
    assertTrue(Math.abs(bounds.height - firstHalf.height - secondHalf.height)
        < 3);

    // Bottom-to-top.
    var btt = getNthListItemInlineTextBox(3);
    bounds = btt.location;
    firstHalf = btt.boundsForRange(0, 4);
    secondHalf = btt.boundsForRange(4, btt.name.length);
    assertEq(bounds.left, secondHalf.left);
    assertTrue(Math.abs(bounds.top - secondHalf.top) < 3);
    assertEq(bounds.width, secondHalf.width);
    assertEq(bounds.left, firstHalf.left);
    assertEq(bounds.width, firstHalf.width);
    assertTrue(firstHalf.top > bounds.top);
    assertTrue(secondHalf.height < bounds.height);
    assertTrue(firstHalf.height < bounds.height);
    assertTrue(Math.abs(bounds.height - secondHalf.height - firstHalf.height)
        < 3);
    chrome.test.succeed();

  }
];

setUpAndRunTests(allTests, 'bounds_for_range.html');
