// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/console-exception-while-no-inspector.html');
  TestRunner.addResult(
      `Tests that console will NOT contain stack trace for exception thrown when inspector front-end was closed. Bug 109427. https://bugs.webkit.org/show_bug.cgi?id=109427\n`);
  await TestRunner.waitForEvent(SDK.ConsoleModel.Events.MessageAdded, SDK.consoleModel);

  var message = SDK.consoleModel.messages()[0];
  var stack = message.stackTrace;
  if (stack && stack.callFrames.length)
    TestRunner.addResult('FAIL: found message with stack trace');
  else
    TestRunner.addResult('SUCCESS: message doesn\'t have stack trace');

  TestRunner.addResult('TEST COMPLETE.');
  TestRunner.completeTest();
})();
