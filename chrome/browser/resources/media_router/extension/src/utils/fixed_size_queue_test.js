// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.require('mr.FixedSizeQueue');

describe('mr.FixedSizeQueue', function() {
  let queue;

  beforeEach(function() {
    queue = new mr.FixedSizeQueue(3);
  });

  it('works', function() {
    expect(queue.getCount()).toBe(0);
    expect(queue.isFull()).toBe(false);
    expect(queue.isEmpty()).toBe(true);
    expect(queue.getValues()).toEqual([]);

    queue.enqueue(1);
    expect(queue.getCount()).toBe(1);
    expect(queue.isFull()).toBe(false);
    expect(queue.isEmpty()).toBe(false);
    expect(queue.getValues()).toEqual([1]);

    queue.enqueue(2);
    queue.enqueue(3);
    expect(queue.getCount()).toBe(3);
    expect(queue.isFull()).toBe(true);
    expect(queue.isEmpty()).toBe(false);
    expect(queue.getValues()).toEqual([1, 2, 3]);

    queue.enqueue(4);
    expect(queue.getCount()).toBe(3);
    expect(queue.isFull()).toBe(true);
    expect(queue.isEmpty()).toBe(false);
    expect(queue.getValues()).toEqual([2, 3, 4]);

    queue.clear();
    expect(queue.getCount()).toBe(0);
    expect(queue.isFull()).toBe(false);
    expect(queue.isEmpty()).toBe(true);
    expect(queue.getValues()).toEqual([]);
  });

  describe('when full', function() {
    beforeEach(function() {
      queue.enqueue(1);
      queue.enqueue(2);
      queue.enqueue(3);
      expect(queue.getCount()).toBe(3);
      expect(queue.isFull()).toBe(true);
    });

    it('supports dequeue', function() {
      expect(queue.dequeue()).toBe(1);
      expect(queue.getCount()).toBe(2);
      expect(queue.getValues()).toEqual([2, 3]);
      expect(queue.dequeue()).toBe(2);
      expect(queue.getCount()).toBe(1);
      expect(queue.getValues()).toEqual([3]);
      expect(queue.dequeue()).toBe(3);
      expect(queue.getCount()).toBe(0);
      expect(queue.getValues()).toEqual([]);
      expect(queue.isFull()).toBe(false);
      expect(queue.isEmpty()).toBe(true);
    });

    it('supports deqeueAll', function() {
      expect(queue.dequeueAll()).toEqual([1, 2, 3]);
      expect(queue.isEmpty()).toBe(true);
    });
  });
});
