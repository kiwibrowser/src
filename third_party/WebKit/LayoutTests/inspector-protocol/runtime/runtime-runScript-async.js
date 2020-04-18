(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests that Runtime.compileScript and Runtime.runScript work with awaitPromise flag.`);

  function dumpResult(result) {
    if (result.error) {
      result.error.code = 0;
      testRunner.log(result.error);
      return;
    }
    result = result.result;
    if (result.exceptionDetails) {
      result.exceptionDetails.exceptionId = 0;
      result.exceptionDetails.exception.objectId = 0;
    }
    if (result.exceptionDetails && result.exceptionDetails.scriptId)
      result.exceptionDetails.scriptId = 0;
    if (result.exceptionDetails && result.exceptionDetails.stackTrace) {
      for (var frame of result.exceptionDetails.stackTrace.callFrames)
        frame.scriptId = 0;
    }
    if (result.result && result.result.objectId)
      result.result.objectId = '[ObjectId]';
    testRunner.log(result);
  }

  testRunner.runTestSuite([
    async function testRunAndCompileWithoutAgentEnable() {
      dumpResult(await dp.Runtime.compileScript({expression: '', sourceURL: '', persistScript: true}));
      dumpResult(await dp.Runtime.runScript({scriptId: '1'}));
    },

    async function testSyntaxErrorInScript() {
      await dp.Runtime.enable();
      dumpResult(await dp.Runtime.compileScript({expression: '\n }', sourceURL: 'boo.js', persistScript: true}));
      await dp.Runtime.disable();
    },

    async function testSyntaxErrorInEvalInScript() {
      await dp.Runtime.enable();
      var response = await dp.Runtime.compileScript({expression: '{\n eval(\'\\\n}\')\n}', sourceURL: 'boo.js', persistScript: true});
      dumpResult(await dp.Runtime.runScript({scriptId: response.result.scriptId}));
      await dp.Runtime.disable();
    },

    async function testRunNotCompiledScript() {
      await dp.Runtime.enable();
      dumpResult(await dp.Runtime.runScript({scriptId: '1'}));
      await dp.Runtime.disable();
    },

    async function testRunCompiledScriptAfterAgentWasReenabled() {
      await dp.Runtime.enable();
      var response = await dp.Runtime.compileScript({expression: '{\n eval(\'\\\n}\')\n}', sourceURL: 'boo.js', persistScript: true});
      var scriptId = response.result.scriptId;
      await dp.Runtime.disable();
      dumpResult(await dp.Runtime.runScript({scriptId}));
      await dp.Runtime.enable();
      dumpResult(await dp.Runtime.runScript({scriptId}));
      await dp.Runtime.disable();
    },

    async function testRunScriptWithPreview() {
      await dp.Runtime.enable();
      var response = await dp.Runtime.compileScript({expression: '({a:1})', sourceURL: 'boo.js', persistScript: true});
      dumpResult(await dp.Runtime.runScript({scriptId: response.result.scriptId, generatePreview: true}));
      await dp.Runtime.disable();
    },

    async function testRunScriptReturnByValue() {
      await dp.Runtime.enable();
      var response = await dp.Runtime.compileScript({expression: '({a:1})', sourceURL: 'boo.js', persistScript: true});
      dumpResult(await dp.Runtime.runScript({scriptId: response.result.scriptId, returnByValue: true}));
      await dp.Runtime.disable();
    },

    async function testAwaitNotPromise() {
      await dp.Runtime.enable();
      var response = await dp.Runtime.compileScript({expression: '({a:1})', sourceURL: 'boo.js', persistScript: true});
      dumpResult(await dp.Runtime.runScript({scriptId: response.result.scriptId, awaitPromise: true, returnByValue: true}));
      await dp.Runtime.disable();
    },

    async function testAwaitResolvedPromise() {
      await dp.Runtime.enable();
      var response = await dp.Runtime.compileScript({expression: 'Promise.resolve({a:1})', sourceURL: 'boo.js', persistScript: true});
      dumpResult(await dp.Runtime.runScript({scriptId: response.result.scriptId, awaitPromise: true, returnByValue: true}));
      await dp.Runtime.disable();
    },

    async function testAwaitRejectedPromise() {
      await dp.Runtime.enable();
      var response = await dp.Runtime.compileScript({expression: 'Promise.reject({a:1})', sourceURL: 'boo.js', persistScript: true});
      dumpResult(await dp.Runtime.runScript({scriptId: response.result.scriptId, awaitPromise: true, returnByValue: true}));
      await dp.Runtime.disable();
    }
  ]);
})
