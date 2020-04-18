// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests JSON parsing.\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('network');

  function check(jsonText) {
    var resultData = SourceFrame.JSONView._extractJSON(jsonText);
    if (!resultData) {
      failure();
      return;
    }

    Formatter.formatterWorkerPool().parseJSONRelaxed(resultData.data).then(success);

    function success(data) {
      if (data === null) {
        failure();
        return;
      }
      resultData.data = data;
      TestRunner.addResult('');
      TestRunner.addResult('Input: ' + cleanData(jsonText));

      TestRunner.addResult('Prefix: ' + cleanData(resultData.prefix));
      TestRunner.addResult('Suffix: ' + cleanData(resultData.suffix));
      TestRunner.addResult('Data: ' + cleanData(JSON.stringify(resultData.data, function(k, v) {
                             if (v === undefined)
                               return '!<undefined>!';
                             else if (v === Infinity)
                               return '!<Infinity>!';
                             else if (v === -Infinity)
                               return '!<-Infinity>!';
                             return v;
                           })));
      runNext();
    }

    function failure() {
      TestRunner.addResult('');
      TestRunner.addResult('Input: ' + cleanData(jsonText));

      TestRunner.addResult('Invalid JSON');
      runNext();
    }
  }
  function cleanData(data) {
    return data.replace(/\0/g, '**NULL**').replace(/\r/g, '**CR**');
  }
  function runNext() {
    if (currentTestIndex < tests.length) {
      currentTestIndex++;
      check(tests[currentTestIndex - 1]);
    } else {
      TestRunner.completeTest();
    }
  }

  var currentTestIndex = 0;
  var tests = [
    'plain text', '{"x": 5, "y": false, "z":"foo"}', '{"bar": [,,,1, 2, 3,,,], "baz": {"data": []}}', '[[],[],[]]',
    '/* GUARD */callback({"a": []]});', 'foo({a = 5});', '(function(){return {"a": []}})()',

    '{"name": "value"}', 'while(1); {"name": "value"}', '[, "foo", -4.2, true, false, null]',
    '[{"foo": {}, "bar": []},[[],{}]]', '/* vanilla */ run ( [1, 2, 3] ) ;', '["A\\"B\\u0020C\\nD\\\\E\\u04ABF"]',
    'Text with with {}) inside', '<html>404 Page not found with foo({}) inside</html>',
    '/* vanilla prefix too large to be considered prefix Ok? */ run([1, 2, 3]); // since it is unlikely JSONP',
    '["This is a really long string"]{"This is also a very long string":"short"}', '{010:4}', '{"foo":bar}',
    '{"foo":<"bar"}', '{"foo":"b\\ar"}', '["foo"]', '{foo:"bar"}', '{ 10 : 4 }', '{ "foo" : 010 }', '{"foo": 3e3}',
    '{"foo": 3e3,}', '[,,false, null, true,1,0,,,]', '[,,,]',
    '{foo:\t{bar: null,\r},bar: [\n],foo1: 1,foo2:\r\n -1,\n\rfoo3: -1e-30,foo4: 1E+30,foo5: -1E+1,' +
        'foo6: "bar","foo7": false,"\\"\\f\\o\\o8": true,\'fo\\\'o9\': undefined,' +
        '"foo10": ["bar", null],"": -Infinity,foo11: {},foo12: [{}],}',
    '', '\n', '[]', '{}', '()', 'foobar', '"foobar"', '"foo" "bar"', '[010, 10, 0x10]',
    '{010: 010, 0x10: 0x10, 10: 10}', '{-010: -010}', '{-0x10: -0x10}', '{-10: -10}',
    '{010: -010, 0x10: -0x10, 10: -10}',
    '[10000, 0x1000000, 0xfEaB3, 01023456, 1234567901, 123.456, -123.456,12.33E-5, 123.456e+5, 123.456e5]',
    '["S\\"0\u0000me\\"teXt"]', '   ', '   [ {} ]  ',

    'var i=0;', 'while(true) {}', 'while(true); {d:5}', 'while(true); {d:-5}', 'while(true); {d:[-5]}',
    'while(true); {d:[-5,-0,03,0xF,0xFF]}', '{d:function () {var i;}}', '{5:d}', '[-{},-[],{},[]]',
    '[-foo, foo, -bar, bar]', '[function(){}, -function(){}]', '{-true:0}', '{true: 0}', '[-true]', '[true]', '[!true]',
    '[~true]', '{length: 0}', '[true, false]', '[true, -false]',
    '[true, false, null, undefined, {}, [], "", "1", \'\', \'1\', 1, -1, -0, 0]', '[-{}, !{}, -[{}], ![{}]]',
    '[-1, -0, +1, +0, +-1, -!~0]', '[0xFF, -0xFF, +0xFF, 1e1, -1e1, 1E-1, -1E-1, 010, -010, +010]',
    // Test from json5 ( https://github.com/aseemk/json5 )
    `{
            foo: 'bar',
            while: true,

            this: 'is a \\
multi-line string',

            // this is an inline comment
            here: 'is another', // inline comment

            /* this is a block comment
               that continues on another line */

            hex: 0xDEADbeef,
            half: .5,
            delta: +10,
            to: Infinity,   // and beyond!

            finally: 'a trailing comma',
            oh: [
                "we shouldn't forget",
                'arrays can have',
                'trailing commas too',
            ],
        }`
  ];
  runNext();
})();
