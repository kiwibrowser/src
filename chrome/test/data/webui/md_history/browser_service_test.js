// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('BrowserService', function() {
  test('makes correct call to removeVisits', function(done) {
    registerMessageCallback('removeVisits', this, function(toRemove) {
      assertEquals('http://www.example.com', toRemove[0].url);
      assertEquals('https://en.wikipedia.org', toRemove[1].url);
      assertEquals(1234, toRemove[0].timestamps[0]);
      assertEquals(5678, toRemove[1].timestamps[0]);
      done();
    });

    toDelete = [
      createHistoryEntry(1234, 'http://www.example.com'),
      createHistoryEntry(5678, 'https://en.wikipedia.org')
    ];

    md_history.BrowserService.getInstance().deleteItems(toDelete);
  });

  teardown(function() {
    registerMessageCallback('removeVisits', this, undefined);
  });
});
