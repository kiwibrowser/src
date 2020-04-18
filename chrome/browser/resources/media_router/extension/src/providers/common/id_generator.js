// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview An ID generator that keeps its state across extension
 *  suspend/wakeup cycles.
 *

 */

goog.provide('mr.IdGenerator');

goog.require('mr.PersistentData');
goog.require('mr.PersistentDataManager');


/**
 * @implements {mr.PersistentData}
 */
mr.IdGenerator = class {
  /**
   * @param {!string} storageKey
   */
  constructor(storageKey) {
    /** @private @const {!string} */
    this.storageKey_ = storageKey;

    /** @private {!number} */
    this.nextId_ = Math.floor(Math.random() * 1e6) * 1000;
  }

  /**
   * Enables persistent
   */
  enablePersistent() {
    mr.PersistentDataManager.register(this);
  }

  /**
   * @return {!number} The next ID. 0 will never be returned.
   */
  getNext() {
    let nextId = this.nextId_++;
    if (nextId == 0) {
      // Skip 0, which is used by Cast receiver to
      // indicate that the broadcast status message is not coming from a
      // specific
      // sender (it is an autonomous status change, not triggered by a command
      // from any sender). Strange usage of 0 though; could be a null / optional
      // field.
      nextId = this.nextId_++;
    }
    return nextId;
  }

  /**
   * @override
   */
  getStorageKey() {
    return 'IdGenerator.' + this.storageKey_;
  }

  /**
   * @override
   */
  getData() {
    return [this.nextId_];
  }

  /**
   * @override
   */
  loadSavedData() {
    const savedData = mr.PersistentDataManager.getTemporaryData(this);
    if (savedData) {
      this.nextId_ = savedData;
    }
  }
};
