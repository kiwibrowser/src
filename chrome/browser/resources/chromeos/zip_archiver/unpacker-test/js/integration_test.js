// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Simulates extension unload in case of suspend or restarts by clearing
 * 'unpacker.app' members. No need to remove the NaCl module. The reason is
 * that we have to load the NaCl module manually by ourself anyway (in real
 * scenarios the browser does it), so this will only take extra time without
 * really testing anything.
 */
var unloadExtension = function() {
  for (var fileSystemId in unpacker.app.volumes) {
    unpacker.app.cleanupVolume(fileSystemId);
  }
  expect(Object.keys(unpacker.app.volumes).length).to.equal(0);
  expect(Object.keys(unpacker.app.volumeLoadedPromises).length).to.equal(0);
};

// We need at least two zip files for all the tests to run correctly.
var zipList = [
  {
    name: 'small_zip.zip',
    afterOnLaunchTests: function() {
      smallArchiveCheck('small_zip.zip', SMALL_ZIP_METADATA, false, null);
    },
    afterSuspendTests: function() {
      smallArchiveCheck('small_zip.zip', SMALL_ZIP_METADATA, true, null);
      smallArchiveCheckAfterSuspend('small_zip.zip');
    },
    afterRestartTests: function() {
      smallArchiveCheckAfterRestart('small_zip.zip');
      smallArchiveCheck('small_zip.zip', SMALL_ZIP_METADATA, true, null);
    }
  },
  {
    name: 'encrypted.zip',
    afterOnLaunchTests: function() {
      // TODO(mtomasz): Add tests for clicking the Cancel button in the
      // passphrase dialog.
      smallArchiveCheck(
          'encrypted.zip', SMALL_ZIP_METADATA, false, ENCRYPTED_ZIP_PASSPHRASE);
    },
    afterSuspendTests: function() {
      smallArchiveCheck('encrypted.zip', SMALL_ZIP_METADATA, true, null);
      smallArchiveCheckAfterSuspend('encrypted.zip');
    },
    afterRestartTests: function() {
      smallArchiveCheckAfterRestart('encrypted.zip');
      smallArchiveCheck('encrypted.zip', SMALL_ZIP_METADATA, true, null);
    }
  }
];

// Init helper.
var initPromise = tests_helper.init(zipList);

describe('The unpacker', function() {
  var mountProcessCounterBefore;

  before(function(done) {
    // Modify the default timeout until Karma kills the test and marks it as
    // failed in order to give enough time to the browser to load the PNaCl
    // module. First load requires more than the default 2000 ms timeout, but
    // next loads will be faster due to caching in user-data-dir.
    // Timeout is set per every before, beforeEach, it, afterEach, so there is
    // no need to restore it.
    this.timeout(15000 /* milliseconds */);

    // Cannot use 'after' because 'after' gets executed after all 'before'
    // even though 'before' is in another 'describe'. But 'before' gets
    // correctly executed for every 'describe'. So below workaround solves
    // this problem.
    if (unpacker.app.naclModuleIsLoaded())
      unpacker.app.unloadNaclModule();
    expect(unpacker.app.naclModuleIsLoaded()).to.be.false;

    // Rewrite the path of .nmf file.
    unpacker.app.DEFAULT_MODULE_NMF = 'base/module.nmf';

    // Load the module.
    unpacker.app.loadNaclModule('base/module.nmf', 'application/x-pnacl');

    Promise.all([initPromise, unpacker.app.moduleLoadedPromise])
        .then(function() {
          // Here, loadNaclModule() should unload the NaCL module immediately
          // after the loading.
          expect(unpacker.app.naclModuleIsLoaded()).to.be.false;
          // In case below is not printed probably 5000 ms for this.timeout
          // wasn't enough for PNaCl to load during first time run.
          console.debug('Initialization and module loading finished.');
          done();
        })
        .catch(tests_helper.forceFailure);
  });

  beforeEach(function(done) {
    // Called on beforeEach() in order for spies and stubs to reset registered
    // number of calls to methods.
    tests_helper.initChromeApis();

    var launchData = {items: []};
    tests_helper.volumesInformation.forEach(function(volume) {
      launchData.items.push({entry: volume.entry});
    });

    var successfulVolumeLoads = 0;
    mountProcessCounterBefore = unpacker.app.mountProcessCounter;
    unpacker.app.onLaunched(
        launchData,
        function(fileSystemId) {
          successfulVolumeLoads++;
          if (successfulVolumeLoads == tests_helper.volumesInformation.length)
            done();
        },
        function(fileSystemId) {
          tests_helper.forceFailure(
              'Could not load volume <' + fileSystemId + '>.');
        });
  });

  afterEach(function() {
    unloadExtension();
  });

  // Check if volumes were correctly loaded.
  tests_helper.volumesInformation.forEach(function(volumeInformation) {
    describe(
        'that launches <' + volumeInformation.fileSystemId + '>', function() {
          volumeInformation.afterOnLaunchTests();
        });
  });

  // Test mountProcessCounter for onLaunched.
  it('should have the same mountProcessCounter before and after onLaunched ' +
         'call',
     function() {
       expect(mountProcessCounterBefore)
           .to.equal(unpacker.app.mountProcessCounter);
     });

  // Test state save.
  describe('should save state in case of restarts or crashes', function() {
    it('by calling retainEntry with the volume\'s entry', function() {
      expect(chrome.fileSystem.retainEntry.callCount)
          .to.equal(tests_helper.volumesInformation.length);
      tests_helper.volumesInformation.forEach(function(volume) {
        expect(chrome.fileSystem.retainEntry.calledWith(volume.entry))
            .to.be.true;
      });
    });

    it('by storing the volumes state', function() {
      tests_helper.volumesInformation.forEach(function(volumeInformation) {
        var fileSystemId = volumeInformation.fileSystemId;
        expect(tests_helper
                   .localStorageState[unpacker.app.STORAGE_KEY][fileSystemId])
            .to.not.be.undefined;
      });
      expect(chrome.storage.local.set.called).to.be.true;
    });
  });

  // Test restore after suspend page event.
  describe('that receives a suspend page event', function() {
    beforeEach(function() {
      // Reinitialize spies in order to register only the calls after suspend
      // and not before it.
      tests_helper.initChromeApis();

      unpacker.app.onSuspend();  // This gets called before suspend.
      unloadExtension();

      // Remember the state before suspend. Used to test correct restoring
      // after suspend.
      tests_helper.volumesInformation.forEach(function(volumeInformation) {
        var fileSystemId = volumeInformation.fileSystemId;
        volumeInformation.fileSystemMetadata.openedFiles =
            getOpenedFilesBeforeSuspend(fileSystemId);
        tests_helper.localStorageState[unpacker.app.STORAGE_KEY][fileSystemId]
            .passphrase = ENCRYPTED_ZIP_PASSPHRASE;
      });
    });

    it('should call retainEntry again for all mounted volumes', function() {
      expect(chrome.fileSystem.retainEntry.callCount)
          .to.equal(tests_helper.volumesInformation.length);
    });

    it('should store the volumes state for all mounted volumes', function() {
      tests_helper.volumesInformation.forEach(function(volumeInformation) {
        var fileSystemId = volumeInformation.fileSystemId;
        expect(tests_helper
                   .localStorageState[unpacker.app.STORAGE_KEY][fileSystemId])
            .to.not.be.undefined;
      });
      expect(chrome.storage.local.set.called).to.be.true;
    });

    // Check if restore was successful.
    tests_helper.volumesInformation.forEach(function(volumeInformation) {
      volumeInformation.afterSuspendTests();
    });
  });

  // Test restore after restarts, crashes, etc.
  describe('that is restarted', function() {
    beforeEach(function() {
      // Set the opened files before restart. Used to test correct restoring
      // after restart.
      tests_helper.volumesInformation.forEach(function(volumeInformation) {
        var fileSystemId = volumeInformation.fileSystemId;
        volumeInformation.fileSystemMetadata.openedFiles =
            getOpenedFilesBeforeSuspend(fileSystemId);
        tests_helper.localStorageState[unpacker.app.STORAGE_KEY][fileSystemId]
            .passphrase = ENCRYPTED_ZIP_PASSPHRASE;
      });

      unloadExtension();
      tests_helper.volumesInformation.forEach(function(volumeInformation) {
        volumeInformation.fileSystemMetadata.openedFiles = [];
      });

      // Reset spies and stubs.
      tests_helper.initChromeApis();
    });

    // Check if restore was successful.
    tests_helper.volumesInformation.forEach(function(volumeInformation) {
      volumeInformation.afterRestartTests();
    });
  });

  // Check unmount.
  tests_helper.volumesInformation.forEach(function(volumeInformation) {
    var fileSystemId = volumeInformation.fileSystemId;
    var storageKey = unpacker.app.STORAGE_KEY;

    describe('that unmounts volume <' + fileSystemId + '>', function() {
      beforeEach(function(done) {
        // Reinitialize spies in order to register only the calls after
        // suspend and not before it.
        tests_helper.initChromeApis();

        expect(unpacker.app.volumes[fileSystemId]).to.not.be.undefined;
        expect(tests_helper.localStorageState[storageKey][fileSystemId])
            .to.not.be.undefined;
        unpacker.app.onUnmountRequested(
            {fileSystemId: fileSystemId}, function() {
              done();
            }, tests_helper.forceFailure);
      });

      it('should remove volume from unpacker.app.volumes', function() {
        expect(unpacker.app.volumes[fileSystemId]).to.be.undefined;
      });

      it('should not call retainEntry', function() {
        expect(chrome.fileSystem.retainEntry.called).to.be.false;
      });

      it('should remove volume from local storage', function() {
        expect(tests_helper.localStorageState[storageKey][fileSystemId])
            .to.be.undefined;
        expect(chrome.storage.local.set.called).to.be.true;
      });
    });
  });

  // Check if NaCL module is NOT unloaded when one volume is unloaded and
  // Zip Unpacker still has other mounted volumes.
  describe(
      'that unmounts a volume and still has unmounted volumes', function() {
        beforeEach(function(done) {
          // We need at least 2 zip files mounted to run this test.
          expect(tests_helper.volumesInformation.length >= 2).to.be.true;
          // Only unload the first volume.
          var volumeInformation = tests_helper.volumesInformation[0];
          var fileSystemId = volumeInformation.fileSystemId;
          tests_helper.initChromeApis();
          unpacker.app.onUnmountRequested(
              {fileSystemId: fileSystemId}, function() {
                // Make sure that there is at least one volume that is not
                // unmounted.
                expect(Object.keys(unpacker.app.volumes).length > 0).to.be.true;
                done();
              }.bind(this), tests_helper.forceFailure);
        });

        it('should not unload the NaCL module', function() {
          expect(unpacker.app.naclModule).to.not.be.null;
          expect(unpacker.app.moduleLoadedPromise).to.not.be.null;
        });
      });

  // Check if NaCL module is unloaded after all the volumes are unloaded.
  describe('that unmounts all the volumes', function() {
    beforeEach(function(done) {
      // The number of mounted volumes.
      var volumeListSize = zipList.length;
      tests_helper.volumesInformation.forEach(function(volumeInformation) {
        var fileSystemId = volumeInformation.fileSystemId;
        tests_helper.initChromeApis();
        unpacker.app.onUnmountRequested(
            {fileSystemId: fileSystemId}, function() {
              // Wait for all the volumes to be unloaded.
              volumeListSize--;
              if (volumeListSize == 0)
                done();
            }, tests_helper.forceFailure);
      });
    });

    it('should unload the NaCL module', function() {
      expect(unpacker.app.naclModule).to.be.null;
      expect(unpacker.app.moduleLoadedPromise).to.be.null;
    });
  });
});
