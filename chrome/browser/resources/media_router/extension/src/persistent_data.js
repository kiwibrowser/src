// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview API for classes to save data and cleanup before the event page
 *  is shut down.
 *
 * Temporary data is removed once the extension version is changed, and thus
 * objects should never write anything to temporary data that needs to survive
 * an extension update.
 *
 * Persistent data is retained across extension versions and browser restarts;
 * it can be removed when the user clears local storage from their profile.  Do
 * not store any sensitive or per-route data persistently.
 */

goog.provide('mr.PersistentData');
goog.provide('mr.PersistentDataManager');
goog.require('mr.Logger');



/**
 * @interface
 */
mr.PersistentData = class {};


/**
 * Alias for localStorage to deal with broken Storage interface.
 * @const @private {!Object}
 */
mr.PersistentData.storageObj_ = /** @type {!Object} */ (window.localStorage);


/**
 * Get the unique name of the object instance that has data to be saved.
 * The name is used to isolate data from different objects.
 * @return {string}
 */
mr.PersistentData.prototype.getStorageKey;


/**
 * Invoked by persistent data manager when an object registers itself with the
 * manager. The object should load its saved data in this method.
 */
mr.PersistentData.prototype.loadSavedData;


/**
 * Implements this method to cleanup and return data that needs to be saved
 * before the event page is shut down.  The method should normally return a one-
 * or two-element array of temporary and optional persistent data to save.
 *
 * Temporary data is cleared on browser restart or extension version change.
 * Persistent data is retained until local storage is cleared by the browser
 * profile.
 *
 * Any Objects must be serializable with JSON.stringify.
 *
 * The implementation of getData should not make any asynchronous
 * calls; otherwise, there is no guarantee that asynchronous calls can finish.
 *
 * @return {!Array<!Object>} A one- or two-element array of data that needs to
 *    be saved.  The first element is temporary data and the second element is
 *    persistent data. Return an empty array if there is no data to save. If
 *    only persistent data needs to be persisted, pass in undefined for the
 *    first element in the two-element array.
 */
mr.PersistentData.prototype.getData;


/**
 * The total number of characters that can be stored in localStorage,
 * approximately.
 * @const {number}
 */
mr.PersistentDataManager.QUOTA_CHARS = 5200000;


/**
 * The total number of characters used by persistent data. Note that writes that
 * access localStorage directly may not be counted here.

 * @private {number}
 */
mr.PersistentDataManager.charsUsed_ = 0;


/**
 * @param {!mr.PersistentData} obj The object that may have temporary data.
 * @return {T} The data saved before. Null if no data is saved.
 * @template T
 */
mr.PersistentDataManager.getTemporaryData = function(obj) {
  const data = window.localStorage.getItem(
      mr.PersistentDataManager.getStorageKey_(obj, false));
  return data ? JSON.parse(data) : null;
};


/**
 * @param {!mr.PersistentData} obj The object that may have peristent data.
 * @return {T} The data saved before. Null if no data is saved.
 * @template T
 */
mr.PersistentDataManager.getPersistentData = function(obj) {
  const data = window.localStorage.getItem(
      mr.PersistentDataManager.getStorageKey_(obj, true));
  return data ? JSON.parse(data) : null;
};


/**
 * Registers an object so that it gets informed about onSuspend events.
 *
 * @param {!mr.PersistentData} obj An object that has data to save.
 */
mr.PersistentDataManager.register = function(obj) {
  if (mr.PersistentDataManager.dataInstances_.has(obj.getStorageKey())) {
    throw Error('Duplicate instance name ' + obj.getStorageKey());
  }
  mr.PersistentDataManager.dataInstances_.set(obj.getStorageKey(), obj);
  obj.loadSavedData();
};


/**
 * Un-Registers an object from being informed of onSuspend/Resume events.
 *
 * @param {!mr.PersistentData} obj An object to remove from tracking.
 */
mr.PersistentDataManager.unregister = function(obj) {
  mr.PersistentDataManager.dataInstances_.delete(obj.getStorageKey());
};


/**
 * @param {string} mrInstanceId The media router instance ID, which stays the
 *  same till Chrome restarts.
 */
mr.PersistentDataManager.initialize = function(mrInstanceId) {
  let otherChars = 0;
  for (let key of Object.keys(mr.PersistentData.storageObj_)) {
    const itemSize = key.length + window.localStorage.getItem(key).length;
    if (key.startsWith(mr.PersistentDataManager.KEY_PREFIX_)) {
      mr.PersistentDataManager.charsUsed_ += itemSize;
    } else {
      otherChars += itemSize;
    }
  }
  mr.PersistentDataManager.mrInstanceId_ = mrInstanceId;
  if (mr.PersistentDataManager.isVersionChanged_() ||
      mr.PersistentDataManager.isChromeReloaded(mrInstanceId)) {
    mr.PersistentDataManager.removeTemporary_();
  }
  mr.PersistentDataManager.logger_.info(
      'initialize: ' + mr.PersistentDataManager.charsUsed_ + ' chars used, ' +
      otherChars + ' other chars');
  chrome.runtime.onSuspend.addListener(mr.PersistentDataManager.onSuspend_);
};


/**
 * @private {?string}
 */
mr.PersistentDataManager.mrInstanceId_ = null;


/**
 * @const {mr.Logger}
 * @private
 */
mr.PersistentDataManager.logger_ =
    mr.Logger.getInstance('mr.PersistentDataManager');


/**
 * A map from object's instance name to the object.
 * @private @const {!Map<string, !mr.PersistentData>}
 */
mr.PersistentDataManager.dataInstances_ = new Map();


/** @private @const {string} */
mr.PersistentDataManager.KEY_PREFIX_ = 'mr.';


/**
 * @param {!mr.PersistentData} obj
 * @param {boolean} persistent
 * @return {string}
 * @private
 */
mr.PersistentDataManager.getStorageKey_ = function(obj, persistent) {
  return mr.PersistentDataManager.KEY_PREFIX_ +
      (persistent ? 'persistent.' : 'temp.') + obj.getStorageKey();
};


/**
 * Checks if the extension version has changed.
 * @return {boolean} True if current extension has a different version as
 *   the saved version.
 * @private
 */
mr.PersistentDataManager.isVersionChanged_ = function() {
  return !!window.localStorage.getItem('version') &&
      window.localStorage.getItem('version') !==
      chrome.runtime.getManifest().version;
};


/**
 * Checks if the chrome has been reloaded since the last time the extension is
 * loaded.
 * @param {string} mrInstanceId The media router instance ID, which stays the
 *  same till Chrome restarts.
 * @return {boolean} True if Chrome was reloaded.
 */
mr.PersistentDataManager.isChromeReloaded = function(mrInstanceId) {
  return !!window.localStorage.getItem('mrInstanceId') &&
      window.localStorage.getItem('mrInstanceId') !== mrInstanceId;
};


/**
 * Handles onSuspend event.
 * @private
 */
mr.PersistentDataManager.onSuspend_ = function() {
  mr.PersistentDataManager.logger_.info('onSuspend');

  mr.PersistentDataManager.write(
      'version', chrome.runtime.getManifest().version);
  if (mr.PersistentDataManager.mrInstanceId_) {
    mr.PersistentDataManager.write(
        'mrInstanceId', mr.PersistentDataManager.mrInstanceId_);
  }
  const logManager = mr.PersistentDataManager.dataInstances_.get('LogManager');
  for (const [key, obj] of mr.PersistentDataManager.dataInstances_) {
    if (obj != logManager) {
      mr.PersistentDataManager.saveData(
          /** @type {!mr.PersistentData} */ (obj));
    }
  }
  // Save the data for LogManager last, so that we save the logs generated
  // during saveData() calls.
  if (logManager) {
    mr.PersistentDataManager.saveData(logManager);
  }
};


/**
 * Save PersistentData object to local storage.
 * @param {!mr.PersistentData} obj
 */
mr.PersistentDataManager.saveData = function(obj) {
  try {
    const data = obj.getData();
    if (data && data[0] != undefined) {
      mr.PersistentDataManager.write(
          mr.PersistentDataManager.getStorageKey_(obj, false),
          JSON.stringify(data[0]));
    }
    if (data && data[1] != undefined) {
      mr.PersistentDataManager.write(
          mr.PersistentDataManager.getStorageKey_(obj, true),
          JSON.stringify(data[1]));
    }
  } catch (e) {
    mr.PersistentDataManager.logger_.error(
        `Error while saving data for ${obj.getStorageKey()}: ${e.message}`);
  }
};


/**
 * Writes value to localStorage under key.  If the value is too large to fit
 * into the remaining localStorage quota, temporary data is first removed.  If
 * the value still won't fit, an exception is thrown.

 * @param {string} key The localStorage key.
 * @param {string} value The value to write.
 */
mr.PersistentDataManager.write = function(key, value) {
  const dm = mr.PersistentDataManager;
  let sizeDelta = 0;
  const currentValue = window.localStorage.getItem(key);
  if (currentValue != null) {
    sizeDelta = value.length - currentValue.length;
  } else {
    sizeDelta = key.length + value.length;
  }

  if (dm.charsUsed_ + sizeDelta > dm.QUOTA_CHARS) {
    mr.PersistentDataManager.logger_.warning(
        'Unable to write ' + sizeDelta + ' chars');
    dm.removeTemporary_();
  }

  if (dm.charsUsed_ + sizeDelta > dm.QUOTA_CHARS) {
    dm.logger_.error(
        'Unable to write ' + sizeDelta + ' chars after clearing temporary');
    throw Error(
        `Setting the value of '${key}' would exceed the quota, ` +
        'according to accounting.');
  }

  try {
    window.localStorage.setItem(key, value);
  } catch (error) {
    throw Error(
        `Setting the value of '${key}' would exceed the quota, ` +
        'according to the browser.');
  }
  // Adjusting dm.charsUsed_ only after the call to setItem() has succeeded.
  dm.charsUsed_ += sizeDelta;
};


/**
 * Writes a Blob to localStorage under the given key, making space-efficient use
 * of localStorage by encoding two of the Blob's bytes into each DOMString
 * character. Use readBlob() to read the value back.
 * @param {string} key The localStorage key.
 * @param {!Blob} value The value to write.
 * @return {!Promise<void>} Resolves once the Blob has been written; or rejects
 *     if it won't fit.
 */
mr.PersistentDataManager.writeBlob = function(key, value) {
  // The byte size needs to be a multiple of two, since each string character
  // code is a 16-bit unsigned integer. Thus, append padding byte(s) to the end
  // of the Blob. These will also be used to indicate whether the original Blob
  // was of even or odd length when reading back later.
  if (value.size % 2 == 0) {
    value = new Blob([value, new Uint8Array([0, 0])]);
  } else {
    value = new Blob([value, new Uint8Array([1])]);
  }

  return new Promise((resolve, reject) => {
    // Use FileReader to gain access to the Blob content via an ArrayBuffer.
    const reader = new FileReader();
    reader.onloadend = () => {
      if (reader.error) {
        reject(reader.error);
        return;
      }

      try {
        const buffer = /** @type {!ArrayBuffer} */ (reader.result);

        // Convert the buffer bytes into a string, storing two bytes in each of
        // the string's characters for space efficiency. This is done in batches
        // to avoid smashing the call stack when calling String.fromCharCode().
        const batchSize = 8192;
        const pieces = [];
        for (let pos = 0, end = buffer.byteLength; pos < end;
             pos += batchSize) {
          // Note: The byteLength will always be an even number since all input
          // values to the following expression must be even numbers:
          const byteLengthOfChunk = Math.min(end - pos, batchSize);
          pieces.push(String.fromCharCode.apply(
              null, new Uint16Array(buffer, pos, byteLengthOfChunk / 2)));
        }

        // Finally, join the pieces into a single string and attempt to store
        // the string using the quota management heuristics in write().
        mr.PersistentDataManager.write(key, pieces.join(''));

        resolve();
      } catch (error) {
        reject(error);
      }
    };
    reader.readAsArrayBuffer(value);
  });
};


/**
 * Reads a Blob from localStorage under the given key. Returns null if it does
 * not exist.
 * @param {string} key The localStorage key.
 * @param {Object=} blobOptions The options for the reconstituted Blob (e.g.,
 *     {'type': 'application/gzip'}).
 * @return {?Blob}
 */
mr.PersistentDataManager.readBlob = function(key, blobOptions) {
  const asString = window.localStorage.getItem(key);
  if (asString == null || asString.length < 1) {
    return null;
  }
  const charCodes = new Uint16Array(asString.length);
  for (let i = 0; i < asString.length; ++i) {
    charCodes[i] = asString.charCodeAt(i);
  }
  // Determine, from the last byte value, whether the original Blob was of even
  // or odd length (see writeBlob()). Create a Blob from a view of all but the
  // padding byte(s) at the end of the buffer.
  const buffer = charCodes.buffer;
  const flagByte = (new Uint8Array(buffer, buffer.byteLength - 1, 1))[0];
  const viewOfPayload =
      new Uint8Array(buffer, 0, buffer.byteLength - ((flagByte == 0) ? 2 : 1));
  return new Blob([viewOfPayload], blobOptions);
};


/**
 * Removes temporary data.
 * @private
 */
mr.PersistentDataManager.removeTemporary_ = function() {
  for (let key of Object.keys(mr.PersistentData.storageObj_)) {
    if (key.startsWith(mr.PersistentDataManager.KEY_PREFIX_ + 'temp.')) {
      mr.PersistentDataManager.charsUsed_ -=
          (key.length + window.localStorage.getItem(key).length);
      delete window.localStorage[key];
    }
  }
  mr.PersistentDataManager.logger_.info(
      'removeTemporary_: ' + mr.PersistentDataManager.charsUsed_ +
      ' chars used');
};


/**
 * Removes all data.
 * @private
 */
mr.PersistentDataManager.removeAll_ = function() {
  for (let key of Object.keys(mr.PersistentData.storageObj_)) {
    if (key.startsWith(mr.PersistentDataManager.KEY_PREFIX_))
      window.localStorage.removeItem(key);
  }
  mr.PersistentDataManager.charsUsed_ = 0;
};


/**
 * Clears internal state and remove all saved data.
 * Utility method for unit test.

 */
mr.PersistentDataManager.clear = function() {
  mr.PersistentDataManager.removeAll_();
  mr.PersistentDataManager.dataInstances_.clear();
};


/**
 * Simulates suspend.
 * Utility method for unit test.

 */
mr.PersistentDataManager.suspendForTest = function() {
  mr.PersistentDataManager.onSuspend_();
  mr.PersistentDataManager.dataInstances_.clear();
};
