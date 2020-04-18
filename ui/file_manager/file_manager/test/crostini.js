// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testCrostiniNotEnabled(done) {
  chrome.fileManagerPrivate.crostiniEnabled_ = false;
  fileManager.setupCrostini_();
  test.setupAndWaitUntilReady()
      .then(() => {
        return test.waitForElementLost(
            '#directory-tree .tree-item [root-type-icon="crostini"]');
      })
      .then(() => {
        done();
      });
}

function testCrostiniSuccess(done) {
  chrome.fileManagerPrivate.crostiniEnabled_ = true;
  var oldMount = chrome.fileManagerPrivate.mountCrostiniContainer;
  var mountCallback = null;
  chrome.fileManagerPrivate.mountCrostiniContainer = (callback) => {
    mountCallback = callback;
  };
  fileManager.setupCrostini_();
  test.setupAndWaitUntilReady()
      .then(() => {
        // Linux Files fake root is shown.
        return test.waitForElement(
            '#directory-tree .tree-item [root-type-icon="crostini"]');
      })
      .then(() => {
        // Click on Linux Files.
        assertTrue(
            test.fakeMouseClick(
                '#directory-tree .tree-item [root-type-icon="crostini"]'),
            'click linux files');
        return test.waitForElement('paper-progress:not([hidden])');
      })
      .then(() => {
        // Ensure mountCrostiniContainer is called.
        return test.repeatUntil(() => {
          if (!mountCallback)
            return test.pending('Waiting for mountCrostiniContainer');
          return mountCallback;
        });
      })
      .then(() => {
        // Intercept the fileManagerPrivate.mountCrostiniContainer call
        // and add crostini disk mount.
        var volumeInfo = mockVolumeManager.createVolumeInfo(
            VolumeManagerCommon.VolumeType.CROSTINI, 'crostini',
            str('LINUX_FILES_ROOT_LABEL'));
        volumeInfo.fileSystem.populate(
            test.TestEntryInfo.getMockFileSystemPopulateRows(
                test.CROSTINI_ENTRY_SET, '/'),
            true);
        chrome.fileManagerPrivate.dispatchEvent_('onMountCompleted', {
          status: 'success',
          eventType: 'mount',
          volumeMetadata: {
            volumeType: VolumeManagerCommon.VolumeType.CROSTINI,
            volumeId: 'crostini',
            isReadOnly: false,
            iconSet: {},
            profile: {isCurrentProfile: true, displayName: ''},
            mountContext: 'user',
          },
        });
        // Continue from fileManagerPrivate.mountCrostiniContainer callback
        // and ensure expected files are shown.
        mountCallback();
        return test.waitForFiles(
            test.TestEntryInfo.getExpectedRows(test.CROSTINI_ENTRY_SET));
      })
      .then(() => {
        // Reset fileManagerPrivate.mountCrostiniContainer and remove mount.
        chrome.fileManagerPrivate.mountCrostiniContainer = oldMount;
        chrome.fileManagerPrivate.removeMount('crostini');
        // Linux Files fake root is shown.
        return test.waitForElement(
            '#directory-tree .tree-item [root-type-icon="crostini"]');
      })
      .then(() => {
        // Downloads folder should be shown when crostini goes away.
        return test.waitForFiles(
            test.TestEntryInfo.getExpectedRows(test.BASIC_LOCAL_ENTRY_SET));
      })
      .then(() => {
        done();
      });
}

function testCrostiniError(done) {
  chrome.fileManagerPrivate.crostiniEnabled_ = true;
  var oldMount = chrome.fileManagerPrivate.mountCrostiniContainer;
  // Override fileManagerPrivate.mountCrostiniContainer to return error.
  chrome.fileManagerPrivate.mountCrostiniContainer = (callback) => {
    chrome.runtime.lastError = {message: 'test message'};
    callback();
    chrome.runtime.lastError = null;
  };
  fileManager.setupCrostini_();
  test.setupAndWaitUntilReady()
      .then(() => {
        return test.waitForElement(
            '#directory-tree .tree-item [root-type-icon="crostini"]');
      })
      .then(() => {
        // Click on Linux Files, ensure error dialog is shown.
        assertTrue(test.fakeMouseClick(
            '#directory-tree .tree-item [root-type-icon="crostini"]'));
        return test.waitForElement('.cr-dialog-container.shown');
      })
      .then(() => {
        // Click OK button to close.
        assertTrue(test.fakeMouseClick('button.cr-dialog-ok'));
        return test.waitForElementLost('.cr-dialog-container.shown');
      })
      .then(() => {
        // Reset chrome.fileManagerPrivate.mountCrostiniContainer.
        chrome.fileManagerPrivate.mountCrostiniContainer = oldMount;
        done();
      });
}
