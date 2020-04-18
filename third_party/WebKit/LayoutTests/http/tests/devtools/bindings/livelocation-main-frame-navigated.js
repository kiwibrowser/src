// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify that debugger live location gets updated.\n`);
  await TestRunner.loadModule('bindings_test_runner');

  TestRunner.markStep('attachFrame');
  await Promise.all([
    BindingsTestRunner.attachFrame('frame', './resources/sourcemap-frame.html'),
    BindingsTestRunner.waitForSourceMap('sourcemap-script.js.map'),
    BindingsTestRunner.waitForSourceMap('sourcemap-style.css.map'),
  ]);

  var jsLiveLocation = BindingsTestRunner.createDebuggerLiveLocation('js', 'sourcemap-script.js');
  var cssLiveLocation = BindingsTestRunner.createCSSLiveLocation('css', 'sourcemap-style.css');

  TestRunner.markStep('navigateMainFrame');
  var url = TestRunner.url('resources/empty-page.html');
  await TestRunner.navigatePromise(url);
  BindingsTestRunner.dumpLocation(jsLiveLocation);
  BindingsTestRunner.dumpLocation(cssLiveLocation);

  TestRunner.completeTest();
})();
