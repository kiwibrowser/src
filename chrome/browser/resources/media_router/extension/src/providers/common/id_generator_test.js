// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.require('mr.IdGenerator');
goog.require('mr.PersistentDataManager');

describe('IdGenerator Tests', function() {
  let generator;

  beforeEach(function() {
    spyOn(Math, 'random').and.returnValue(0);
    generator = new mr.IdGenerator('test');
    generator.enablePersistent();
  });

  afterEach(function() {
    mr.PersistentDataManager.clear();
  });

  it('getNext', function() {
    expect(generator.getNext()).toBe(1);
    expect(generator.getNext()).toBe(2);
    expect(generator.getNext()).toBe(3);
  });

  it('Persistent', function() {
    expect(generator.getNext()).toBe(1);
    expect(generator.getNext()).toBe(2);
    mr.PersistentDataManager.suspendForTest();
    generator = new mr.IdGenerator('test');
    generator.loadSavedData();
    expect(generator.getNext()).toBe(3);
  });

});
