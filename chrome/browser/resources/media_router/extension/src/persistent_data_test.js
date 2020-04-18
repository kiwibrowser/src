// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.setTestOnly('persistent_data_test');

goog.require('mr.PersistentData');
goog.require('mr.PersistentDataManager');
goog.require('mr.UnitTestUtils');


/**
 * @implements {mr.PersistentData}
 * @private
 */
DummyData_ = class {
  /**
   * @param {string} id
   */
  constructor(id) {
    /** @private {string} */
    this.id_ = id;
  }

  /**
   * @override
   */
  getData() {
    return [];
  }

  /**
   * @override
   */
  getStorageKey() {
    return 'dummy-data' + this.id_;
  }

  /**
   * @override
   */
  loadSavedData() {}
};


describe('Tests PersistentDataManager', () => {
  let dataInstance1;
  let dataInstance2;
  let onSuspendListener;
  let version;
  let mrInstanceId;
  const originalQuota = mr.PersistentDataManager.QUOTA_CHARS;

  beforeEach(() => {
    window.localStorage.clear();
    mr.PersistentDataManager.charsUsed_ = 0;
    dataInstance1 = new DummyData_('1');
    dataInstance2 = new DummyData_('2');
    version = '1.0';
    mrInstanceId = '123';
    mr.UnitTestUtils.mockChromeApi();
    chrome.runtime.onSuspend = {
      addListener: l => {
        onSuspendListener = l;
      }
    };
    chrome.runtime.getManifest = () => ({'version': version});
    mr.PersistentDataManager.initialize(mrInstanceId);
    dataInstance1.loadSavedData = jasmine.createSpy('loadSavedData');
    dataInstance2.loadSavedData = jasmine.createSpy('loadSavedData');
    mr.PersistentDataManager.register(dataInstance1);
    mr.PersistentDataManager.register(dataInstance2);
    expect(dataInstance1.loadSavedData).toHaveBeenCalled();
    expect(dataInstance2.loadSavedData).toHaveBeenCalled();
  });

  afterEach(() => {
    mr.PersistentDataManager.clear();
    mr.PersistentDataManager.QUOTA_CHARS = originalQuota;
    mr.UnitTestUtils.restoreChromeApi();
  });

  it('returns null with no saved data', () => {
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance1)).toBe(null);
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance2)).toBe(null);
  });

  describe('handles onSuspend', () => {
    it('with one instance with data', () => {
      dataInstance1.getData = () => [{'d': 1}, {'e': 1}];
      dataInstance2.getData = () => null;
      onSuspendListener();
      expect(mr.PersistentDataManager.getTemporaryData(dataInstance1)).toEqual({
        'd': 1
      });
      expect(mr.PersistentDataManager.getPersistentData(dataInstance1))
          .toEqual({'e': 1});
      expect(mr.PersistentDataManager.getTemporaryData(dataInstance2))
          .toBe(null);
      expect(mr.PersistentDataManager.charsUsed_).toBe(83);
    });

    it('with two instances with data', () => {
      dataInstance1.getData = () => [{'d': 1}, {'e': 1}];
      dataInstance2.getData = () => [{'d': 2}, {'e': 2}];
      onSuspendListener();
      expect(mr.PersistentDataManager.getTemporaryData(dataInstance1)).toEqual({
        'd': 1
      });
      expect(mr.PersistentDataManager.getPersistentData(dataInstance1))
          .toEqual({'e': 1});
      expect(mr.PersistentDataManager.getTemporaryData(dataInstance2)).toEqual({
        'd': 2
      });
      expect(mr.PersistentDataManager.getPersistentData(dataInstance2))
          .toEqual({'e': 2});
      expect(mr.PersistentDataManager.charsUsed_).toBe(141);
    });

    it('with no instance with data', () => {
      dataInstance1.getData = () => null;
      dataInstance2.getData = () => null;
      onSuspendListener();
      expect(mr.PersistentDataManager.getTemporaryData(dataInstance1))
          .toBe(null);
      expect(mr.PersistentDataManager.getTemporaryData(dataInstance2))
          .toBe(null);
      expect(mr.PersistentDataManager.charsUsed_).toBe(25);
    });

    it('with an instance with a circular dependency', () => {
      const data1 = {};
      data1.data = data1;
      dataInstance1.getData = () => [{'d': data1}, {'e': data1}];
      dataInstance2.getData = () => [{'d': 2}, {'e': 2}];
      onSuspendListener();

      // If there is a circular dependency of objects, that data cannot be
      // converted into JSON or saved. However other data should still be saved.
      expect(mr.PersistentDataManager.getTemporaryData(dataInstance2)).toEqual({
        'd': 2
      });
      expect(mr.PersistentDataManager.getPersistentData(dataInstance2))
          .toEqual({'e': 2});
    });
  });

  it('Test saveData', () => {
    dataInstance1.getData = () => [{'d': 1}, {'e': 1}];
    dataInstance2.getData = () => [{'d': 2}, {'e': 2}];
    const dataInstance3 = new DummyData_('3');
    dataInstance3.getData = () => [false, {'e': 3}];
    const dataInstance4 = new DummyData_('4');
    dataInstance4.getData = () => [undefined, 0];

    // Note that dataInstance2 is not saved.
    mr.PersistentDataManager.saveData(dataInstance1);
    mr.PersistentDataManager.saveData(dataInstance3);
    mr.PersistentDataManager.saveData(dataInstance4);

    expect(mr.PersistentDataManager.getTemporaryData(dataInstance1)).toEqual({
      'd': 1
    });
    expect(mr.PersistentDataManager.getPersistentData(dataInstance1)).toEqual({
      'e': 1
    });
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance2)).toBeNull();
    expect(mr.PersistentDataManager.getPersistentData(dataInstance2))
        .toBeNull();
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance3))
        .toEqual(false);
    expect(mr.PersistentDataManager.getPersistentData(dataInstance3)).toEqual({
      'e': 3
    });
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance4)).toBeNull();
    expect(mr.PersistentDataManager.getPersistentData(dataInstance4))
        .toEqual(0);
  });

  it('Test unregister', () => {
    dataInstance1.getData = () => [{'d': 1}, {'e': 1}];
    dataInstance2.getData = () => [{'d': 2}, {'e': 2}];
    const dataInstance3 = new DummyData_('3');
    dataInstance3.getData = () => [{'d': 3}, {'e': 3}];
    mr.PersistentDataManager.register(dataInstance3);
    onSuspendListener();
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance3)).toEqual({
      'd': 3
    });
    mr.PersistentDataManager.unregister(dataInstance3);
    // Get data should not be called again.
    dataInstance3.getData = () => {
      fail();
    };
    onSuspendListener();
  });

  it('handles version change', () => {
    dataInstance1.getData = () => [{'d': 1}, {'e': 1}];
    onSuspendListener();
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance1)).toEqual({
      'd': 1
    });
    expect(mr.PersistentDataManager.getPersistentData(dataInstance1)).toEqual({
      'e': 1
    });
    version = '1.1';
    expect(mr.PersistentDataManager.isChromeReloaded(mrInstanceId)).toBe(false);
    mr.PersistentDataManager.initialize(mrInstanceId);
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance1)).toBe(null);
    expect(mr.PersistentDataManager.getPersistentData(dataInstance1)).toEqual({
      'e': 1
    });
  });

  it('handles mrInstanceId change', () => {
    dataInstance1.getData = () => [{'d': 1}, {'e': 1}];
    onSuspendListener();
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance1)).toEqual({
      'd': 1
    });
    expect(mr.PersistentDataManager.getPersistentData(dataInstance1)).toEqual({
      'e': 1
    });
    mrInstanceId = '321';
    expect(mr.PersistentDataManager.isChromeReloaded(mrInstanceId)).toBe(true);
    mr.PersistentDataManager.initialize(mrInstanceId);
    expect(mr.PersistentDataManager.getTemporaryData(dataInstance1)).toBe(null);
    expect(mr.PersistentDataManager.getPersistentData(dataInstance1)).toEqual({
      'e': 1
    });
  });

  describe('allows writes', () => {
    it('of small values', () => {
      mr.PersistentDataManager.write('mr.temp.Buckaroo', 'Bonzai');
      expect(window.localStorage.getItem('mr.temp.Buckaroo')).toBe('Bonzai');
      expect(mr.PersistentDataManager.charsUsed_).toBe(22);
    });

    it('of large values over quota', () => {
      mr.PersistentDataManager.QUOTA_CHARS = 100;
      [1, 2, 3, 4].forEach(index => {
        mr.PersistentDataManager.write(
            'mr.temp.' + index, 'Only the dead have seen the end of war.');
      });
      expect(mr.PersistentDataManager.charsUsed_).toBe(96);
      // Normally this would go over QUOTA_CHARS, but clearing temporary
      // values allows it to succeed.
      mr.PersistentDataManager.write(
          'mr.persistent.5', 'Courage is knowing what not to fear.');
      expect(mr.PersistentDataManager.charsUsed_).toBe(51);
      [1, 2, 3, 4].forEach(index => {
        expect(window.localStorage.getItem('mr.temp.' + index)).toBeNull();
      });
      expect(window.localStorage.getItem('mr.persistent.5'))
          .toBe('Courage is knowing what not to fear.');
    });
  });

  describe('stores and reads Blobs', () => {
    const dm = mr.PersistentDataManager;
    const testKey = 'test';

    const allUint16Values = [];
    for (let i = 0; i < (1 << 16); ++i) {
      allUint16Values.push(i);
    }

    // Takes an array of byte values, creates a Blob, stores and retrieves it
    // back, and then maps the bytes of the Blob back to an array of byte
    // values.
    function writeThenReadByteValues(values) {
      return new Promise((resolve, reject) => {
        dm.writeBlob(testKey, new Blob([new Uint8Array(values)])).then(() => {
          const reader = new FileReader();
          reader.onloadend = () => {
            if (reader.error) {
              reject(reader.error);
            } else {
              resolve(Array.from(new Uint8Array(reader.result)));
            }
          };
          reader.readAsArrayBuffer(dm.readBlob(testKey));
        }, reject);
      });
    }

    // Same as writeThenReadByteValues(), except operate on an array of uint16.
    function writeThenReadShortValues(values) {
      return new Promise((resolve, reject) => {
        dm.writeBlob(testKey, new Blob([new Uint16Array(values)])).then(() => {
          const reader = new FileReader();
          reader.onloadend = () => {
            if (reader.error) {
              reject(reader.error);
            } else {
              resolve(Array.from(new Uint16Array(reader.result)));
            }
          };
          reader.readAsArrayBuffer(dm.readBlob(testKey));
        }, reject);
      });
    }

    it('of zero size', done => {
      writeThenReadByteValues([]).then(values => {
        expect(values).toEqual([]);
        done();
      }, fail);
    });

    it('of even size', done => {
      writeThenReadByteValues([42, 1]).then(values => {
        expect(values).toEqual([42, 1]);
        done();
      }, fail);
    });

    it('of odd size', done => {
      writeThenReadByteValues([42, 1, 0]).then(values => {
        expect(values).toEqual([42, 1, 0]);
        done();
      }, fail);
    });

    it('containing all possible uint16 values', done => {
      const original = allUint16Values.concat(allUint16Values.reverse());
      writeThenReadShortValues(original).then(values => {
        expect(values).toEqual(original);
        done();
      }, fail);
    });

    it('that are just within quota', done => {
      // Note on quota space needed: The string length of the key plus value
      // must be available. A blob of 7 uint16 values will have a byte size of
      // 14. The impl will add two padding bytes to the end, making the total 16
      // bytes. Thus, 16/2 = 8 chars of quota must be available for the value.
      // In total, 4 + 8 (key + value) chars must be available.
      mr.PersistentDataManager.QUOTA_CHARS =
          mr.PersistentDataManager.charsUsed_ + testKey.length + 8;
      const original = allUint16Values.slice(0, 7);
      writeThenReadShortValues(original).then(values => {
        expect(values).toEqual(original);
        done();
      }, fail);
    });

    it('that exceed quota', done => {
      // See note above on how quota accounting works.
      mr.PersistentDataManager.QUOTA_CHARS =
          mr.PersistentDataManager.charsUsed_ + testKey.length + 8;
      const original = allUint16Values.slice(0, 8);
      writeThenReadShortValues(original).then(fail, error => {
        expect(dm.readBlob(testKey)).toBeNull();
        done();
      });
    });
  });
});
