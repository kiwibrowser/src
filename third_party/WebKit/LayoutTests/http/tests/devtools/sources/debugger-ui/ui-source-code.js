// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests UISourceCode class.\n`);
  await TestRunner.showPanel('sources');

  var MockProject = class extends Workspace.ProjectStore {
    requestFileContent(uri, callback) {
      TestRunner.addResult('Content is requested from SourceCodeProvider.');
      setTimeout(callback.bind(null, 'var x = 0;'), 0);
    }

    mimeType() {
      return 'text/javascript';
    }

    isServiceProject() {
      return false;
    }

    type() {
      return Workspace.projectTypes.Debugger;
    }

    url() {
      return 'mock://debugger-ui/';
    }
  };

  TestRunner.runTestSuite([function testUISourceCode(next) {
    var uiSourceCode = new Workspace.UISourceCode(new MockProject(), 'url', Common.resourceTypes.Script);
    function didRequestContent(callNumber, content) {
      TestRunner.addResult('Callback ' + callNumber + ' is invoked.');
      TestRunner.assertEquals('text/javascript', uiSourceCode.mimeType());
      TestRunner.assertEquals('var x = 0;', content);

      if (callNumber === 3) {
        // Check that sourceCodeProvider.requestContent won't be called anymore.
        uiSourceCode.requestContent().then(function(content) {
          TestRunner.assertEquals('text/javascript', uiSourceCode.mimeType());
          TestRunner.assertEquals('var x = 0;', content);
          next();
        });
      }
    }
    // Check that all callbacks will be invoked.
    uiSourceCode.requestContent().then(didRequestContent.bind(null, 1));
    uiSourceCode.requestContent().then(didRequestContent.bind(null, 2));
    uiSourceCode.requestContent().then(didRequestContent.bind(null, 3));
  }]);
})();
