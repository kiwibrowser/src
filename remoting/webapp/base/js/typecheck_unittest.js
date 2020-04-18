// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

var anArray = ['an array'];
var aBoolean = false;
var aNumber = 42;
var anObject = {name: 'my object'};
var aString = 'woo';

var anotherArray = ['another array'];
var anotherBoolean = true;
var anotherNumber = -42;
var anotherObject = {name: 'a bad object'};
var anotherString = 'boo!';

var testObj = {
  anArray: anArray,
  aBoolean: aBoolean,
  aNumber: aNumber,
  anObject: anObject,
  aString: aString,
  aNull: null,
  anUndefined: undefined
};

QUnit.module('typecheck');

QUnit.test('base.assertArray', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(base.assertArray(anArray), anArray);
  assert.throws(base.assertArray.bind(null, aBoolean), Error);
  assert.throws(base.assertArray.bind(null, aNumber), Error);
  assert.throws(base.assertArray.bind(null, anObject), Error);
  assert.throws(base.assertArray.bind(null, aString), Error);
  assert.throws(base.assertArray.bind(null, null), Error);
  assert.throws(base.assertArray.bind(null, undefined), Error);
});

QUnit.test('base.assertBoolean', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(base.assertBoolean(aBoolean), aBoolean);
  assert.throws(base.assertNumber.bind(null, anArray), Error);
  assert.throws(base.assertBoolean.bind(null, aNumber), Error);
  assert.throws(base.assertBoolean.bind(null, anObject), Error);
  assert.throws(base.assertBoolean.bind(null, aString), Error);
  assert.throws(base.assertBoolean.bind(null, null), Error);
  assert.throws(base.assertBoolean.bind(null, undefined), Error);
});

QUnit.test('base.assertNumber', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(base.assertNumber(aNumber), aNumber);
  assert.throws(base.assertNumber.bind(null, anArray), Error);
  assert.throws(base.assertNumber.bind(null, aBoolean), Error);
  assert.throws(base.assertNumber.bind(null, anObject), Error);
  assert.throws(base.assertNumber.bind(null, aString), Error);
  assert.throws(base.assertNumber.bind(null, null), Error);
  assert.throws(base.assertNumber.bind(null, undefined), Error);
});

QUnit.test('base.assertObject', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(base.assertObject(anObject), anObject);
  assert.throws(base.assertObject.bind(null, anArray), Error);
  assert.throws(base.assertObject.bind(null, aBoolean), Error);
  assert.throws(base.assertObject.bind(null, aNumber), Error);
  assert.throws(base.assertObject.bind(null, aString), Error);
  assert.throws(base.assertObject.bind(null, null), Error);
  assert.throws(base.assertObject.bind(null, undefined), Error);
});

QUnit.test('base.assertString', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(base.assertString(aString), aString);
  assert.throws(base.assertString.bind(null, anArray), Error);
  assert.throws(base.assertString.bind(null, aBoolean), Error);
  assert.throws(base.assertString.bind(null, aNumber), Error);
  assert.throws(base.assertString.bind(null, anObject), Error);
  assert.throws(base.assertString.bind(null, null), Error);
  assert.throws(base.assertString.bind(null, undefined), Error);
});


QUnit.test('base.getArrayAttr', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(base.getArrayAttr(testObj, 'anArray'), anArray);
  assert.strictEqual(
      base.getArrayAttr(testObj, 'anArray', anotherArray),
      anArray);

  assert.throws(base.getArrayAttr.bind(null, testObj, 'aBoolean'), Error);
  assert.throws(base.getArrayAttr.bind(null, testObj, 'aNumber'), Error);
  assert.throws(base.getArrayAttr.bind(null, testObj, 'anObject'), Error);
  assert.throws(base.getArrayAttr.bind(null, testObj, 'aString'), Error);
  assert.throws(base.getArrayAttr.bind(null, testObj, 'aNull'), Error);
  assert.throws(base.getArrayAttr.bind(null, testObj, 'anUndefined'), Error);
  assert.throws(base.getArrayAttr.bind(null, testObj, 'noSuchKey'), Error);

  assert.strictEqual(
      base.getArrayAttr(testObj, 'aBoolean', anotherArray), anotherArray);
  assert.strictEqual(
      base.getArrayAttr(testObj, 'aNumber', anotherArray), anotherArray);
  assert.strictEqual(
      base.getArrayAttr(testObj, 'anObject', anotherArray), anotherArray);
  assert.strictEqual(
      base.getArrayAttr(testObj, 'aString', anotherArray), anotherArray);
  assert.strictEqual(
      base.getArrayAttr(testObj, 'aNull', anotherArray), anotherArray);
  assert.strictEqual(
      base.getArrayAttr(testObj, 'anUndefined', anotherArray), anotherArray);
  assert.strictEqual(
      base.getArrayAttr(testObj, 'noSuchKey', anotherArray), anotherArray);
});

QUnit.test('base.getBooleanAttr', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(base.getBooleanAttr(testObj, 'aBoolean'), aBoolean);
  assert.strictEqual(
      base.getBooleanAttr(testObj, 'aBoolean', anotherBoolean), aBoolean);

  assert.throws(base.getBooleanAttr.bind(null, testObj, 'anArray'), Error);
  assert.throws(base.getBooleanAttr.bind(null, testObj, 'aNumber'), Error);
  assert.throws(base.getBooleanAttr.bind(null, testObj, 'anObject'), Error);
  assert.throws(base.getBooleanAttr.bind(null, testObj, 'aString'), Error);
  assert.throws(base.getBooleanAttr.bind(null, testObj, 'aNull'), Error);
  assert.throws(base.getBooleanAttr.bind(null, testObj, 'anUndefined'), Error);
  assert.throws(base.getBooleanAttr.bind(null, testObj, 'noSuchKey'), Error);

  assert.strictEqual(
      base.getBooleanAttr(testObj, 'anArray', anotherBoolean),
      anotherBoolean);
  assert.strictEqual(
      base.getBooleanAttr(testObj, 'aNumber', anotherBoolean),
      anotherBoolean);
  assert.strictEqual(
      base.getBooleanAttr(testObj, 'anObject', anotherBoolean),
      anotherBoolean);
  assert.strictEqual(
      base.getBooleanAttr(testObj, 'aString', anotherBoolean),
      anotherBoolean);
  assert.strictEqual(
      base.getBooleanAttr(testObj, 'aNull', anotherBoolean),
      anotherBoolean);
  assert.strictEqual(
      base.getBooleanAttr(testObj, 'anUndefined', anotherBoolean),
      anotherBoolean);
  assert.strictEqual(
      base.getBooleanAttr(testObj, 'noSuchKey', anotherBoolean),
      anotherBoolean);
});

QUnit.test('base.getNumberAttr', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(
      base.getNumberAttr(testObj, 'aNumber'), aNumber);
  assert.strictEqual(
      base.getNumberAttr(testObj, 'aNumber', anotherNumber), aNumber);

  assert.throws(base.getNumberAttr.bind(null, testObj, 'anArray'), Error);
  assert.throws(base.getNumberAttr.bind(null, testObj, 'aBoolean'), Error);
  assert.throws(base.getNumberAttr.bind(null, testObj, 'anObject'), Error);
  assert.throws(base.getNumberAttr.bind(null, testObj, 'aString'), Error);
  assert.throws(base.getNumberAttr.bind(null, testObj, 'aNull'), Error);
  assert.throws(base.getNumberAttr.bind(null, testObj, 'anUndefined'), Error);
  assert.throws(base.getNumberAttr.bind(null, testObj, 'noSuchKey'), Error);

  assert.strictEqual(
      base.getNumberAttr(testObj, 'anArray', anotherNumber), anotherNumber);
  assert.strictEqual(
      base.getNumberAttr(testObj, 'aBoolean', anotherNumber), anotherNumber);
  assert.strictEqual(
      base.getNumberAttr(testObj, 'anObject', anotherNumber), anotherNumber);
  assert.strictEqual(
      base.getNumberAttr(testObj, 'aString', anotherNumber), anotherNumber);
  assert.strictEqual(
      base.getNumberAttr(testObj, 'aNull', anotherNumber), anotherNumber);
  assert.strictEqual(
      base.getNumberAttr(testObj, 'anUndefined', anotherNumber), anotherNumber);
  assert.strictEqual(
      base.getNumberAttr(testObj, 'noSuchKey', anotherNumber), anotherNumber);
});

QUnit.test('base.getObjectAttr', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(
      base.getObjectAttr(testObj, 'anObject'), anObject);
  assert.strictEqual(
      base.getObjectAttr(testObj, 'anObject', anotherObject), anObject);

  assert.throws(base.getObjectAttr.bind(null, testObj, 'anArray'), Error);
  assert.throws(base.getObjectAttr.bind(null, testObj, 'aBoolean'), Error);
  assert.throws(base.getObjectAttr.bind(null, testObj, 'aNumber'), Error);
  assert.throws(base.getObjectAttr.bind(null, testObj, 'aString'), Error);
  assert.throws(base.getObjectAttr.bind(null, testObj, 'aNull'), Error);
  assert.throws(base.getObjectAttr.bind(null, testObj, 'anUndefined'), Error);
  assert.throws(base.getObjectAttr.bind(null, testObj, 'noSuchKey'), Error);

  assert.strictEqual(
      base.getObjectAttr(testObj, 'anArray', anotherObject), anotherObject);
  assert.strictEqual(
      base.getObjectAttr(testObj, 'aBoolean', anotherObject), anotherObject);
  assert.strictEqual(
      base.getObjectAttr(testObj, 'aNumber', anotherObject), anotherObject);
  assert.strictEqual(
      base.getObjectAttr(testObj, 'aString', anotherObject), anotherObject);
  assert.strictEqual(
      base.getObjectAttr(testObj, 'aNull', anotherObject), anotherObject);
  assert.strictEqual(
      base.getObjectAttr(testObj, 'anUndefined', anotherObject), anotherObject);
  assert.strictEqual(
      base.getObjectAttr(testObj, 'noSuchKey', anotherObject), anotherObject);
});

QUnit.test('base.getStringAttr', function(/** QUnit.Assert */ assert) {
  assert.strictEqual(
      base.getStringAttr(testObj, 'aString'), aString);
  assert.strictEqual(
      base.getStringAttr(testObj, 'aString', anotherString), aString);

  assert.throws(base.getStringAttr.bind(null, testObj, 'anArray'), Error);
  assert.throws(base.getStringAttr.bind(null, testObj, 'aBoolean'), Error);
  assert.throws(base.getStringAttr.bind(null, testObj, 'aNumber'), Error);
  assert.throws(base.getStringAttr.bind(null, testObj, 'anObject'), Error);
  assert.throws(base.getStringAttr.bind(null, testObj, 'aNull'), Error);
  assert.throws(base.getStringAttr.bind(null, testObj, 'anUndefined'), Error);
  assert.throws(base.getStringAttr.bind(null, testObj, 'noSuchKey'), Error);

  assert.strictEqual(
      base.getStringAttr(testObj, 'anArray', anotherString), anotherString);
  assert.strictEqual(
      base.getStringAttr(testObj, 'aBoolean', anotherString), anotherString);
  assert.strictEqual(
      base.getStringAttr(testObj, 'aNumber', anotherString), anotherString);
  assert.strictEqual(
      base.getStringAttr(testObj, 'anObject', anotherString), anotherString);
  assert.strictEqual(
      base.getStringAttr(testObj, 'aNull', anotherString), anotherString);
  assert.strictEqual(
      base.getStringAttr(testObj, 'anUndefined', anotherString), anotherString);
  assert.strictEqual(
      base.getStringAttr(testObj, 'noSuchKey', anotherString), anotherString);
});

})();
