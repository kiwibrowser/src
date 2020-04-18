// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testClickBreadcrumb(done) {
  test.setupAndWaitUntilReady()
      .then(() => {
        // Reset metrics.
        chrome.metricsPrivate.userActions_ = [];
        // Click first row which is 'photos' dir, wait for breadcrumb to show.
        assertTrue(test.fakeMouseDoubleClick('#file-list li.table-row'));
        return test.waitForElement(
            '#location-breadcrumbs .breadcrumb-path:nth-of-type(2)');
      })
      .then(result => {
        // Click breadcrumb to return to parent dir.
        assertTrue(test.fakeMouseClick(
            '#location-breadcrumbs .breadcrumb-path:nth-of-type(1)'));
        return test.waitForFiles(
            test.TestEntryInfo.getExpectedRows(test.BASIC_LOCAL_ENTRY_SET));
      })
      .then(result => {
        assertArrayEquals(
            ['FileBrowser.ClickBreadcrumbs'],
            chrome.metricsPrivate.userActions_);
        done();
      });
}
