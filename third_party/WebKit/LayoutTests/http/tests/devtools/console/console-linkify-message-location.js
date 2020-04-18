// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Test that console.log() would linkify its location in respect with blackboxing.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.evaluateInPagePromise(`
    function foo()
    {
      console.trace(239);
    }
    //# sourceURL=foo.js
  `);
  await TestRunner.evaluateInPagePromise(`
    function boo()
    {
      foo();
    }
    //# sourceURL=boo.js
  `);

  TestRunner.evaluateInPage('boo()', step1);

  function step1() {
    dumpConsoleMessageURLs();

    TestRunner.addSniffer(Bindings.BlackboxManager.prototype, '_patternChangeFinishedForTests', step2);
    var frameworkRegexString = 'foo\\.js';
    Common.settingForTest('skipStackFramesPattern').set(frameworkRegexString);
  }

  function step2() {
    dumpConsoleMessageURLs();
    TestRunner.addSniffer(Bindings.BlackboxManager.prototype, '_patternChangeFinishedForTests', step3);
    var frameworkRegexString = 'foo\\.js|boo\\.js';
    Common.settingForTest('skipStackFramesPattern').set(frameworkRegexString);
  }

  function step3() {
    dumpConsoleMessageURLs();
    TestRunner.addSniffer(Bindings.BlackboxManager.prototype, '_patternChangeFinishedForTests', step4);
    var frameworkRegexString = '';
    Common.settingForTest('skipStackFramesPattern').set(frameworkRegexString);
  }

  function step4() {
    dumpConsoleMessageURLs();
    TestRunner.completeTest();
  }

  function dumpConsoleMessageURLs() {
    var messages = Console.ConsoleView.instance()._visibleViewMessages;
    for (var i = 0; i < messages.length; ++i) {
      var element = messages[i].toMessageElement();
      var anchor = element.querySelector('.console-message-anchor');
      TestRunner.addResult(anchor.textContent.replace(/VM\d+/g, 'VM'));
    }
  }
})();
