// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testMetadataCacheItemBasic() {
  var item = new MetadataCacheItem();
  var loadRequested = item.createRequests(['propertyA']);
  assertEquals(1, loadRequested.length);
  assertEquals('propertyA', loadRequested[0]);

  item.startRequests(1, loadRequested);
  assertTrue(item.storeProperties(1, {propertyA: 'value'}));

  var result = item.get(['propertyA']);
  assertEquals('value', result.propertyA);
}

function testMetadataCacheItemAvoidDoubleLoad() {
  var item = new MetadataCacheItem();
  item.startRequests(1, ['propertyA']);
  var loadRequested = item.createRequests(['propertyA']);
  assertEquals(0, loadRequested.length);

  item.startRequests(2, loadRequested);
  assertTrue(item.storeProperties(1, {propertyA: 'value'}));

  var result = item.get(['propertyA']);
  assertEquals('value', result.propertyA);
}

function testMetadataCacheItemInvalidate() {
  var item = new MetadataCacheItem();
  item.startRequests(1, item.createRequests(['propertyA']));
  item.invalidate(2);
  assertFalse(item.storeProperties(1, {propertyA: 'value'}));

  var loadRequested = item.createRequests(['propertyA']);
  assertEquals(1, loadRequested.length);
}

function testMetadataCacheItemStoreInReverseOrder() {
  var item = new MetadataCacheItem();
  item.startRequests(1, item.createRequests(['propertyA']));
  item.startRequests(2, item.createRequests(['propertyA']));

  assertTrue(item.storeProperties(2, {propertyA: 'value2'}));
  assertFalse(item.storeProperties(1, {propertyA: 'value1'}));

  var result = item.get(['propertyA']);
  assertEquals('value2', result.propertyA);
}

function testMetadataCacheItemClone() {
  var itemA = new MetadataCacheItem();
  itemA.startRequests(1, itemA.createRequests(['property']));
  var itemB = itemA.clone();
  itemA.storeProperties(1, {property: 'value'});
  assertFalse(itemB.hasFreshCache(['property']));

  itemB.storeProperties(1, {property: 'value'});
  assertTrue(itemB.hasFreshCache(['property']));

  itemA.invalidate(2);
  assertTrue(itemB.hasFreshCache(['property']));
}

function testMetadataCacheItemHasFreshCache() {
  var item = new MetadataCacheItem();
  assertFalse(item.hasFreshCache(['propertyA', 'propertyB']));

  item.startRequests(1, item.createRequests(['propertyA', 'propertyB']));
  item.storeProperties(1, {propertyA: 'valueA', propertyB: 'valueB'});
  assertTrue(item.hasFreshCache(['propertyA', 'propertyB']));

  item.invalidate(2);
  assertFalse(item.hasFreshCache(['propertyA', 'propertyB']));

  item.startRequests(1, item.createRequests(['propertyA']));
  item.storeProperties(1, {propertyA: 'valueA'});
  assertFalse(item.hasFreshCache(['propertyA', 'propertyB']));
  assertTrue(item.hasFreshCache(['propertyA']));
}

function testMetadataCacheItemShouldNotUpdateBeforeInvalidation() {
  var item = new MetadataCacheItem();
  item.startRequests(1, item.createRequests(['property']));
  item.storeProperties(1, {property: 'value1'});
  item.storeProperties(2, {property: 'value2'});
  assertEquals('value1', item.get(['property']).property);
}

function testMetadataCacheItemError() {
  var item = new MetadataCacheItem();
  item.startRequests(1, item.createRequests(['property']));
  item.storeProperties(
      1, {property: 'value1', propertyError: new Error('Error')});
  assertEquals(undefined, item.get(['property']).property);
  assertEquals('Error', item.get(['property']).propertyError.message);
}

function testMetadataCacheItemErrorShouldNotFetchedDirectly() {
  var item = new MetadataCacheItem();
  item.startRequests(1, item.createRequests(['property']));
  item.storeProperties(
      1, {property: 'value1', propertyError: new Error('Error')});
  assertThrows(function() { item.get(['propertyError']); });
}
