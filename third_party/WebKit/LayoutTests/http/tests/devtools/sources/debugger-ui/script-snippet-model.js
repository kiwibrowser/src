// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests script snippet model.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.loadHTML('<p></p>');

  function evaluateSnippetAndDumpEvaluationDetails(
      uiSourceCode, context, callback) {
    TestRunner.addSniffer(
        Snippets.ScriptSnippetModel.prototype, '_printRunScriptResult',
        dumpResult);
    Snippets.scriptSnippetModel.evaluateScriptSnippet(context, uiSourceCode);
    var target = context.target();
    var mapping = Snippets.scriptSnippetModel._mappingForDebuggerModel.get(
        target.model(SDK.DebuggerModel));
    var evaluationSourceURL = mapping._evaluationSourceURL(uiSourceCode);
    var snippetId =
        Snippets.scriptSnippetModel._snippetIdForUISourceCode.get(uiSourceCode);
    TestRunner.addResult(
        'Last evaluation source url for snippet: ' + evaluationSourceURL);
    TestRunner.assertEquals(
        snippetId,
        Snippets.scriptSnippetModel._snippetIdForSourceURL(evaluationSourceURL),
        'Snippet can not be identified by its evaluation sourceURL.');


    function dumpResult(target, result) {
      TestRunner.addResult('Snippet execution result: ' + result.description);
      callback();
    }
  }

  function resetSnippetsSettings() {
    Snippets.scriptSnippetModel._snippetStorage._lastSnippetIdentifierSetting
        .set(0);
    Snippets.scriptSnippetModel._snippetStorage._snippetsSetting.set([]);
    Snippets.scriptSnippetModel._lastSnippetEvaluationIndexSetting.set(0);
    Snippets.scriptSnippetModel._project.removeProject();
    Snippets.scriptSnippetModel =
        new Snippets.ScriptSnippetModel(Workspace.workspace);
  }

  var workspace = Workspace.workspace;
  SourcesTestRunner.runDebuggerTestSuite([
    function testCreateEditRenameRemove(next) {
      var uiSourceCode1;

      function filterSnippet(uiSourceCode) {
        return uiSourceCode.project().type() ===
            Workspace.projectTypes.Snippets;
      }

      function uiSourceCodeAdded(event) {
        var uiSourceCode = event.data;
        TestRunner.addResult('UISourceCodeAdded: ' + uiSourceCode.name());
      }

      function uiSourceCodeRemoved(event) {
        var uiSourceCode = event.data;
        TestRunner.addResult('UISourceCodeRemoved: ' + uiSourceCode.name());
      }

      workspace.addEventListener(
          Workspace.Workspace.Events.UISourceCodeAdded, uiSourceCodeAdded);
      workspace.addEventListener(
          Workspace.Workspace.Events.UISourceCodeRemoved, uiSourceCodeRemoved);

      async function renameSnippetAndCheckWorkspace(uiSourceCode, snippetName) {
        TestRunner.addResult('Renaming snippet to \'' + snippetName + '\' ...');
        await uiSourceCode.rename(snippetName)
            .then(success => renameCallback(success));

        function renameCallback(success) {
          if (success)
            TestRunner.addResult('Snippet renamed successfully.');
          else
            TestRunner.addResult('Snippet was not renamed.');
        }
        TestRunner.addResult(
            'UISourceCode name is \'' + uiSourceCode.name() + '\' now.');
        TestRunner.addResult(
            'Number of uiSourceCodes in workspace: ' +
            workspace.uiSourceCodes().filter(filterSnippet).length);
        var storageSnippetsCount =
            Snippets.scriptSnippetModel._snippetStorage.snippets().length;
        TestRunner.addResult(
            'Number of snippets in the storage: ' + storageSnippetsCount);
      }

      function contentCallback(content) {
        TestRunner.addResult('Snippet content: ' + content);
      }

      resetSnippetsSettings();

      Snippets.scriptSnippetModel.project()
          .createFile('', null, '')
          .then(step2.bind(this));

      function step2(uiSourceCode) {
        uiSourceCode1 = uiSourceCode;

        uiSourceCode1.requestContent()
            .then(contentCallback)
            .then(contentDumped1);

        function contentDumped1() {
          uiSourceCode1.addRevision('<snippet content>');
          TestRunner.addResult('Snippet content set.');
          uiSourceCode1._requestContentPromise = null;
          uiSourceCode1.contentLoaded = false;
          uiSourceCode1.requestContent()
              .then(contentCallback)
              .then(contentDumped2);
        }

        function contentDumped2() {
          TestRunner.addResult('Snippet1 created.');
          Snippets.scriptSnippetModel.project()
              .createFile('', null, '')
              .then(step3.bind(this));
        }
      }

      async function step3(uiSourceCode) {
        var uiSourceCode2 = uiSourceCode;
        TestRunner.addResult('Snippet2 created.');
        await renameSnippetAndCheckWorkspace(uiSourceCode1, 'foo');
        await renameSnippetAndCheckWorkspace(uiSourceCode1, '   ');
        await renameSnippetAndCheckWorkspace(uiSourceCode1, ' bar ');
        await renameSnippetAndCheckWorkspace(uiSourceCode1, 'foo');
        await renameSnippetAndCheckWorkspace(uiSourceCode2, 'bar');
        await renameSnippetAndCheckWorkspace(uiSourceCode2, 'foo');
        uiSourceCode1._requestContentPromise = null;
        uiSourceCode1.contentLoaded = false;
        uiSourceCode1.requestContent()
            .then(contentCallback)
            .then(onContentDumped);

        function onContentDumped() {
          Snippets.scriptSnippetModel.project().deleteFile(uiSourceCode1);
          Snippets.scriptSnippetModel.project().deleteFile(uiSourceCode2);
          Snippets.scriptSnippetModel.project()
              .createFile('', null, '')
              .then(step4.bind(this));
        }
      }

      function step4(uiSourceCode) {
        var uiSourceCode3 = uiSourceCode;
        TestRunner.addResult('Snippet3 created.');
        Snippets.scriptSnippetModel.project().deleteFile(uiSourceCode3);
        TestRunner.addResult('Snippet3 deleted.');

        TestRunner.addResult(
            'Number of uiSourceCodes in workspace: ' +
            workspace.uiSourceCodes().filter(filterSnippet).length);
        var storageSnippetsCount =
            Snippets.scriptSnippetModel._snippetStorage.snippets().length;
        TestRunner.addResult(
            'Number of snippets in the storage: ' + storageSnippetsCount);

        workspace.removeEventListener(
            Workspace.Workspace.Events.UISourceCodeAdded, uiSourceCodeAdded);
        workspace.removeEventListener(
            Workspace.Workspace.Events.UISourceCodeRemoved,
            uiSourceCodeRemoved);

        next();
      }
    },

    function testEvaluate(next) {
      var uiSourceCode1;
      var uiSourceCode2;
      var uiSourceCode3;
      var context = UI.context.flavor(SDK.ExecutionContext);

      resetSnippetsSettings();
      var snippetScriptMapping =
          Snippets.scriptSnippetModel.snippetScriptMapping(
              SDK.targetManager.models(SDK.DebuggerModel)[0]);

      Snippets.scriptSnippetModel.project()
          .createFile('', null, '')
          .then(step2.bind(this));

      function step2(uiSourceCode) {
        uiSourceCode1 = uiSourceCode;
        uiSourceCode1.rename('Snippet1');
        var content = '';
        content += '// This snippet does nothing.\n';
        content += 'var i = 2+2;\n';
        uiSourceCode1.setWorkingCopy(content);
        Snippets.scriptSnippetModel.project()
            .createFile('', null, '')
            .then(step3.bind(this));
      }

      function step3(uiSourceCode) {
        uiSourceCode2 = uiSourceCode;
        uiSourceCode2.rename('Snippet2');
        content = '';
        content +=
            '// This snippet creates a function that does nothing and returns it.\n';
        content += 'function doesNothing() {\n';
        content += '    var  i = 2+2;\n';
        content += '};\n';
        content += 'doesNothing;\n';
        uiSourceCode2.setWorkingCopy(content);
        Snippets.scriptSnippetModel.project()
            .createFile('', null, '')
            .then(step4.bind(this));
      }

      function step4(uiSourceCode) {
        uiSourceCode3 = uiSourceCode;
        uiSourceCode3.rename('Snippet3');
        content = '';
        content += '// This snippet uses Command Line API.\n';
        content += '$$("p").length';
        uiSourceCode3.setWorkingCopy(content);
        evaluateSnippetAndDumpEvaluationDetails(uiSourceCode1, context, step5);
      }

      function step5() {
        evaluateSnippetAndDumpEvaluationDetails(uiSourceCode2, context, step6);
      }

      function step6() {
        evaluateSnippetAndDumpEvaluationDetails(uiSourceCode1, context, step7);
      }

      function step7() {
        evaluateSnippetAndDumpEvaluationDetails(uiSourceCode3, context, next);
      }
    },

    function testEvaluateEditReload(next) {
      function evaluateSnippetAndReloadPage(uiSourceCode, callback) {
        TestRunner.addSniffer(
            Snippets.ScriptSnippetModel.prototype, '_printRunScriptResult',
            snippetFinished);
        Snippets.scriptSnippetModel.evaluateScriptSnippet(
            UI.context.flavor(SDK.ExecutionContext), uiSourceCode);

        function snippetFinished(result) {
          var script =
              snippetScriptMapping._scriptForUISourceCode.get(uiSourceCode);
          TestRunner.addResult(
              'Snippet execution result: ' + result.description);

          TestRunner.reloadPage(callback);
        }
      }

      resetSnippetsSettings();
      var snippetScriptMapping =
          Snippets.scriptSnippetModel.snippetScriptMapping(
              SDK.targetManager.models(SDK.DebuggerModel)[0]);

      Snippets.scriptSnippetModel.project()
          .createFile('', null, '')
          .then(step3.bind(this));

      function step3(uiSourceCode) {
        var uiSourceCode1 = uiSourceCode;
        uiSourceCode1.rename('Snippet1');
        var content = '';
        content += '// This snippet does nothing.\n';
        content += 'var i = 2+2;\n';
        uiSourceCode1.setWorkingCopy(content);

        evaluateSnippetAndReloadPage(uiSourceCode1, next);
      }
    },

    function testEvaluateInWorker(next) {
      var context;

      TestRunner.addSniffer(
          SDK.RuntimeModel.prototype, '_executionContextCreated',
          contextCreated);
      TestRunner.evaluateInPagePromise(`
          var workerScript = "postMessage('Done.');";
          var blob = new Blob([workerScript], { type: "text/javascript" });
          var worker = new Worker(URL.createObjectURL(blob));
      `);

      function contextCreated() {
        // Take the only execution context from the worker's RuntimeModel.
        context = this.executionContexts()[0];

        resetSnippetsSettings();
        Snippets.scriptSnippetModel.project()
            .createFile('', null, '')
            .then(step2.bind(this));
      }

      function step2(uiSourceCode) {
        uiSourceCode.rename('Snippet1');
        var content = '2+2;\n';
        uiSourceCode.setWorkingCopy(content);
        evaluateSnippetAndDumpEvaluationDetails(uiSourceCode, context, next);
      }
    },

    function testDangerousNames(next) {
      resetSnippetsSettings();

      Snippets.scriptSnippetModel.project()
          .createFile('', null, '')
          .then(step2.bind(this));

      function step2(uiSourceCode) {
        uiSourceCode.rename('toString');
        SourcesTestRunner.showUISourceCode(uiSourceCode, step3.bind(this));
      }

      function step3() {
        Snippets.scriptSnippetModel.project()
            .createFile('', null, '')
            .then(step4.bind(this));
      }

      function step4(uiSourceCode) {
        uiSourceCode.rename('myfile.toString');
        SourcesTestRunner.showUISourceCode(uiSourceCode, next);
      }
    }
  ]);
})();
