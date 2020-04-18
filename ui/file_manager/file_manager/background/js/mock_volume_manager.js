// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Mock class for VolumeManager.
 * @constructor
 * @implements {VolumeManager}
 */
function MockVolumeManager() {
  this.volumeInfoList = new VolumeInfoListImpl();
  this.driveConnectionState = {
    type: VolumeManagerCommon.DriveConnectionType.ONLINE
  };
  this.createVolumeInfo(
      VolumeManagerCommon.VolumeType.DRIVE,
      'drive',
      str('DRIVE_DIRECTORY_LABEL'));
  this.createVolumeInfo(
      VolumeManagerCommon.VolumeType.DOWNLOADS,
      'downloads',
      str('DOWNLOADS_DIRECTORY_LABEL'));
}

/**
 * @private {?VolumeManager}
 */
MockVolumeManager.instance_ = null;

/**
 * Replaces the VolumeManager singleton with a MockVolumeManager.
 * @param {!MockVolumeManager=} opt_singleton
 */
MockVolumeManager.installMockSingleton = function(opt_singleton) {
  MockVolumeManager.instance_ = opt_singleton || new MockVolumeManager();

  volumeManagerFactory.getInstance = function() {
    return Promise.resolve(MockVolumeManager.instance_);
  };
};

/**
 * Creates, installs and returns a mock VolumeInfo instance.
 *
 * @param {!VolumeManagerCommon.VolumeType} type
 * @param {string} volumeId
 * @param {string} label
 *
 * @return {!VolumeInfo}
 */
MockVolumeManager.prototype.createVolumeInfo =
    function(type, volumeId, label) {
  var volumeInfo =
      MockVolumeManager.createMockVolumeInfo(type, volumeId, label);
  this.volumeInfoList.add(volumeInfo);
  return volumeInfo;
};

/**
 * Returns the corresponding VolumeInfo.
 *
 * @param {!Entry|!FakeEntry} entry FileEntry pointing anywhere on a volume.
 * @return {VolumeInfo} Corresponding VolumeInfo.
 */
MockVolumeManager.prototype.getVolumeInfo = function(entry) {
  return this.volumeInfoList.findByEntry(entry);
};

/**
 * Obtains location information from an entry.
 * Current implementation can handle only fake entries.
 *
 * @param {!Entry|!FakeEntry} entry A fake entry.
 * @return {EntryLocation} Location information.
 */
MockVolumeManager.prototype.getLocationInfo = function(entry) {
  if (util.isFakeEntry(entry)) {
    return new EntryLocationImpl(
        this.volumeInfoList.item(0), entry.rootType, true, true);
  }

  if (entry.filesystem.name === VolumeManagerCommon.VolumeType.DRIVE) {
    var volumeInfo = this.volumeInfoList.item(0);
    var rootType = VolumeManagerCommon.RootType.DRIVE;
    var isRootEntry = entry.fullPath === '/root';
    if (entry.fullPath.startsWith('/team_drives')) {
      if (entry.fullPath === '/team_drives') {
        rootType = VolumeManagerCommon.RootType.TEAM_DRIVES_GRAND_ROOT;
        isRootEntry = true;
      } else {
        rootType = VolumeManagerCommon.RootType.TEAM_DRIVE;
        isRootEntry = util.isTeamDriveRoot(entry);
      }
    }
    return new EntryLocationImpl(volumeInfo, rootType, isRootEntry, true);
  }

  throw new Error('Not implemented exception.');
};

/**
 * @param {VolumeManagerCommon.VolumeType} volumeType Volume type.
 * @return {VolumeInfo} Volume info.
 */
MockVolumeManager.prototype.getCurrentProfileVolumeInfo = function(volumeType) {
  for (var i = 0; i < this.volumeInfoList.length; i++) {
    var volumeInfo = this.volumeInfoList.item(i);
    if (volumeInfo.profile.isCurrentProfile &&
        volumeInfo.volumeType === volumeType)
      return volumeInfo;
  }
  return null;
};

/**
 * @return {VolumeManagerCommon.DriveConnectionState} Current drive connection
 *     state.
 */
MockVolumeManager.prototype.getDriveConnectionState = function() {
  return this.driveConnectionState;
};

/**
 * Utility function to create a mock VolumeInfo.
 * @param {!VolumeManagerCommon.VolumeType} type Volume type.
 * @param {string} volumeId Volume id.
 * @param {string} label Label.
 * @return {!VolumeInfo} Created mock VolumeInfo.
 */
MockVolumeManager.createMockVolumeInfo = function(type, volumeId, label) {
  var fileSystem = new MockFileSystem(volumeId, 'filesystem:' + volumeId);

  var volumeInfo = new VolumeInfoImpl(
      type, volumeId, fileSystem,
      '',                                          // error
      '',                                          // deviceType
      '',                                          // devicePath
      false,                                       // isReadOnly
      false,                                       // isReadOnlyRemovableDevice
      {isCurrentProfile: true, displayName: ''},   // profile
      label,                                       // label
      undefined,                                   // providerId
      false,                                       // hasMedia
      false,                                       // configurable
      false,                                       // watchable
      VolumeManagerCommon.Source.NETWORK,          // source
      VolumeManagerCommon.FileSystemType.UNKNOWN,  // diskFileSystemType
      {});                                         // iconSet

  return volumeInfo;
};

MockVolumeManager.prototype.mountArchive = function(
    fileUrl, successCallback, errorCallback) {
  throw new Error('Not implemented.');
};
MockVolumeManager.prototype.unmount = function(
    volumeInfo, successCallback, errorCallback) {
  throw new Error('Not implemented.');
};
MockVolumeManager.prototype.configure = function(volumeInfo) {
  throw new Error('Not implemented.');
};
MockVolumeManager.prototype.addEventListener = function(type, handler) {
  throw new Error('Not implemented.');
};
MockVolumeManager.prototype.removeEventListener = function(type, handler) {
  throw new Error('Not implemented.');
};
MockVolumeManager.prototype.dispatchEvent = function(event) {
  throw new Error('Not implemented.');
};

/**
 * Mock class for VolumeManagerWrapper.
 *
 * TODO(mtomasz): Merge mocks once VolumeManagerWrapper and VolumeManager
 * implement an identical interface.
 * @constructor
 */
function MockVolumeManagerWrapper() {
  this.volumeInfoList = new cr.ui.ArrayDataModel([]);
  this.driveConnectionState = {
    type: VolumeManagerCommon.DriveConnectionType.ONLINE
  };
  this.createVolumeInfo(
      VolumeManagerCommon.VolumeType.DRIVE,
      'drive',
      str('DRIVE_DIRECTORY_LABEL'));
  this.createVolumeInfo(
      VolumeManagerCommon.VolumeType.DOWNLOADS,
      'downloads',
      str('DOWNLOADS_DIRECTORY_LABEL'));
}
/**
 * @private {?VolumeManager}
 */
MockVolumeManagerWrapper.instance_ = null;
/**
 * Replaces the VolumeManager singleton with a MockVolumeManagerWrapper.
 * @param {!MockVolumeManagerWrapper=} opt_singleton
 */
MockVolumeManagerWrapper.installMockSingleton = function(opt_singleton) {
  MockVolumeManagerWrapper.instance_ =
      /** @type {!VolumeManager} */ (
          opt_singleton || new MockVolumeManagerWrapper());
  volumeManagerFactory.getInstance = function() {
    return Promise.resolve(MockVolumeManagerWrapper.instance_);
  };
};
/**
 * Creates, installs and returns a mock VolumeInfo instance.
 *
 * @param {!VolumeManagerCommon.VolumeType} type
 * @param {string} volumeId
 * @param {string} label
 *
 * @return {!VolumeInfo}
 */
MockVolumeManagerWrapper.prototype.createVolumeInfo =
    function(type, volumeId, label) {
  var volumeInfo =
      MockVolumeManagerWrapper.createMockVolumeInfo(type, volumeId, label);
  this.volumeInfoList.push(volumeInfo);
  return volumeInfo;
};
/**
 * Returns the corresponding VolumeInfo.
 *
 * @param {FileEntry} entry MockFileEntry pointing anywhere on a volume.
 * @return {VolumeInfo} Corresponding VolumeInfo.
 */
MockVolumeManagerWrapper.prototype.getVolumeInfo = function(entry) {
  for (var i = 0; i < this.volumeInfoList.length; i++) {
    if (this.volumeInfoList.item(i).volumeId === entry.filesystem.name)
      return /** @type {!VolumeInfo} */ (this.volumeInfoList.item(i));
  }
  return null;
};
/**
 * Obtains location information from an entry.
 * Current implementation can handle only fake entries.
 *
 * @param {!Entry} entry A fake entry.
 * @return {EntryLocation} Location information.
 */
MockVolumeManagerWrapper.prototype.getLocationInfo = function(entry) {
  var volumeInfo = /** @type {!VolumeInfo} */ (this.volumeInfoList.item(0));
  if (util.isFakeEntry(entry)) {
    var fakeEntry = /** @type {!FakeEntry} */ (entry);
    return new EntryLocationImpl(volumeInfo, fakeEntry.rootType, true, true);
  }
  if (entry.filesystem.name === VolumeManagerCommon.VolumeType.DRIVE) {
    var rootType = VolumeManagerCommon.RootType.DRIVE;
    var isRootEntry = entry.fullPath === '/root';
    if (entry.fullPath.startsWith('/team_drives')) {
      if (entry.fullPath === '/team_drives') {
        rootType = VolumeManagerCommon.RootType.TEAM_DRIVES_GRAND_ROOT;
        isRootEntry = true;
      } else {
        rootType = VolumeManagerCommon.RootType.TEAM_DRIVE;
        isRootEntry = util.isTeamDriveRoot(entry);
      }
    }
    return new EntryLocationImpl(volumeInfo, rootType, isRootEntry, true);
  }
  throw new Error('Not implemented exception.');
};
/**
 * @param {VolumeManagerCommon.VolumeType} volumeType Volume type.
 * @return {VolumeInfo} Volume info.
 */
MockVolumeManagerWrapper.prototype.getCurrentProfileVolumeInfo = function(
    volumeType) {
  return VolumeManager.prototype.getCurrentProfileVolumeInfo.call(
      /** @type {!VolumeManager} */ (this), volumeType);
};
/**
 * @return {VolumeManagerCommon.DriveConnectionState} Current drive connection
 *     state.
 */
MockVolumeManagerWrapper.prototype.getDriveConnectionState = function() {
  return this.driveConnectionState;
};
/**
 * Utility function to create a mock VolumeInfo.
 * @param {VolumeManagerCommon.VolumeType} type Volume type.
 * @param {string} volumeId Volume id.
 * @param {string} label Label.
 * @return {!VolumeInfo} Created mock VolumeInfo.
 */
MockVolumeManagerWrapper.createMockVolumeInfo =
    function(type, volumeId, label) {
  var fileSystem = new MockFileSystem(volumeId, 'filesystem:' + volumeId);
  var volumeInfo = new VolumeInfoImpl(
      type, volumeId, fileSystem,
      '',                                          // error
      '',                                          // deviceType
      '',                                          // devicePath
      false,                                       // isReadonly
      false,                                       // isReadOnlyRemovableDevice
      {isCurrentProfile: true, displayName: ''},   // profile
      label,                                       // label
      undefined,                                   // providerId
      false,                                       // hasMedia
      false,                                       // configurable
      false,                                       // watchable
      VolumeManagerCommon.Source.NETWORK,          // source
      VolumeManagerCommon.FileSystemType.UNKNOWN,  // diskFileSystemType
      {});                                         // iconSet
  return volumeInfo;
};
