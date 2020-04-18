// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @const {!Event} */
var EMPTY_EVENT = new Event('directory-changed');

/** @type {!MockVolumeManager} */
var volumeManager;

/** @type {!TestMediaScanner} */
var mediaScanner;

/** @type {!TestImportRunner} */
var mediaImporter;

/** @type {!TestControllerEnvironment} */
var environment;

/** @type {!VolumeInfo} */
var sourceVolume;

/** @type {!VolumeInfo} */
var destinationVolume;

/** @type {!importer.TestCommandWidget} */
var widget;

/** @type {!DirectoryEntry} */
var nonDcimDirectory;

/**
 * @enum {string}
 */
var MESSAGES = {
  CLOUD_IMPORT_BUTTON_LABEL: 'Import it!',
  CLOUD_IMPORT_ACTIVE_IMPORT_BUTTON_LABEL: 'Already importing!',
  CLOUD_IMPORT_EMPTY_SCAN_BUTTON_LABEL: 'No new media',
  CLOUD_IMPORT_INSUFFICIENT_SPACE_BUTTON_LABEL: 'Not enough space!',
  CLOUD_IMPORT_SCANNING_BUTTON_LABEL: 'Scanning... ...!',
  DRIVE_DIRECTORY_LABEL: 'My Drive',
  DOWNLOADS_DIRECTORY_LABEL: 'Downloads'
};

// Set up string assets.
loadTimeData.data = MESSAGES;

function setUp() {
  new MockChromeStorageAPI();
  new MockCommandLinePrivate();

  widget = new importer.TestCommandWidget();

  nonDcimDirectory = new MockDirectoryEntry(
      new MockFileSystem('testFs'),
      '/jellybeans/');

  volumeManager = new MockVolumeManager();
  MockVolumeManager.installMockSingleton(volumeManager);

  destinationVolume = volumeManager.getCurrentProfileVolumeInfo(
      VolumeManagerCommon.VolumeType.DOWNLOADS);

  mediaScanner = new TestMediaScanner();
  mediaImporter = new TestImportRunner();
}

function testClickMainToStartImport(callback) {
  reportPromise(
      startImport(importer.ClickSource.MAIN),
      callback);
}

function testClickPanelToStartImport(callback) {
  reportPromise(
      startImport(importer.ClickSource.IMPORT),
      callback);
}

function testClickCancel(callback) {
  var promise = startImport(importer.ClickSource.IMPORT)
      .then(
          function(task) {
            widget.click(importer.ClickSource.CANCEL);
            return task.whenCanceled;
          });

  reportPromise(promise, callback);
}

function testVolumeUnmount_InvalidatesScans(callback) {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
        '/DCIM/photos0/IMG00002.jpg',
        '/DCIM/photos1/',
        '/DCIM/photos1/IMG00001.jpg',
        '/DCIM/photos1/IMG00003.jpg'
      ],
      '/DCIM');

  var dcim = environment.getCurrentDirectory();

  environment.directoryChangedListener(EMPTY_EVENT);
  var promise = widget.updateResolver.promise.then(
      function() {
        // Reset the promise so we can wait on a second widget update.
        widget.resetPromises();
        environment.setCurrentDirectory(nonDcimDirectory);
        environment.simulateUnmount();

        environment.setCurrentDirectory(dcim);
        environment.directoryChangedListener(EMPTY_EVENT);
        // Return the new promise, so subsequent "thens" only
        // fire once the widget has been updated again.
        return widget.updateResolver.promise;
      }).then(
          function() {
            mediaScanner.assertScanCount(2);
          });

  reportPromise(promise, callback);
}

function testDirectoryChange_TriggersUpdate(callback) {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
      ],
      '/DCIM');

  environment.directoryChangedListener(EMPTY_EVENT);
  reportPromise(widget.updateResolver.promise, callback);
}

function testDirectoryChange_CancelsScan(callback) {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
        '/DCIM/photos0/IMG00002.jpg',
        '/DCIM/photos1/',
        '/DCIM/photos1/IMG00001.jpg',
        '/DCIM/photos1/IMG00003.jpg'
      ],
      '/DCIM');

  environment.directoryChangedListener(EMPTY_EVENT);
  var promise = widget.updateResolver.promise.then(
      function() {
        // Reset the promise so we can wait on a second widget update.
        widget.resetPromises();
        environment.setCurrentDirectory(nonDcimDirectory);
        environment.directoryChangedListener(EMPTY_EVENT);
      }).then(
          function() {
            mediaScanner.assertScanCount(1);
            mediaScanner.assertLastScanCanceled();
          });

  reportPromise(promise, callback);
}

function testWindowClose_CancelsScan(callback) {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
        '/DCIM/photos0/IMG00002.jpg',
        '/DCIM/photos1/',
        '/DCIM/photos1/IMG00001.jpg',
        '/DCIM/photos1/IMG00003.jpg'
      ],
      '/DCIM');

  environment.directoryChangedListener(EMPTY_EVENT);
  var promise = widget.updateResolver.promise.then(
      function() {
        // Reset the promise so we can wait on a second widget update.
        widget.resetPromises();
        environment.windowCloseListener();
      }).then(
          function() {
            mediaScanner.assertScanCount(1);
            mediaScanner.assertLastScanCanceled();
          });

  reportPromise(promise, callback);
}

function testDirectoryChange_DetailsPanelVisibility_InitialChangeDir(callback) {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
      ],
      '/DCIM');

  var fileSystem = new MockFileSystem('testFs');
  var event = new Event('directory-changed');
  event.newDirEntry = new MockDirectoryEntry(
      fileSystem,
      '/DCIM/');
  // ensure there is some content in the scan so the code that depends
  // on this state doesn't croak which it finds it missing.
  mediaScanner.fileEntries.push(
      new MockFileEntry(fileSystem, '/DCIM/photos0/IMG00001.jpg', {size: 0}));

  // Make controller enter a scanning state.
  environment.directoryChangedListener(event);
  assertFalse(widget.detailsVisible);

  var promise = widget.updateResolver.promise.then(function() {
    // "scanning..."
    assertFalse(widget.detailsVisible);
    widget.resetPromises();
    mediaScanner.finalizeScans();
    return widget.updateResolver.promise;
  }).then(function() {
    // "ready to update"
    // Details should pop up.
    assertTrue(widget.detailsVisible);
  });
  reportPromise(promise, callback);
}

function testDirectoryChange_DetailsPanelVisibility_SubsequentChangeDir() {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
      ],
      '/DCIM');

  var event = new Event('directory-changed');
  event.newDirEntry = new MockDirectoryEntry(
      new MockFileSystem('testFs'),
      '/DCIM/');
  // Any previous dir at all will skip the new window logic.
  event.previousDirEntry = event.newDirEntry;

  environment.directoryChangedListener(event);
  assertFalse(widget.detailsVisible);
}

function testSelectionChange_TriggersUpdate(callback) {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
      ],
      '/DCIM');

  var fileSystem = new MockFileSystem('testFs');
  // ensure there is some content in the scan so the code that depends
  // on this state doesn't croak which it finds it missing.
  environment.selection.push(
      new MockFileEntry(fileSystem, '/DCIM/photos0/IMG00001.jpg', {size: 0}));

  environment.selectionChangedListener();
  mediaScanner.finalizeScans();
  reportPromise(widget.updateResolver.promise, callback);
}

function testFinalizeScans_TriggersUpdate(callback) {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
      ],
      '/DCIM');

  var fileSystem = new MockFileSystem('testFs');
  // ensure there is some content in the scan so the code that depends
  // on this state doesn't croak which it finds it missing.
  mediaScanner.fileEntries.push(
      new MockFileEntry(fileSystem, '/DCIM/photos0/IMG00001.jpg', {size: 0}));

  environment.directoryChangedListener(EMPTY_EVENT);  // initiates a scan.
  widget.resetPromises();
  mediaScanner.finalizeScans();

  reportPromise(widget.updateResolver.promise, callback);
}

function testClickDestination_ShowsRootPriorToImport(callback) {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
      ],
      '/DCIM');

  widget.click(importer.ClickSource.DESTINATION);

  reportPromise(environment.showImportRootResolver.promise, callback);
}

function testClickDestination_ShowsDestinationAfterImportStarted(callback) {
  var promise = startImport(importer.ClickSource.MAIN)
      .then(
          function() {
            return mediaImporter.importResolver.promise.then(
                function() {
                  widget.click(importer.ClickSource.DESTINATION);
                  return
                    environment.showImportDestinationResolver.promise;
                });
          });

  reportPromise(promise, callback);
}

function startImport(clickSource) {
  var controller = createController(
      VolumeManagerCommon.VolumeType.MTP,
      'mtp-volume',
      [
        '/DCIM/',
        '/DCIM/photos0/',
        '/DCIM/photos0/IMG00001.jpg',
        '/DCIM/photos0/IMG00002.jpg',
      ],
      '/DCIM');

  var fileSystem = new MockFileSystem('testFs');
  // ensure there is some content in the scan so the code that depends
  // on this state doesn't croak which it finds it missing.
  mediaScanner.fileEntries.push(
      new MockFileEntry(fileSystem, '/DCIM/photos0/IMG00001.jpg', {size: 0}));

  // First we need to force the controller into a scanning state.
  environment.directoryChangedListener(EMPTY_EVENT);

  return widget.updateResolver.promise.then(
      function() {
        widget.resetPromises();
        mediaScanner.finalizeScans();
        return widget.updateResolver.promise.then(
            function() {
              widget.resetPromises();
              widget.click(clickSource);
              return mediaImporter.importResolver.promise;
            });
      });
}

/**
 * A stub that just provides interfaces from ImportTask that are required by
 * these tests.
 *
 * @constructor
 *
 * @param {!impoter.ScanResult} scan
 * @param {!impoter.Destination} destination
 * @param {DirectoryEntry} destinationDirectory
 */
function TestImportTask(scan, destination, destinationDirectory) {

  /** @public {!importer.ScanResult} */
  this.scan = scan;

  /** @type {!impoter.Destination} */
  this.destination = destination;

  /** @type {!DirectoryEntry} */
  this.destinationDirectory = destinationDirectory;

  /** @private {!importer.Resolver} */
  this.finishedResolver_ = new importer.Resolver();

  /** @private {!importer.Resolver} */
  this.canceledResolver_ = new importer.Resolver();

  /** @public {!Promise} */
  this.whenFinished = this.finishedResolver_.promise;

  /** @public {!Promise} */
  this.whenCanceled = this.canceledResolver_.promise;
}

/** @return {!Promise<DirectoryEntry>} */
TestImportTask.prototype.finish = function() {
  this.finishedResolver_.resolve();
};

/** @return {!Promise<DirectoryEntry>} */
TestImportTask.prototype.requestCancel = function() {
  this.canceledResolver_.resolve();
};

/**
 * Test import runner.
 *
 * @constructor
 */
function TestImportRunner() {
  /** @public {!Array<!importer.ScanResult>} */
  this.imported = [];

  /**
   * Resolves when import is started.
   * @public {!importer.Resolver.<!TestImportTask>}
   */
  this.importResolver = new importer.Resolver();

  /** @private {!Array<!TestImportTask>} */
  this.tasks_ = [];
}

/** @override */
TestImportRunner.prototype.importFromScanResult =
    function(scan, destination, destinationDirectory) {
  this.imported.push(scan);
  var task = new TestImportTask(scan, destination, destinationDirectory);
  this.tasks_.push(task);
  this.importResolver.resolve(task);
  return task;
};

/**
 * @param {!DirectoryEntry} destination
 */
TestImportRunner.prototype.finishImportTasks = function() {
  this.tasks_.forEach(
      function(task) {
        task.finish();
      });
};

/**
 * @param {!DirectoryEntry} destination
 */
TestImportRunner.prototype.cancelImportTasks = function() {
  // No diff to us.
  this.finishImportTasks();
};

/**
 * @param {number} expected
 */
TestImportRunner.prototype.assertImportsStarted = function(expected) {
  assertEquals(expected, this.imported.length);
};

/**
 * Interface abstracting away the concrete file manager available
 * to commands. By hiding file manager we make it easy to test
 * importer.ImportController.
 *
 * @constructor
 * @implements {importer.CommandInput}
 *
 * @param {!VolumeInfo} volumeInfo
 * @param {!DirectoryEntry} directory
 */
TestControllerEnvironment = function(volumeInfo, directory) {
  /** @private {!VolumeInfo} */
  this.volumeInfo_ = volumeInfo;

  /** @private {!DirectoryEntry} */
  this.directory_ = directory;

  /** @public {function()} */
  this.windowCloseListener;

  /** @public {function(string)} */
  this.volumeUnmountListener;

  /** @public {function()} */
  this.directoryChangedListener;

  /** @public {function()} */
  this.selectionChangedListener;

  /** @public {!Entry} */
  this.selection = [];

  /** @public {boolean} */
  this.isDriveMounted = true;

  /** @public {number} */
  this.freeStorageSpace = 123456789;  // bytes

  /** @public {!importer.Resolver} */
  this.showImportRootResolver = new importer.Resolver();

  /** @public {!importer.Resolver} */
  this.showImportDestinationResolver = new importer.Resolver();
};

/** @override */
TestControllerEnvironment.prototype.getSelection =
    function() {
  return this.selection;
};

/** @override */
TestControllerEnvironment.prototype.getCurrentDirectory =
    function() {
  return this.directory_;
};

/** @override */
TestControllerEnvironment.prototype.setCurrentDirectory = function(entry) {
  this.directory_ = entry;
};

/** @override */
TestControllerEnvironment.prototype.getVolumeInfo =
    function(entry) {
  return this.volumeInfo_;
};

/** @override */
TestControllerEnvironment.prototype.isGoogleDriveMounted =
    function() {
  return this.isDriveMounted;
};

/** @override */
TestControllerEnvironment.prototype.getFreeStorageSpace =
    function() {
  return Promise.resolve(this.freeStorageSpace);
};

/** @override */
TestControllerEnvironment.prototype.addWindowCloseListener =
    function(listener) {
  this.windowCloseListener = listener;
};

/** @override */
TestControllerEnvironment.prototype.addVolumeUnmountListener =
    function(listener) {
  this.volumeUnmountListener = listener;
};

/** @override */
TestControllerEnvironment.prototype.addDirectoryChangedListener =
    function(listener) {
  this.directoryChangedListener = listener;
};

/** @override */
TestControllerEnvironment.prototype.addSelectionChangedListener =
    function(listener) {
  this.selectionChangedListener = listener;
};

/** @override */
TestControllerEnvironment.prototype.getImportDestination =
    function(date) {
  var fileSystem = new MockFileSystem('testFs');
  return new MockDirectoryEntry(fileSystem, '/abc/123');
};

/** @override */
TestControllerEnvironment.prototype.showImportDestination = function() {
  this.showImportDestinationResolver.resolve();
};

/** @override */
TestControllerEnvironment.prototype.showImportRoot = function() {
  this.showImportRootResolver.resolve();
};

/**
 * Simulates an unmount event.
 */
TestControllerEnvironment.prototype.simulateUnmount = function() {
  this.volumeUnmountListener(this.volumeInfo_.volumeId);
};

/**
 * Test implementation of importer.CommandWidget.
 *
 * @constructor
 * @implements {importer.CommandWidget}
 * @struct
 */
importer.TestCommandWidget = function() {
  /** @public {function()} */
  this.clickListener;

  /** @public {!importer.Resolver.<!importer.CommandUpdate>} */
  this.updateResolver = new importer.Resolver();

  /** @public {!importer.Resolver.<!importer.CommandUpdate>} */
  this.toggleDetailsResolver = new importer.Resolver();

  /** @public {boolean} */
  this.detailsVisible = false;
};

/** Resets the widget */
importer.TestCommandWidget.prototype.resetPromises = function() {
  this.updateResolver = new importer.Resolver();
  this.toggleDetailsResolver = new importer.Resolver();
};

/** @override */
importer.TestCommandWidget.prototype.addClickListener =
    function(listener) {
  this.clickListener = listener;
};

/**
 * Fires faux click.
 * @param  {!importer.ClickSource} source
 */
importer.TestCommandWidget.prototype.click = function(source) {
  this.clickListener(source);
};

/** @override */
importer.TestCommandWidget.prototype.update = function(update) {
  assertFalse(
      this.updateResolver.settled,
      'Update promise should not have been settled.');
  this.updateResolver.resolve(update);
};

/** @override */
importer.TestCommandWidget.prototype.updateDetails = function(scan) {
  // TODO(smckay)
};

/** @override */
importer.TestCommandWidget.prototype.performMainButtonRippleAnimation =
    function() {};

/** @override */
importer.TestCommandWidget.prototype.toggleDetails = function() {
  assertFalse(
      this.toggleDetailsResolver.settled,
      'Toggle details promise should not have been settled.');
  this.setDetailsVisible(!this.detailsVisible);
  this.toggleDetailsResolver.resolve();
};

/** @override */
importer.TestCommandWidget.prototype.setDetailsVisible = function(visible) {
  this.detailsVisible = visible;
};

/** @override */
importer.TestCommandWidget.prototype.setDetailsBannerVisible =
    function(visible) {
  // TODO(smckay)
};

/**
 * @param {!VolumeManagerCommon.VolumeType} volumeType
 * @param {string} volumeId
 * @param {!Array<string>} fileNames
 * @param {string} currentDirectory
 * @return {!importer.ImportControler}
 */
function createController(volumeType, volumeId, fileNames, currentDirectory) {
  sourceVolume = setupFileSystem(
      volumeType,
      volumeId,
      fileNames);

  environment = new TestControllerEnvironment(
      sourceVolume,
      sourceVolume.fileSystem.entries[currentDirectory]);

  return new importer.ImportController(
      environment,
      mediaScanner,
      mediaImporter,
      widget,
      new TestTracker());
}

/**
 * @param {!VolumeManagerCommon.VolumeType} volumeType
 * @param {string} volumeId
 * @param {!Array<string>} fileNames
 * @return {!VolumeInfo}
 */
function setupFileSystem(volumeType, volumeId, fileNames) {
  var volumeInfo = volumeManager.createVolumeInfo(
      volumeType, volumeId, 'A volume known as ' + volumeId);
  assertTrue(volumeInfo != null);
  volumeInfo.fileSystem.populate(fileNames);
  return volumeInfo;
}
