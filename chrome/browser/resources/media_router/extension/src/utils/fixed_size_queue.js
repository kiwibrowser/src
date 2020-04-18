// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview FIFO with a hard limit on its size.

 */

goog.module('mr.FixedSizeQueue');
goog.module.declareLegacyNamespace();


/**
 * A fixed-sized buffer with FIFO semantics.
 * @template T
 */
class FixedSizeQueue {
  /**
   * @param {number} maxSize The size of the buffer.
   */
  constructor(maxSize) {
    if (maxSize <= 0) {
      throw Error('invalid buffer size');
    }

    /**
     * Items are popped from here.  Elements are stored in reverse
     * insertion order.
     * @private @type {!Array<T>}
     */
    this.head_ = [];

    /**
     * Items are pushed here.  Elements are stored in insertion order.
     * @private @type {!Array<T>}
     */
    this.tail_ = [];

    /**
     * @private @const
     */
    this.maxSize_ = maxSize;
  }

  /**
   * Adds an item to the buffer.  Drops the last added item if the
   * buffer is full.
   * @param {T} item
   */
  enqueue(item) {
    if (this.getCount() >= this.maxSize_) {
      this.dequeue();
    }
    this.tail_.push(item);
  }

  /**
   * Removes the oldest item from the buffer, which must be non-empty.
   * @return {T} The removed item.
   */
  dequeue() {
    if (this.isEmpty()) {
      throw Error('Empty queue');
    }
    if (this.head_.length == 0) {
      this.head_ = this.tail_;
      this.head_.reverse();
      this.tail_ = [];
    }
    return this.head_.pop();
  }

  /**
   * Removes and returns all items in the buffer in insertion order.
   * @return {!Array<T>}
   */
  dequeueAll() {
    const result = this.getValues();
    this.clear();
    return result;
  }

  /**
   * @return {number} The number of items in the buffer.
   */
  getCount() {
    return this.head_.length + this.tail_.length;
  }

  /**
   * @return {boolean} True if the buffer is full.
   */
  isFull() {
    return this.getCount() == this.maxSize_;
  }

  /**
   * @return {boolean} True if the buffer is empty.
   */
  isEmpty() {
    return this.getCount() == 0;
  }

  /**
   * Gets all the items in the buffer in insertion order.
   * @return {!Array<T>}
   */
  getValues() {
    const result = this.head_.slice();  // clones array
    result.reverse();
    result.push(...this.tail_);
    return result;
  }

  /**
   * Makes the buffer empty.
   */
  clear() {
    this.head_ = [];
    this.tail_ = [];
  }
}


exports = FixedSizeQueue;
