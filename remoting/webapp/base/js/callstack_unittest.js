// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @suppress {checkTypes|reportUnknownTypes} */

(function() {

'use strict';

QUnit.module('callstack');

QUnit.test('well-formed stack frames are parsed correctly', function(assert) {
  var tests = [
    // Function and URL both present.
    {
      error: {
        stack: 'Error\n    at function (chrome-extension://id/file.js:1:2)'
      },
      result: {
        fn: 'function',
        url: 'chrome-extension://id',
        file: 'file.js',
        line: 1,
        column: 2
      }
    },
    // Missing function.
    {
      error: {
        stack: 'Error\n    at chrome-extension://id/file.js:3:4'
      },
      result: {
        fn: '',
        url: 'chrome-extension://id',
        file: 'file.js',
        line: 3,
        column: 4
      }
    },
    // Missing URL.
    {
      error: {
        stack: 'Error\n    at function (file.js:5:6)'
      },
      result: {
        fn: 'function',
        url: '',
        file: 'file.js',
        line: 5,
        column: 6
      }
    },
    // Missing both function and URL.
    {
      error: { stack: 'Error\n    at <anonymous>:7:8' },
      result: {
        fn: '',
        url: '',
        file: '<anonymous>',
        line: 7,
        column: 8
      }
    }
  ];

  for (var test of tests) {
    var callstack = new base.Callstack(test.error);
    assert.equal(callstack.callstack.length, 1);
    assert.equal(callstack.callstack[0].fn, test.result.fn);
    assert.equal(callstack.callstack[0].url, test.result.url);
    assert.equal(callstack.callstack[0].file, test.result.file);
    assert.equal(callstack.callstack[0].line, test.result.line);
    assert.equal(callstack.callstack[0].column, test.result.column);
  }

});

QUnit.test('line numbers are correct', function(assert) {
  var callstack1 = new base.Callstack();
  var callstack2 = new base.Callstack();
  assert.equal(callstack2.callstack[0].line, callstack1.callstack[0].line + 1);
});

QUnit.test('toString() behaves as expected', function(assert) {
  var error = {
    stack: 'Error\n' +
           '    at fn1 (http://test.com/file1:1:2)\n' +
           '    at fn2 (file2:3:4)'
  };
  var callstack = new base.Callstack(error);
  assert.equal(callstack.toString(),
               'fn1 (http://test.com/file1:1:2)\nfn2 (file2:3:4)');
});

})();
