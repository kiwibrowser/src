// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests breakpoints are correctly dimmed and restored in JavaScriptSourceFrame during live edit.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.addScriptTag('resources/edit-me-breakpoints.js');

  function pathToFileName(path) {
    return path.substring(path.lastIndexOf('/') + 1).replace(/VM[\d]+/, 'VMXX');
  }

  function dumpBreakpointStorageAndLocations() {
    var breakpointManager = Bindings.breakpointManager;
    var breakpoints = breakpointManager._storage._setting.get();
    TestRunner.addResult('    Dumping breakpoint storage');
    for (var i = 0; i < breakpoints.length; ++i)
      TestRunner.addResult(
          '        ' + pathToFileName(breakpoints[i].url) + ':' + breakpoints[i].lineNumber +
          ', enabled:' + breakpoints[i].enabled);

    locations = breakpointManager.allBreakpointLocations();
    TestRunner.addResult('    Dumping breakpoint locations');
    for (var i = 0; i < locations.length; ++i) {
      var uiLocation = locations[i].uiLocation;
      var uiSourceCode = uiLocation.uiSourceCode;
      var url = uiSourceCode.url();
      var lineNumber = uiLocation.lineNumber;
      var project = uiSourceCode.project();
      TestRunner.addResult(
          '        url: ' + pathToFileName(url) + ', lineNumber: ' + lineNumber + ', project type: ' + project.type());
    }

    breakpoints = breakpointManager._allBreakpoints();
    TestRunner.addResult('    Dumping breakpoints');
    for (var i = 0; i < breakpoints.length; ++i) {
      var breakpoint = breakpoints[i];
      var uiSourceCode = breakpoint._primaryUISourceCode;
      var lineNumber = breakpoint.lineNumber();
      var url = uiSourceCode.url();
      var project = uiSourceCode.project();
      TestRunner.addResult(
          '        url: ' + pathToFileName(url) + ', lineNumber: ' + lineNumber + ', project type: ' + project.type());
    }
  }

  Bindings.breakpointManager._storage._breakpoints = new Map();

  SourcesTestRunner.runDebuggerTestSuite([
    function testEditUndo(next) {
      var javaScriptSourceFrame, uiSourceCode, script, originalJavaScriptSourceFrame, originalUISourceCode;

      SourcesTestRunner.showScriptSource('edit-me-breakpoints.js', didShowScriptSource);

      function didShowScriptSource(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        uiSourceCode = sourceFrame._uiSourceCode;

        TestRunner.addResult('Setting breakpoint:');
        TestRunner.addSniffer(
            Bindings.BreakpointManager.ModelBreakpoint.prototype, '_didSetBreakpointInDebugger', breakpointResolved);
        SourcesTestRunner.setBreakpoint(sourceFrame, 2, '', true);
      }

      function breakpointResolved(callback, breakpointId, locations) {
        var location = locations[0];
        script = TestRunner.debuggerModel.scriptForId(location.scriptId);

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Editing source:');
        SourcesTestRunner.replaceInSource(javaScriptSourceFrame, '}', '}//');

        originalUISourceCode = Bindings.debuggerWorkspaceBinding.rawLocationToUILocation(location).uiSourceCode;
        SourcesTestRunner.showUISourceCode(originalUISourceCode, didShowOriginalUISourceCode);
      }

      function didShowOriginalUISourceCode(sourceFrame) {
        originalJavaScriptSourceFrame = sourceFrame;
        TestRunner.assertTrue(
            originalJavaScriptSourceFrame !== javaScriptSourceFrame,
            'Edited and original javaScriptSourceFrames should differ.');
        TestRunner.assertTrue(
            originalUISourceCode !== uiSourceCode, 'Edited and original uiSourceCodes should differ.');

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Undoing source editing:');
        TestRunner.addSniffer(
            Bindings.BreakpointManager.ModelBreakpoint.prototype, '_didSetBreakpointInDebugger',
            breakpointResolvedAgain);
        SourcesTestRunner.undoSourceEditing(javaScriptSourceFrame);
      }

      function breakpointResolvedAgain() {
        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Finally removing breakpoint:');
        SourcesTestRunner.removeBreakpoint(javaScriptSourceFrame, 2);

        dumpBreakpointStorageAndLocations();
        next();
      }
    },

    function testEditCommit(next) {
      var javaScriptSourceFrame, uiSourceCode, script, originalJavaScriptSourceFrame, originalUISourceCode;

      SourcesTestRunner.showScriptSource('edit-me-breakpoints.js', didShowScriptSource);

      function didShowScriptSource(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        uiSourceCode = sourceFrame._uiSourceCode;

        TestRunner.addResult('Setting breakpoint:');
        TestRunner.addSniffer(
            Bindings.BreakpointManager.ModelBreakpoint.prototype, '_didSetBreakpointInDebugger', breakpointResolved);
        SourcesTestRunner.setBreakpoint(sourceFrame, 2, '', true);
      }

      function breakpointResolved(callback, breakpointId, locations) {
        var location = locations[0];
        script = TestRunner.debuggerModel.scriptForId(location.scriptId);

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Editing source:');
        SourcesTestRunner.replaceInSource(javaScriptSourceFrame, '}', '}//');

        originalUISourceCode = Bindings.debuggerWorkspaceBinding.rawLocationToUILocation(location).uiSourceCode;
        SourcesTestRunner.showUISourceCode(originalUISourceCode, didShowOriginalUISourceCode);
      }

      function didShowOriginalUISourceCode(sourceFrame) {
        originalJavaScriptSourceFrame = sourceFrame;
        TestRunner.assertTrue(
            originalJavaScriptSourceFrame !== javaScriptSourceFrame,
            'Edited and original javaScriptSourceFrames should differ.');
        TestRunner.assertTrue(
            originalUISourceCode !== uiSourceCode, 'Edited and original uiSourceCodes should differ.');

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Committing edited source:');
        TestRunner.addSniffer(
            Bindings.BreakpointManager.ModelBreakpoint.prototype, '_didSetBreakpointInDebugger',
            breakpointResolvedAgain);
        SourcesTestRunner.commitSource(javaScriptSourceFrame);
      }

      function breakpointResolvedAgain() {
        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Finally removing breakpoint:');
        SourcesTestRunner.removeBreakpoint(javaScriptSourceFrame, 2);

        dumpBreakpointStorageAndLocations();
        next();
      }
    },

    function testEditCommitFailEditCommit(next) {
      var javaScriptSourceFrame, uiSourceCode, script, originalJavaScriptSourceFrame, originalUISourceCode;

      SourcesTestRunner.showScriptSource('edit-me-breakpoints.js', didShowScriptSource);

      function didShowScriptSource(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        uiSourceCode = sourceFrame._uiSourceCode;

        TestRunner.addResult('Setting breakpoint:');
        TestRunner.addSniffer(
            Bindings.BreakpointManager.ModelBreakpoint.prototype, '_didSetBreakpointInDebugger', breakpointResolved);
        SourcesTestRunner.setBreakpoint(sourceFrame, 2, '', true);
      }

      function breakpointResolved(callback, breakpointId, locations) {
        var location = locations[0];
        script = TestRunner.debuggerModel.scriptForId(location.scriptId);

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Editing source:');
        SourcesTestRunner.replaceInSource(javaScriptSourceFrame, '}', '//}');

        originalUISourceCode = Bindings.debuggerWorkspaceBinding.rawLocationToUILocation(location).uiSourceCode;
        SourcesTestRunner.showUISourceCode(originalUISourceCode, didShowOriginalUISourceCode);
      }

      function didShowOriginalUISourceCode(sourceFrame) {
        originalJavaScriptSourceFrame = sourceFrame;
        TestRunner.assertTrue(
            originalJavaScriptSourceFrame !== javaScriptSourceFrame,
            'Edited and original javaScriptSourceFrames should differ.');
        TestRunner.assertTrue(
            originalUISourceCode !== uiSourceCode, 'Edited and original uiSourceCodes should differ.');

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Committing edited source:');
        TestRunner.addSniffer(TestRunner.debuggerModel, '_didEditScriptSource', commitFailed);
        SourcesTestRunner.commitSource(javaScriptSourceFrame);
      }

      function commitFailed(error) {
        TestRunner.assertTrue(!!error, 'Commit should have failed.');
        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Editing source again so that live edit could succeed:');
        SourcesTestRunner.replaceInSource(javaScriptSourceFrame, '//}', '}//');

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Committing edited source again:');
        TestRunner.addSniffer(
            Bindings.BreakpointManager.ModelBreakpoint.prototype, '_didSetBreakpointInDebugger',
            breakpointResolvedAgain);
        SourcesTestRunner.commitSource(javaScriptSourceFrame);
      }

      function breakpointResolvedAgain() {
        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Finally removing breakpoint:');
        SourcesTestRunner.removeBreakpoint(javaScriptSourceFrame, 2);

        dumpBreakpointStorageAndLocations();
        next();
      }
    },

    function testEditCommitFailUndoCommit(next) {
      var javaScriptSourceFrame, uiSourceCode, script, originalJavaScriptSourceFrame, originalUISourceCode;

      SourcesTestRunner.showScriptSource('edit-me-breakpoints.js', didShowScriptSource);

      function didShowScriptSource(sourceFrame) {
        javaScriptSourceFrame = sourceFrame;
        uiSourceCode = sourceFrame._uiSourceCode;

        TestRunner.addResult('Setting breakpoint:');
        TestRunner.addSniffer(
            Bindings.BreakpointManager.ModelBreakpoint.prototype, '_didSetBreakpointInDebugger', breakpointResolved);
        SourcesTestRunner.setBreakpoint(sourceFrame, 2, '', true);
      }

      function breakpointResolved(callback, breakpointId, locations) {
        var location = locations[0];
        script = TestRunner.debuggerModel.scriptForId(location.scriptId);

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Editing source:');
        SourcesTestRunner.replaceInSource(javaScriptSourceFrame, '}', '//}');

        originalUISourceCode = Bindings.debuggerWorkspaceBinding.rawLocationToUILocation(location).uiSourceCode;
        SourcesTestRunner.showUISourceCode(originalUISourceCode, didShowOriginalUISourceCode);
      }

      function didShowOriginalUISourceCode(sourceFrame) {
        originalJavaScriptSourceFrame = sourceFrame;
        TestRunner.assertTrue(
            originalJavaScriptSourceFrame !== javaScriptSourceFrame,
            'Edited and original javaScriptSourceFrames should differ.');
        TestRunner.assertTrue(
            originalUISourceCode !== uiSourceCode, 'Edited and original uiSourceCodes should differ.');

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Committing edited source:');
        TestRunner.addSniffer(TestRunner.debuggerModel, '_didEditScriptSource', commitFailed);
        SourcesTestRunner.commitSource(javaScriptSourceFrame);
      }

      function commitFailed(error) {
        TestRunner.assertTrue(!!error, 'Commit should have failed.');
        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Undoing source editing:');
        SourcesTestRunner.undoSourceEditing(javaScriptSourceFrame);

        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Committing edited source again:');
        TestRunner.addSniffer(
            Bindings.BreakpointManager.ModelBreakpoint.prototype, '_didSetBreakpointInDebugger',
            breakpointResolvedAgain);
        SourcesTestRunner.commitSource(javaScriptSourceFrame);
      }

      function breakpointResolvedAgain() {
        dumpBreakpointStorageAndLocations();
        TestRunner.addResult('Finally removing breakpoint:');
        SourcesTestRunner.removeBreakpoint(javaScriptSourceFrame, 2);

        dumpBreakpointStorageAndLocations();
        next();
      }
    },
  ]);
})();
