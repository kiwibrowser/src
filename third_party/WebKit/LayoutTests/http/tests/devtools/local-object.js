// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests callFunction on local remote objects.\n`);


  var object = [6, 28, 496];
  var localObject = SDK.RemoteObject.fromLocalObject(object);

  function getItem(index) {
    return this[index];
  }

  function getItemCallback(result) {
    TestRunner.addResult('getItem(1) result: ' + result);
  }

  function compareAndSwap(index, value, newValue) {
    if (this[index] !== value)
      throw 'Data corrupted';
    this[index] = newValue;
    return 'Done';
  }

  function compareAndSwapCallback(result) {
    TestRunner.addResult('compareAndSwap(1, 28, 42) result: ' + result.description);
  }

  function exceptionCallback(result, exceptionDetails) {
    TestRunner.addResult('compareAndSwap(1, 28, 42) throws exception: ' + !!exceptionDetails);
  }

  function guessWhat() {
    return 42;
  }

  function guessWhatCallback(result) {
    TestRunner.addResult('guessWhat() result: ' + result.description);
  }

  localObject.callFunctionJSON(getItem, [{value: 1}], getItemCallback);
  localObject.callFunction(compareAndSwap, [{value: 1}, {value: 28}, {value: 42}], compareAndSwapCallback);
  localObject.callFunction(compareAndSwap, [{value: 1}, {value: 28}, {value: 42}], exceptionCallback);
  localObject.callFunction(guessWhat, undefined, guessWhatCallback);
  localObject.callFunction(compareAndSwap, [{value: 0}, {value: 6}, {value: 7}]);
  TestRunner.addResult('Final value of object: [' + object.join(', ') + ']');
  TestRunner.completeTest();
})();
