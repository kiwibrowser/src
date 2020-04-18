// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @type {!FileListSelectionModel} */
var selection_model;

function setUp() {
  selection_model = new FileListSelectionModel();
}

// Verify that all selection and focus is dropped if all selected files get
// deleted.
function testAdjustToReorderingAllAreDeleted() {
  // Set initial selection.
  selection_model.selectedIndexes = [0, 1];
  // Delete the selected items.
  selection_model.adjustToReordering([-1, -1, 0]);
  // Assert nothing is selected or in focus.
  assertArrayEquals([], selection_model.selectedIndexes);
  assertFalse(selection_model.getCheckSelectMode());
}

// Verify that all selection and focus is dropped only if all selected files get
// deleted.
function testAdjustToReorderingSomeAreDeleted() {
  // Set initial selection.
  selection_model.selectedIndexes = [0, 1];
  // Delete the selected items.
  selection_model.adjustToReordering([-1, 0, 1]);
  // Assert selection is not dropped.
  assertArrayEquals([0], selection_model.selectedIndexes);
  assertTrue(selection_model.getCheckSelectMode());
}
