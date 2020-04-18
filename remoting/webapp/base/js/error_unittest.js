// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

QUnit.module('error');

QUnit.test('error constructor 1', function(assert) {
  var error = new remoting.Error(remoting.Error.Tag.HOST_OVERLOAD);
  assert.equal(error.getTag(), remoting.Error.Tag.HOST_OVERLOAD);
  assert.equal(error.toString(), remoting.Error.Tag.HOST_OVERLOAD);
});

QUnit.test('error constructor 2', function(assert) {
  var error = new remoting.Error(
      remoting.Error.Tag.HOST_IS_OFFLINE,
      'detail');
  assert.equal(error.getTag(), remoting.Error.Tag.HOST_IS_OFFLINE);
  assert.ok(error.toString().indexOf(remoting.Error.Tag.HOST_IS_OFFLINE) != -1);
  assert.ok(error.toString().indexOf('detail') != -1);
});

QUnit.test('hasTag', function(assert) {
  var error = new remoting.Error(remoting.Error.Tag.HOST_OVERLOAD);
  assert.ok(error.hasTag(remoting.Error.Tag.HOST_OVERLOAD));
  assert.ok(error.hasTag(
    remoting.Error.Tag.HOST_OVERLOAD,
    remoting.Error.Tag.HOST_IS_OFFLINE));
  assert.ok(!error.hasTag(remoting.Error.Tag.HOST_IS_OFFLINE));
});

QUnit.test('constructor methods', function(assert) {
  assert.ok(remoting.Error.none().hasTag(remoting.Error.Tag.NONE));
  assert.ok(remoting.Error.unexpected().hasTag(remoting.Error.Tag.UNEXPECTED));
});

QUnit.test('isNone', function(assert) {
  assert.ok(remoting.Error.none().isNone());
  assert.ok(!remoting.Error.unexpected().isNone());
  assert.ok(!new remoting.Error(remoting.Error.Tag.CANCELLED).isNone());
});


QUnit.test('fromHttpStatus', function(assert) {
  assert.ok(remoting.Error.fromHttpStatus(200).isNone());
  assert.ok(remoting.Error.fromHttpStatus(201).isNone());
  assert.ok(!remoting.Error.fromHttpStatus(500).isNone());
  assert.equal(
      remoting.Error.fromHttpStatus(0).getTag(),
      remoting.Error.Tag.NETWORK_FAILURE);
  assert.equal(
      remoting.Error.fromHttpStatus(100).getTag(),
      remoting.Error.Tag.UNEXPECTED);
  assert.equal(
      remoting.Error.fromHttpStatus(200).getTag(),
      remoting.Error.Tag.NONE);
  assert.equal(
      remoting.Error.fromHttpStatus(201).getTag(),
      remoting.Error.Tag.NONE);
  assert.equal(
      remoting.Error.fromHttpStatus(400).getTag(),
      remoting.Error.Tag.AUTHENTICATION_FAILED);
  assert.equal(
      remoting.Error.fromHttpStatus(401).getTag(),
      remoting.Error.Tag.AUTHENTICATION_FAILED);
  assert.equal(
      remoting.Error.fromHttpStatus(402).getTag(),
      remoting.Error.Tag.UNEXPECTED);
  assert.equal(
      remoting.Error.fromHttpStatus(403).getTag(),
      remoting.Error.Tag.NOT_AUTHORIZED);
  assert.equal(
      remoting.Error.fromHttpStatus(404).getTag(),
      remoting.Error.Tag.NOT_FOUND);
  assert.equal(
      remoting.Error.fromHttpStatus(500).getTag(),
      remoting.Error.Tag.SERVICE_UNAVAILABLE);
  assert.equal(
      remoting.Error.fromHttpStatus(501).getTag(),
      remoting.Error.Tag.SERVICE_UNAVAILABLE);
  assert.equal(
      remoting.Error.fromHttpStatus(600).getTag(),
      remoting.Error.Tag.UNEXPECTED);
});

QUnit.test('handler 1', function(assert) {
  /** @type {!remoting.Error} */
  var reportedError;
  var onError = function(/** !remoting.Error */ arg) {
    reportedError = arg;
  };
  remoting.Error.handler(onError)('not a real tag');
  assert.ok(reportedError instanceof remoting.Error);
  assert.equal(reportedError.getTag(), remoting.Error.Tag.UNEXPECTED);
});


QUnit.test('handler 2', function(assert) {
  /** @type {!remoting.Error} */
  var reportedError;
  var onError = function(/** !remoting.Error */ arg) {
    reportedError = arg;
  };
  var origError = new remoting.Error(remoting.Error.Tag.HOST_IS_OFFLINE);
  remoting.Error.handler(onError)(origError);
  assert.equal(reportedError, origError);
});

})();
