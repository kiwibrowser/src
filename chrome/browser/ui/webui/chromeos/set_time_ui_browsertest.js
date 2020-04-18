// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

GEN('#if defined(OS_CHROMEOS)');

/**
 * SetTimeWebUITest tests loading and interacting with the SetTimeUI web UI,
 * which is normally shown as a dialog.
 * @constructor
 * @extends {testing.Test}
 */
function SetTimeWebUITest() {}

SetTimeWebUITest.prototype = {
  __proto__: testing.Test.prototype,

  /**
   * Browse to set time dialog.
   * @override
   */
  browsePreload: 'chrome://set-time/',
};

TEST_F('SetTimeWebUITest', 'testChangeTimezone', function() {
  assertEquals(this.browsePreload, document.location.href);

  var TimeSetter = settime.TimeSetter;

  // Verify timezone.
  TimeSetter.setTimezone('America/New_York');
  expectEquals('America/New_York', $('timezone-select').value);
  TimeSetter.setTimezone('Europe/Moscow');
  expectEquals('Europe/Moscow', $('timezone-select').value);
});

GEN('#endif');
