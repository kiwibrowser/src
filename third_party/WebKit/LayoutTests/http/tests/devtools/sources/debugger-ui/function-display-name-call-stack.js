// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that we display function's "displayName" property in the call stack. CrBug 17356\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.evaluateInPagePromise(`
      var error = false;

      function func1()
      {
          debugger;
      }
      func1.displayName = "my.framework.foo";

      var func2 = (function() {
          var f = function() { func1(); }
          f.displayName = "my.framework.bar";
          return f;
      })();

      var func3 = function() { func2(); }
      func3.__defineGetter__("displayName", function() { error = true; throw new Error("Should not crash!"); });

      function func4() { func3(); }
      func4.__defineGetter__("displayName", function() { error = true; return "FAIL: Should not execute getters!"; });

      function func5() { func4(); }
      func5.displayName = "my.framework.func5";
      func5.__defineSetter__("displayName", function() { error = true; throw new Error("FAIL: Should not call setter!"); });

      function func6() { func5(); }
      func6.displayName = { "foo": 6, toString: function() { error = true; return "FAIL: Should not call toString!"; } };

      function testFunction()
      {
          (function() {
              arguments.callee.displayName = "<anonymous_inside_testFunction>";
              func6();
          })();
          console.assert(!error, "FAIL: No getter or setter or toString should have been called!");
      }
      testFunction.displayName = "<InspectorTest::testFunction>";
  `);

  SourcesTestRunner.startDebuggerTest(step1);

  function step1() {
    SourcesTestRunner.runTestFunctionAndWaitUntilPaused(step2);
  }

  function step2(callFrames) {
    SourcesTestRunner.captureStackTrace(callFrames);
    SourcesTestRunner.completeDebuggerTest();
  }
})();
