// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests construction of RemoteObjects from local values.\n`);
  await TestRunner.loadModule('sdk');

  var expressions = [
    '1n', '42', '\'foo string\'', 'NaN', 'Infinity', '-Infinity', '-0', '[1n, 2, NaN, -0, null, undefined]',
    '({ foo: \'bar\' })', '(function(){ return arguments; })(1,2,3,4)', '(function func() {})', 'new Error(\'errr\')'
  ];

  for (const expression of expressions) {
    const remoteObject = SDK.RemoteObject.fromLocalObject(eval(expression));
    TestRunner.addResult(`Expression: "${expression}"`);
    dumpRemoteObject(remoteObject);
    TestRunner.addResult(``);
    dumpCallArgument(SDK.RemoteObject.toCallArgument(remoteObject));
    TestRunner.addResult(``);
  }

  TestRunner.completeTest();

  // BigInts fail in JSON.stringify(), so manually log properties.
  function dumpRemoteObject(object) {
    const properties = ['type', 'subtype', 'value', 'description', 'hasChildren', 'preview'];
    const methods = ['unserializableValue', 'arrayLength'];
    for (const prop of properties)
      TestRunner.addResult(`${prop}: ${object[prop]}`);
    for (const method of methods)
      TestRunner.addResult(`${method}: ${object[method]()}`);
  }

  function dumpCallArgument(object) {
    TestRunner.addResult(`ToCallArgument:`);
    const properties = ['value', 'unserializableValue'];
    for (const prop of properties)
      TestRunner.addResult(`${prop}: ${object[prop]}`);
  }
})();
