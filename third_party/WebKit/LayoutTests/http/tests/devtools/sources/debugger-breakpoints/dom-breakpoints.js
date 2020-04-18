// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests DOM breakpoints.`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.showPanel('elements');
  await TestRunner.navigatePromise('resources/dom-breakpoints.html');

  var pane = self.runtime.sharedInstance(BrowserDebugger.DOMBreakpointsSidebarPane);
  var rootElement;
  var outerElement;
  var authorShadowRoot;
  SourcesTestRunner.runDebuggerTestSuite([
    function testInsertChild(next) {
      TestRunner.addResult(
          'Test that \'Subtree Modified\' breakpoint is hit when appending a child.');
      ElementsTestRunner.nodeWithId('rootElement', step2);

      function step2(node) {
        rootElement = node;
        TestRunner.domDebuggerModel.setDOMBreakpoint(
            node, SDK.DOMDebuggerModel.DOMBreakpoint.Type.SubtreeModified);
        TestRunner.addResult(
            'Set \'Subtree Modified\' DOM breakpoint on rootElement.');
        TestRunner.evaluateInPageWithTimeout(
            'appendElement(\'rootElement\', \'childElement\')');
        TestRunner.addResult('Append childElement to rootElement.');
        SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(next);
      }
    },

    function testBreakpointToggle(next) {
      TestRunner.addResult(
          'Test that DOM breakpoint toggles properly using checkbox.');
      var breakpoint = TestRunner.domDebuggerModel.setDOMBreakpoint(
          rootElement,
          SDK.DOMDebuggerModel.DOMBreakpoint.Type.AttributeModified);
      TestRunner.addResult('Set DOM breakpoint.');
      pane._items.get(breakpoint).checkbox.click();
      TestRunner.addResult('Uncheck DOM breakpoint.');
      TestRunner
          .evaluateInPagePromise(
              'modifyAttribute(\'rootElement\', \'data-test-breakpoint-toggle\', \'foo\')')
          .then(step2);
      TestRunner.addResult('DOM breakpoint should not be hit when disabled.');

      function step2() {
        TestRunner.addResult('Check DOM breakpoint.');
        pane._items.get(breakpoint).checkbox.click();
        TestRunner.evaluateInPageWithTimeout(
            'modifyAttribute(\'rootElement\', \'data-test-breakpoint-toggle\', \'bar\')');
        TestRunner.addResult(
            'Test that DOM breakpoint is hit when re-enabled.');
        SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(next);
      }
    },

    function testInsertGrandchild(next) {
      TestRunner.addResult(
          'Test that \'Subtree Modified\' breakpoint is hit when appending a grandchild.');
      TestRunner.evaluateInPageWithTimeout(
          'appendElement(\'childElement\', \'grandchildElement\')');
      TestRunner.addResult('Append grandchildElement to childElement.');
      SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(next);
    },

    function testRemoveChild(next) {
      TestRunner.addResult(
          'Test that \'Subtree Modified\' breakpoint is hit when removing a child.');
      TestRunner.evaluateInPageWithTimeout(
          'removeElement(\'grandchildElement\')');
      TestRunner.addResult('Remove grandchildElement.');
      SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(next);
    },

    function testInnerHTML(next) {
      TestRunner.addResult(
          'Test that \'Subtree Modified\' breakpoint is hit exactly once when setting innerHTML.');
      TestRunner.evaluateInPageWithTimeout(
          'setInnerHTML(\'childElement\', \'<br><br>\')');
      TestRunner.addResult('Set childElement.innerHTML.');
      SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(step2);

      function step2() {
        SourcesTestRunner.waitUntilPaused(step3);
        TestRunner.evaluateInPageWithTimeout('breakDebugger()');
        TestRunner.addResult(
            'Call breakDebugger, expect it to show up in next stack trace.');
      }

      function step3(frames) {
        SourcesTestRunner.captureStackTrace(frames);
        TestRunner.domDebuggerModel.removeDOMBreakpoint(
            rootElement,
            SDK.DOMDebuggerModel.DOMBreakpoint.Type.SubtreeModified);
        SourcesTestRunner.resumeExecution(next);
      }
    },

    function testModifyAttribute(next) {
      TestRunner.addResult(
          'Test that \'Attribute Modified\' breakpoint is hit when modifying attribute.');
      TestRunner.domDebuggerModel.setDOMBreakpoint(
          rootElement,
          SDK.DOMDebuggerModel.DOMBreakpoint.Type.AttributeModified);
      TestRunner.addResult(
          'Set \'Attribute Modified\' DOM breakpoint on rootElement.');
      TestRunner.evaluateInPageWithTimeout(
          'modifyAttribute(\'rootElement\', \'data-test\', \'foo\')');
      TestRunner.addResult('Modify rootElement data-test attribute.');
      SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(step2);

      function step2(callFrames) {
        TestRunner.domDebuggerModel.removeDOMBreakpoint(
            rootElement,
            SDK.DOMDebuggerModel.DOMBreakpoint.Type.AttributeModified);
        next();
      }
    },

    function testModifyAttrNode(next) {
      TestRunner.addResult(
          'Test that \'Attribute Modified\' breakpoint is hit when modifying Attr node.');
      TestRunner.domDebuggerModel.setDOMBreakpoint(
          rootElement,
          SDK.DOMDebuggerModel.DOMBreakpoint.Type.AttributeModified);
      TestRunner.addResult(
          'Set \'Attribute Modified\' DOM breakpoint on rootElement.');
      TestRunner.evaluateInPageWithTimeout(
          'modifyAttrNode(\'rootElement\', \'data-test\', \'bar\')');
      TestRunner.addResult('Modify rootElement data-test attribute.');
      SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(step2);

      function step2(callFrames) {
        TestRunner.domDebuggerModel.removeDOMBreakpoint(
            rootElement,
            SDK.DOMDebuggerModel.DOMBreakpoint.Type.AttributeModified);
        next();
      }
    },

    function testSetAttrNode(next) {
      TestRunner.addResult(
          'Test that \'Attribute Modified\' breakpoint is hit when adding a new Attr node.');
      TestRunner.domDebuggerModel.setDOMBreakpoint(
          rootElement,
          SDK.DOMDebuggerModel.DOMBreakpoint.Type.AttributeModified);
      TestRunner.addResult(
          'Set \'Attribute Modified\' DOM breakpoint on rootElement.');
      TestRunner.evaluateInPageWithTimeout(
          'setAttrNode(\'rootElement\', \'data-foo\', \'bar\')');
      TestRunner.addResult('Modify rootElement data-foo attribute.');
      SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(step2);

      function step2(callFrames) {
        TestRunner.domDebuggerModel.removeDOMBreakpoint(
            rootElement,
            SDK.DOMDebuggerModel.DOMBreakpoint.Type.AttributeModified);
        next();
      }
    },

    function testModifyStyleAttribute(next) {
      TestRunner.addResult(
          'Test that \'Attribute Modified\' breakpoint is hit when modifying style attribute.');
      TestRunner.domDebuggerModel.setDOMBreakpoint(
          rootElement,
          SDK.DOMDebuggerModel.DOMBreakpoint.Type.AttributeModified);
      TestRunner.addResult(
          'Set \'Attribute Modified\' DOM breakpoint on rootElement.');
      TestRunner.evaluateInPageWithTimeout(
          'modifyStyleAttribute(\'rootElement\', \'color\', \'green\')');
      TestRunner.addResult('Modify rootElement style.color attribute.');
      SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(step2);

      function step2(callFrames) {
        TestRunner.domDebuggerModel.removeDOMBreakpoint(
            rootElement,
            SDK.DOMDebuggerModel.DOMBreakpoint.Type.AttributeModified);
        next();
      }
    },

    function testRemoveNode(next) {
      TestRunner.addResult(
          'Test that \'Node Removed\' breakpoint is hit when removing a node.');
      ElementsTestRunner.nodeWithId('elementToRemove', step2);

      function step2(node) {
        TestRunner.domDebuggerModel.setDOMBreakpoint(
            node, SDK.DOMDebuggerModel.DOMBreakpoint.Type.NodeRemoved);
        TestRunner.addResult(
            'Set \'Node Removed\' DOM breakpoint on elementToRemove.');
        TestRunner.evaluateInPageWithTimeout(
            'removeElement(\'elementToRemove\')');
        TestRunner.addResult('Remove elementToRemove.');
        SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(next);
      }
    },

    function testReload(next) {
      TestRunner.addResult(
          'Test that DOM breakpoints are persisted between page reloads.');
      ElementsTestRunner.nodeWithId('rootElement', step2);

      function step2(node) {
        TestRunner.domDebuggerModel.setDOMBreakpoint(
            node, SDK.DOMDebuggerModel.DOMBreakpoint.Type.SubtreeModified);
        TestRunner.addResult(
            'Set \'Subtree Modified\' DOM breakpoint on rootElement.');
        TestRunner.reloadPage(step3);
      }

      function step3() {
        ElementsTestRunner.expandElementsTree(step4);
      }

      function step4() {
        TestRunner.evaluateInPageWithTimeout(
            'appendElement(\'rootElement\', \'childElement\')');
        TestRunner.addResult('Append childElement to rootElement.');
        SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(next);
      }
    },

    function testInsertChildIntoAuthorShadowTree(next) {
      ElementsTestRunner.shadowRootByHostId('hostElement', callback);

      function callback(node) {
        authorShadowRoot = node;
        TestRunner.addResult(
            'Test that \'Subtree Modified\' breakpoint on author shadow root is hit when appending a child.');
        TestRunner.domDebuggerModel.setDOMBreakpoint(
            authorShadowRoot,
            SDK.DOMDebuggerModel.DOMBreakpoint.Type.SubtreeModified);
        TestRunner.addResult(
            'Set \'Subtree Modified\' DOM breakpoint on author shadow root.');
        TestRunner.evaluateInPageWithTimeout(
            'appendElementToOpenShadowRoot(\'childElement\')');
        TestRunner.addResult('Append childElement to author shadow root.');
        SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(next);
      }
    },

    function testReloadWithShadowElementBreakpoint(next) {
      ElementsTestRunner.nodeWithId('outerElement', step1);

      function step1(node) {
        outerElement = node;

        TestRunner.addResult(
            'Test that shadow DOM breakpoints are persisted between page reloads.');
        TestRunner.domDebuggerModel.setDOMBreakpoint(
            outerElement,
            SDK.DOMDebuggerModel.DOMBreakpoint.Type.SubtreeModified);
        TestRunner.addResult(
            'Set \'Subtree Modified\' DOM breakpoint on outerElement.');
        TestRunner.reloadPage(step2);
      }

      function step2() {
        ElementsTestRunner.expandElementsTree(step3);
      }

      function step3() {
        TestRunner.evaluateInPageWithTimeout(
            'appendElementToAuthorShadowTree(\'outerElement\', \'childElement\')');
        TestRunner.addResult('Append childElement to outerElement.');
        SourcesTestRunner.waitUntilPausedAndDumpStackAndResume(next);
      }
    }

  ]);
})();
