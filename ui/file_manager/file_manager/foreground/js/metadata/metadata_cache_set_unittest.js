// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var entryA = {
  toURL: function() { return "filesystem://A"; }
};

var entryB = {
  toURL: function() { return "filesystem://B"; }
};

function testMetadataCacheSetBasic() {
  var set = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  var loadRequested = set.createRequests([entryA, entryB], ['property']);
  assertEquals(2, loadRequested.length);
  assertEquals(entryA, loadRequested[0].entry);
  assertEquals(1, loadRequested[0].names.length);
  assertEquals('property', loadRequested[0].names[0]);
  assertEquals(entryB, loadRequested[1].entry);
  assertEquals(1, loadRequested[1].names.length);
  assertEquals('property', loadRequested[1].names[0]);

  set.startRequests(1, loadRequested);
  assertTrue(set.storeProperties(
      1, [entryA, entryB], [{property: 'valueA'}, {property: 'valueB'}]));

  var results = set.get([entryA, entryB], ['property']);
  assertEquals(2, results.length);
  assertEquals('valueA', results[0].property);
  assertEquals('valueB', results[1].property);
}

function testMetadataCacheSetStorePartial() {
  var set = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  set.startRequests(1, set.createRequests([entryA, entryB], ['property']));

  assertTrue(set.storeProperties(
      1, [entryA], [{property: 'valueA'}]));
  var results = set.get([entryA, entryB], ['property']);
  assertEquals(2, results.length);
  assertEquals('valueA', results[0].property);
  assertEquals(null, results[1].property);

  assertTrue(set.storeProperties(
      1, [entryB], [{property: 'valueB'}]));
  var results = set.get([entryA, entryB], ['property']);
  assertEquals(2, results.length);
  assertEquals('valueA', results[0].property);
  assertEquals('valueB', results[1].property);
}

function testMetadataCacheSetCachePartial() {
  var set = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  set.startRequests(1, set.createRequests([entryA], ['property']));
  set.storeProperties(1, [entryA], [{property: 'valueA'}]);

  // entryA has already been cached.
  var loadRequested = set.createRequests([entryA, entryB], ['property']);
  assertEquals(1, loadRequested.length);
  assertEquals(entryB, loadRequested[0].entry);
  assertEquals(1, loadRequested[0].names.length);
  assertEquals('property', loadRequested[0].names[0]);
}

function testMetadataCacheSetInvalidatePartial() {
  var set = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  set.startRequests(1, set.createRequests([entryA, entryB], ['property']));
  set.invalidate(2, [entryA]);

  assertTrue(set.storeProperties(
      1, [entryA, entryB], [{property: 'valueA'}, {property: 'valueB'}]));

  var results = set.get([entryA, entryB], ['property']);
  assertEquals(2, results.length);
  assertEquals(null, results[0].property);
  assertEquals('valueB', results[1].property);

  var loadRequested = set.createRequests([entryA, entryB], ['property']);
  assertEquals(1, loadRequested.length);
  assertEquals(entryA, loadRequested[0].entry);
  assertEquals(1, loadRequested[0].names.length);
  assertEquals('property', loadRequested[0].names[0]);
}

function testMetadataCacheSetCreateSnapshot() {
  var setA = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  setA.startRequests(1, setA.createRequests([entryA, entryB], ['property']));
  var setB = setA.createSnapshot([entryA]);
  setA.storeProperties(
      1, [entryA, entryB], [{property: 'valueA'}, {property: 'valueB'}]);
  var results = setB.get([entryA, entryB], ['property']);
  assertEquals(2, results.length);
  assertEquals(null, results[0].property);
  assertEquals(undefined, results[1].property);

  setB.storeProperties(
      1, [entryA, entryB], [{property: 'valueA'}, {property: 'valueB'}]);
  var results = setB.get([entryA, entryB], ['property']);
  assertEquals(2, results.length);
  assertEquals('valueA', results[0].property);
  assertEquals(undefined, results[1].property);

  setA.invalidate(2, [entryA, entryB]);
  var results = setB.get([entryA, entryB], ['property']);
  assertEquals(2, results.length);
  assertEquals('valueA', results[0].property);
  assertEquals(undefined, results[1].property);
}

function testMetadataCacheSetHasFreshCache() {
  var set = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  assertFalse(set.hasFreshCache([entryA, entryB], ['property']));

  set.startRequests(1, set.createRequests([entryA, entryB], ['property']));
  set.storeProperties(
      1, [entryA, entryB], [{property: 'valueA'}, {property: 'valueB'}]);
  assertTrue(set.hasFreshCache([entryA, entryB], ['property']));

  set.invalidate(2, [entryB]);
  assertFalse(set.hasFreshCache([entryA, entryB], ['property']));

  assertTrue(set.hasFreshCache([entryA], ['property']));
}

function testMetadataCacheSetHasFreshCacheWithEmptyNames() {
  var set = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  assertTrue(set.hasFreshCache([entryA, entryB], []));
}

function testMetadataCacheSetClear() {
  var set = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  set.startRequests(1, set.createRequests([entryA], ['propertyA']));
  set.storeProperties(1, [entryA], [{propertyA: 'value'}]);
  assertTrue(set.hasFreshCache([entryA], ['propertyA']));

  set.startRequests(1, set.createRequests([entryA], ['propertyB']));
  set.clear([entryA.toURL()]);
  // PropertyB should not be stored because it is requsted before clear.
  set.storeProperties(1, [entryA], [{propertyB: 'value'}]);

  assertFalse(set.hasFreshCache([entryA], ['propertyA']));
  assertFalse(set.hasFreshCache([entryA], ['propertyB']));
}

function testMetadataCacheSetUpdateEvent() {
  var set = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  var event = null;
  set.addEventListener('update', function(inEvent) {
    event = inEvent;
  });
  set.startRequests(1, set.createRequests([entryA], ['propertyA']));
  set.storeProperties(1, [entryA], [{propertyA: 'value'}]);
  assertEquals(1, event.entries.length);
  assertEquals(entryA, event.entries[0]);
}

function testMetadataCacheSetClearAll() {
  var set = new MetadataCacheSet(new MetadataCacheSetStorageForObject({}));
  set.startRequests(1, set.createRequests([entryA, entryB], ['propertyA']));
  set.storeProperties(
      1, [entryA, entryB], [{propertyA: 'value'}, {propertyA: 'value'}]);

  assertTrue(set.hasFreshCache([entryA, entryB], ['propertyA']));
  set.clearAll();
  assertFalse(set.hasFreshCache([entryA], ['propertyA']));
  assertFalse(set.hasFreshCache([entryB], ['propertyA']));
}
