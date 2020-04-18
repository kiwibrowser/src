(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that CommandLineAPI is presented only while evaluation.');

  await session.evaluate(`
    var methods = ['dir','dirxml','profile','profileEnd','clear','table','keys','values','debug','undebug','monitor','unmonitor','inspect','copy'];

    function presentedAPIMethods() {
      var methodCount = 0;
      for (var method of methods) {
        try {
          if (eval('window.' + method + '&&' + method + '.toString ? ' + method + '.toString().indexOf("[Command Line API]") !== -1 : false'))
            ++methodCount;
        } catch (e) {
        }
      }
      methodCount += eval('"$_" in window ? $_ === 239 : false') ? 1 : 0;
      return methodCount;
    }

    function setPropertyForMethod() {
      window.dir = 42;
    }

    function defineValuePropertyForMethod() {
      Object.defineProperty(window, 'dir', { value: 42 });
    }

    function defineAccessorPropertyForMethod() {
      Object.defineProperty(window, 'dir', { set: function() {}, get: function(){ return 42 } });
    }

    function definePropertiesForMethod() {
      Object.defineProperties(window, { 'dir': { set: function() {}, get: function(){ return 42 } }});
    }

    var builtinGetOwnPropertyDescriptorOnObject;
    var builtinGetOwnPropertyDescriptorOnObjectPrototype;
    var builtinGetOwnPropertyDescriptorOnWindow;

    function redefineGetOwnPropertyDescriptors() {
      builtinGetOwnPropertyDescriptorOnObject = Object.getOwnPropertyDescriptor;
      Object.getOwnPropertyDescriptor = function() {};
      builtinGetOwnPropertyDescriptorOnObjectPrototype = Object.prototype.getOwnPropertyDescriptor;
      Object.prototype.getOwnPropertyDescriptor = function() {};
      builtinGetOwnPropertyDescriptorOnWindow = window.getOwnPropertyDescriptor;
      window.getOwnPropertyDescriptor = function() {};
    }

    function restoreGetOwnPropertyDescriptors() {
      Object.getOwnPropertyDescriptor = builtinGetOwnPropertyDescriptorOnObject;
      Object.prototype.getOwnPropertyDescriptor = builtinGetOwnPropertyDescriptorOnObjectPrototype;
      window.getOwnPropertyDescriptor = builtinGetOwnPropertyDescriptorOnWindow;
    }
  `);

  async function evaluate(expression, includeCommandLineAPI) {
    var response = await dp.Runtime.evaluate({ expression: expression, objectGroup: 'console', includeCommandLineAPI: includeCommandLineAPI });
    return response.result;
  }

  function setLastEvaluationResultTo239() {
    return evaluate('239', false);
  }

  async function runExpressionAndDumpPresentedMethods(expression) {
    testRunner.log(expression);
    await setLastEvaluationResultTo239();
    var result = await evaluate(expression + '; var a = presentedAPIMethods(); a', true);
    testRunner.log(result);
  }

  async function dumpLeftMethods() {
    // Should always be zero.
    await setLastEvaluationResultTo239();
    var result = await evaluate('presentedAPIMethods()', false);
    testRunner.log(result);
  }

  async function dumpDir() {
    // Should always be presented.
    var result = await evaluate('dir', false);
    testRunner.log(result);
  }

  await runExpressionAndDumpPresentedMethods('');
  await dumpLeftMethods();
  await runExpressionAndDumpPresentedMethods('setPropertyForMethod()');
  await dumpLeftMethods();
  await dumpDir();
  await runExpressionAndDumpPresentedMethods('defineValuePropertyForMethod()');
  await dumpLeftMethods();
  await dumpDir();
  await runExpressionAndDumpPresentedMethods('definePropertiesForMethod()');
  await dumpLeftMethods();
  await dumpDir();
  await runExpressionAndDumpPresentedMethods('defineAccessorPropertyForMethod()');
  await dumpLeftMethods();
  await dumpDir();
  await runExpressionAndDumpPresentedMethods('redefineGetOwnPropertyDescriptors()');
  await dumpLeftMethods();
  await dumpDir();
  await evaluate('restoreGetOwnPropertyDescriptors()', false);
  testRunner.completeTest();
})
