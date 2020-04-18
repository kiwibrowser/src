// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testCancelCheckSelectModeAfterAction(done) {
  test.setupAndWaitUntilReady()
      .then(() => {
        // Click 2nd last file on checkmark to start check-select-mode.
        assertTrue(test.fakeMouseClick(
            '#file-list li.table-row:nth-of-type(4) .detail-checkmark'));
        return test.waitForElement(
            '#file-list li[selected].table-row:nth-of-type(4)');
      })
      .then(result => {
        // Click last file on checkmark, adds to selection.
        assertTrue(test.fakeMouseClick(
            '#file-list li.table-row:nth-of-type(5) .detail-checkmark'));
        return test.waitForElement(
            '#file-list li[selected].table-row:nth-of-type(5)');
      })
      .then(result => {
        assertEquals(
            2, document.querySelectorAll('#file-list li[selected').length);
        // Click selection menu (3-dots).
        assertTrue(test.fakeMouseClick('#selection-menu-button'));
        return test.waitForElement(
            '#file-context-menu:not([hidden]) ' +
            'cr-menu-item[command="#cut"]:not([disabled])');
      })
      .then(result => {
        // Click 'cut'.
        test.fakeMouseClick('#file-context-menu cr-menu-item[command="#cut"]');
        return test.waitForElement('#file-context-menu[hidden]');
      })
      .then(result => {
        // Click first photos dir in checkmark and make sure 4 and 5 not
        // selected.
        assertTrue(test.fakeMouseClick(
            '#file-list li.table-row:nth-of-type(1) .detail-checkmark'));
        return test.waitForElement(
            '#file-list li[selected].table-row:nth-of-type(1)');
      })
      .then(result => {
        assertEquals(
            1, document.querySelectorAll('#file-list li[selected]').length);
        done();
      });
}
