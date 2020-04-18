// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

QUnit.module('base.inherits', {
  beforeEach: function() {
    base_inherits.setupTestClasses();
  }
});

var base_inherits = {};

base_inherits.setupTestClasses = function() {

/**
 * @constructor
 * @extends {base_inherits.ChildClass}
 */
base_inherits.GrandChildClass = function() {
  base.inherits(this, base_inherits.ChildClass);
  this.name = 'grandChild';
}

/**
 * @param {string} arg
 * @return {string}
 */
base_inherits.GrandChildClass.prototype.overrideMethod = function(arg) {
  return 'overrideMethod - grandChild - ' + arg;
}

/**
 * @constructor
 * @extends {base_inherits.ParentClass}
 */
base_inherits.ChildClass = function() {
  base.inherits(this, base_inherits.ParentClass, 'parentArg');
  this.name = 'child';
  this.childOnly = 'childOnly';
}

/**
 * @param {string} arg
 * @return {string}
 */
base_inherits.ChildClass.prototype.overrideMethod = function(arg) {
  return 'overrideMethod - child - ' + arg;
}

/** @return {string} */
base_inherits.ChildClass.prototype.childMethod = function() {
  return 'childMethod';
}

/**
 * @param {string} arg
 * @constructor
 */
base_inherits.ParentClass = function(arg) {
  /** @type  {string} */
  this.name = 'parent';
  /** @type {string} */
  this.parentOnly = 'parentOnly';
  /** @type {string} */
  this.parentConstructorArg = arg;

  // Record the parent method so that we can ensure that it is available in
  // the parent constructor regardless of what type of object |this| points to.
  this.parentMethodDuringCtor = this.parentMethod;
}

/** @return  {string} */
base_inherits.ParentClass.prototype.parentMethod = function() {
  return 'parentMethod';
}

/**
 * @param {string} arg
 * @return {string}
 */
base_inherits.ParentClass.prototype.overrideMethod = function(arg) {
  return 'overrideMethod - parent - ' + arg;
}

};

QUnit.test('should invoke parent constructor with the correct arguments',
  function(assert) {
  var child = new base_inherits.ChildClass();
  assert.equal(child.parentConstructorArg, 'parentArg');
});

QUnit.test('should preserve parent property and method', function(assert) {
  var child = new base_inherits.ChildClass();
  assert.equal(child.parentOnly, 'parentOnly');
  assert.equal(child.parentMethod(), 'parentMethod');
});

QUnit.test('should preserve instanceof', function(assert) {
  var child = new base_inherits.ChildClass();
  var grandChild = new base_inherits.GrandChildClass();
  assert.ok(child instanceof base_inherits.ParentClass);
  assert.ok(child instanceof base_inherits.ChildClass);
  assert.ok(grandChild instanceof base_inherits.ParentClass);
  assert.ok(grandChild instanceof base_inherits.ChildClass);
  assert.ok(grandChild instanceof base_inherits.GrandChildClass);
});

QUnit.test('should override parent property and method', function(assert) {
  var child = new base_inherits.ChildClass();
  assert.equal(child.name, 'child');
  assert.equal(child.overrideMethod('123'), 'overrideMethod - child - 123');
  assert.equal(child.childOnly, 'childOnly');
  assert.equal(child.childMethod(), 'childMethod');
});

QUnit.test('should work on an inheritance chain', function(assert) {
  var grandChild = new base_inherits.GrandChildClass();
  assert.equal(grandChild.name, 'grandChild');
  assert.equal(grandChild.overrideMethod('246'),
               'overrideMethod - grandChild - 246');
  assert.equal(grandChild.childOnly, 'childOnly');
  assert.equal(grandChild.childMethod(), 'childMethod');
  assert.equal(grandChild.parentOnly, 'parentOnly');
  assert.equal(grandChild.parentMethod(), 'parentMethod');
});

QUnit.test('should be able to access parent class methods', function(assert) {
  var grandChild = new base_inherits.GrandChildClass();

  assert.equal(grandChild.overrideMethod('789'),
               'overrideMethod - grandChild - 789');

  var childMethod = base_inherits.ChildClass.prototype.overrideMethod.
      call(grandChild, '81');
  assert.equal(childMethod, 'overrideMethod - child - 81');

  var parentMethod = base_inherits.ParentClass.prototype.overrideMethod.
      call(grandChild, '4');
  assert.equal(parentMethod, 'overrideMethod - parent - 4');
});

QUnit.test('parent ctor should have access to parent methods',
    function(assert) {
  var child = new base_inherits.ChildClass();
  assert.ok(!!child.parentMethodDuringCtor);

  var parent = new base_inherits.ParentClass('blah');
  assert.ok(!!parent.parentMethodDuringCtor);
});
