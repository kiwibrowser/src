// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/document-write.html');
  TestRunner.addResult(`Tests that console reports zero line number for scripts generated with document.write. https://bugs.webkit.org/show_bug.cgi?id=71099\n`);
  await TestRunner.loadModule('console_test_runner');
  ConsoleTestRunner.dumpConsoleMessages();
  TestRunner.completeTest();
})();
