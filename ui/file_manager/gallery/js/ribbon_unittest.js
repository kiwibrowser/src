// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testEmptySpliceEvent() {
  var dataModel = new cr.ui.ArrayDataModel([]);
  var selectionModel = {
    addEventListener: function() {},
    selectedIndexes: []
  };
  var ribbon = new Ribbon(
      document,
      window,
      dataModel,
      selectionModel,
      null);
  ribbon.enable();
  dataModel.dispatchEvent({type: 'splice', added: [], removed: []});
}
