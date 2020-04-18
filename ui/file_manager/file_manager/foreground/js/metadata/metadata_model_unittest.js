// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function TestMetadataProvider() {
  MetadataProvider.call(this, ['property', 'propertyA', 'propertyB']);
  this.requestCount = 0;
}

TestMetadataProvider.prototype.__proto__ = MetadataProvider.prototype;

TestMetadataProvider.prototype.get = function(requests) {
  this.requestCount++;
  return Promise.resolve(requests.map(function(request) {
    var entry = request.entry;
    var names = request.names;
    var result = {};
    for (var i = 0; i < names.length; i++) {
      result[names[i]] = entry.toURL() + ':' + names[i];
    }
    return result;
  }));
};

function TestEmptyMetadataProvider() {
  MetadataProvider.call(this, ['property']);
}

TestEmptyMetadataProvider.prototype.__proto__ = MetadataProvider.prototype;

TestEmptyMetadataProvider.prototype.get = function(requests) {
  return Promise.resolve(requests.map(function() {
    return {};
  }));
};

function ManualTestMetadataProvider() {
  MetadataProvider.call(
      this, ['propertyA', 'propertyB', 'propertyC']);
  this.callback = [];
}

ManualTestMetadataProvider.prototype.__proto__ = MetadataProvider.prototype;

ManualTestMetadataProvider.prototype.get = function(requests) {
  return new Promise(function(fulfill) {
    this.callback.push(fulfill);
  }.bind(this));
};

var entryA = {
  toURL: function() { return "filesystem://A"; }
};

var entryB = {
  toURL: function() { return "filesystem://B"; }
};

function testMetadataModelBasic(callback) {
  var model = new MetadataModel(new TestMetadataProvider());
  reportPromise(model.get([entryA, entryB], ['property']).then(
      function(results) {
        assertEquals(1, model.getProvider().requestCount);
        assertEquals('filesystem://A:property', results[0].property);
        assertEquals('filesystem://B:property', results[1].property);
      }), callback);
}

function testMetadataModelRequestForCachedProperty(callback) {
  var model = new MetadataModel(new TestMetadataProvider());
  reportPromise(model.get([entryA, entryB], ['property']).then(
      function() {
        // All the result should be cached here.
        return model.get([entryA, entryB], ['property']);
      }).then(function(results) {
        assertEquals(1, model.getProvider().requestCount);
        assertEquals('filesystem://A:property', results[0].property);
        assertEquals('filesystem://B:property', results[1].property);
      }), callback);
}

function testMetadataModelRequestForCachedAndNonCachedProperty(callback) {
  var model = new MetadataModel(new TestMetadataProvider());
  reportPromise(model.get([entryA, entryB], ['propertyA']).then(
      function() {
        assertEquals(1, model.getProvider().requestCount);
        // propertyB has not been cached here.
        return model.get([entryA, entryB], ['propertyA', 'propertyB']);
      }).then(function(results) {
        assertEquals(2, model.getProvider().requestCount);
        assertEquals('filesystem://A:propertyA', results[0].propertyA);
        assertEquals('filesystem://A:propertyB', results[0].propertyB);
        assertEquals('filesystem://B:propertyA', results[1].propertyA);
        assertEquals('filesystem://B:propertyB', results[1].propertyB);
      }), callback);
}

function testMetadataModelRequestForCachedAndNonCachedEntry(callback) {
  var model = new MetadataModel(new TestMetadataProvider());
  reportPromise(model.get([entryA], ['property']).then(
      function() {
        assertEquals(1, model.getProvider().requestCount);
        // entryB has not been cached here.
        return model.get([entryA, entryB], ['property']);
      }).then(function(results) {
        assertEquals(2, model.getProvider().requestCount);
        assertEquals('filesystem://A:property', results[0].property);
        assertEquals('filesystem://B:property', results[1].property);
      }), callback);
}

function testMetadataModelRequestBeforeCompletingPreviousRequest(
    callback) {
  var model = new MetadataModel(new TestMetadataProvider());
  model.get([entryA], ['property']);
  assertEquals(1, model.getProvider().requestCount);
  // The result of first call has not been fetched yet.
  reportPromise(model.get([entryA], ['property']).then(
      function(results) {
        assertEquals(1, model.getProvider().requestCount);
        assertEquals('filesystem://A:property', results[0].property);
      }), callback);
}

function testMetadataModelNotUpdateCachedResultAfterRequest(
    callback) {
  var model = new MetadataModel(new ManualTestMetadataProvider());
  var promise = model.get([entryA], ['propertyA']);
  model.getProvider().callback[0]([{propertyA: 'valueA1'}]);
  reportPromise(promise.then(function() {
    // 'propertyA' is cached here.
    var promise1 = model.get([entryA], ['propertyA', 'propertyB']);
    var promise2 = model.get([entryA], ['propertyC']);
    // Returns propertyC.
    model.getProvider().callback[2](
        [{propertyA: 'valueA2', propertyC: 'valueC'}]);
    model.getProvider().callback[1]([{propertyB: 'valueB'}]);
    return Promise.all([promise1, promise2]);
  }).then(function(results) {
    // The result should be cached value at the time when get was called.
    assertEquals('valueA1', results[0][0].propertyA);
    assertEquals('valueB', results[0][0].propertyB);
    assertEquals('valueC', results[1][0].propertyC);
  }), callback);
}

function testMetadataModelGetCache(callback) {
  var model = new MetadataModel(new TestMetadataProvider());
  var promise = model.get([entryA], ['property']);
  var cache = model.getCache([entryA], ['property']);
  assertEquals(null, cache[0].property);
  reportPromise(promise.then(function() {
    var cache = model.getCache([entryA], ['property']);
    assertEquals(1, model.getProvider().requestCount);
    assertEquals('filesystem://A:property', cache[0].property);
  }), callback);
}

function testMetadataModelUnknownProperty() {
  var model = new MetadataModel(new TestMetadataProvider());
  assertThrows(function() {
    model.get([entryA], ['unknown']);
  });
}

function testMetadataModelEmptyResult(callback) {
  var model = new MetadataModel(new TestEmptyMetadataProvider());
  // getImpl returns empty result.
  reportPromise(model.get([entryA], ['property']).then(function(results) {
    assertEquals(undefined, results[0].property);
  }), callback);
}
