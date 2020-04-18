(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that Runtime.callFunctionOn works with awaitPromise flag.');

  function dumpResult(result) {
    if (result.exceptionDetails && result.exceptionDetails.scriptId)
      result.exceptionDetails.scriptId = 0;
    if (result.result && result.result.objectId)
      result.result.objectId = '[ObjectId]';
    if (result.exceptionDetails) {
      result.exceptionDetails.exceptionId = 0;
      result.exceptionDetails.exception.objectId = 0;
    }
    testRunner.log(result);
  }

  async function callFunctionOn(objectExpression, functionDeclaration, argumentExpressions, returnByValue, generatePreview, awaitPromise) {
    var objectId = (await dp.Runtime.evaluate({ expression: objectExpression })).result.result.objectId;

    var callArguments = [];
    for (var argumentExpression of argumentExpressions) {
      var result = (await dp.Runtime.evaluate({ expression: argumentExpression })).result.result;
      if (result.objectId) {
        callArguments.push({ objectId: result.objectId });
      } else if (result.value) {
        callArguments.push({ value: result.value })
      } else if (result.unserializableValue) {
        callArguments.push({ unserializableValue: result.unserializableValue });
      } else if (result.type === 'undefined') {
        callArguments.push({});
      } else {
        testRunner.log('Unexpected argument object:');
        testRunner.log(result);
        testRunner.completeTest();
      }
    }

    return dp.Runtime.callFunctionOn({ objectId, functionDeclaration, arguments: callArguments, returnByValue, generatePreview, awaitPromise });
  }

  await testRunner.runTestSuite([
    async function testArguments() {
      var result = await callFunctionOn(
          '({a : 1})',
          'function(arg1, arg2, arg3, arg4) { return \'\' + arg1 + \'|\' + arg2 + \'|\' + arg3 + \'|\' + arg4; }',
          [ 'undefined', 'NaN', '({a:2})', 'window'],
          /* returnByValue */ true,
          /* generatePreview */ false,
          /* awaitPromise */ false);
      dumpResult(result.result);
    },

    async function testSyntaxErrorInFunction() {
      var result = await callFunctionOn(
          '({a : 1})',
          '\n  }',
          [],
          /* returnByValue */ false,
          /* generatePreview */ false,
          /* awaitPromise */ true);
      dumpResult(result.result);
    },

    async function testExceptionInFunctionExpression() {
      var result = await callFunctionOn(
          '({a : 1})',
          '(function() { throw new Error() })()',
          [],
          /* returnByValue */ false,
          /* generatePreview */ false,
          /* awaitPromise */ true);
      dumpResult(result.result);
    },

    async function testFunctionReturnNotPromise() {
      var result = await callFunctionOn(
          '({a : 1})',
          '(function() { return 239; })',
          [],
          /* returnByValue */ true,
          /* generatePreview */ false,
          /* awaitPromise */ true);
      dumpResult(result.result);
    },

    async function testFunctionReturnResolvedPromiseReturnByValue() {
      var result = await callFunctionOn(
          '({a : 1})',
          '(function(arg) { return Promise.resolve({a : this.a + arg.a}); })',
          [ '({a:2})' ],
          /* returnByValue */ true,
          /* generatePreview */ false,
          /* awaitPromise */ true);
      dumpResult(result.result);
    },

    async function testFunctionReturnResolvedPromiseWithPreview() {
      var result = await callFunctionOn(
          '({a : 1})',
          '(function(arg) { return Promise.resolve({a : this.a + arg.a}); })',
          [ '({a:2})' ],
          /* returnByValue */ false,
          /* generatePreview */ true,
          /* awaitPromise */ true);
      dumpResult(result.result);
    },

    async function testFunctionReturnRejectedPromise() {
      var result = await callFunctionOn(
          '({a : 1})',
          '(function(arg) { return Promise.reject({a : this.a + arg.a}); })',
          [ '({a:2})' ],
          /* returnByValue */ true,
          /* generatePreview */ false,
          /* awaitPromise */ true);
      dumpResult(result.result);
    }
  ]);
})
