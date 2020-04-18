// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * The container of the VolumeInfo for each mounted volume.
 * @constructor
 * @implements {VolumeInfoList}
 */
function VolumeInfoListImpl() {
  var field = 'volumeType,volumeId';

  /**
   * Holds VolumeInfo instances.
   * @type {cr.ui.ArrayDataModel}
   * @private
   */
  this.model_ = new cr.ui.ArrayDataModel([]);
  this.model_.setCompareFunction(field,
                                 /** @type {function(*, *): number} */
                                 (VolumeInfoListImpl.compareVolumeInfo_));
  this.model_.sort(field, 'asc');

  Object.freeze(this);
}

/**
 * The order of the volume list based on root type.
 * @type {Array<VolumeManagerCommon.VolumeType>}
 * @const
 * @private
 */
VolumeInfoListImpl.VOLUME_LIST_ORDER_ = [
  VolumeManagerCommon.VolumeType.DRIVE,
  VolumeManagerCommon.VolumeType.DOWNLOADS,
  VolumeManagerCommon.VolumeType.CROSTINI,
  VolumeManagerCommon.VolumeType.ANDROID_FILES,
  VolumeManagerCommon.VolumeType.ARCHIVE,
  VolumeManagerCommon.VolumeType.REMOVABLE,
  VolumeManagerCommon.VolumeType.MTP,
  VolumeManagerCommon.VolumeType.PROVIDED,
  VolumeManagerCommon.VolumeType.MEDIA_VIEW,
];

/**
 * The order of the media view roots.
 * @type {Array<VolumeManagerCommon.MediaViewRootType>}
 * @const
 * @private
 */
VolumeInfoListImpl.MEDIA_VIEW_ROOT_ORDER_ = [
  VolumeManagerCommon.MediaViewRootType.IMAGES,
  VolumeManagerCommon.MediaViewRootType.VIDEOS,
  VolumeManagerCommon.MediaViewRootType.AUDIO,
];

/**
 * Orders two volumes by volumeType and volumeId.
 *
 * The volumes at first are compared by volume type in the order of
 * volumeListOrder_.  Then they are compared by volume ID, except for media
 * views which are sorted in a fixed order.
 *
 * @param {!VolumeInfo} volumeInfo1 Volume info to be compared.
 * @param {!VolumeInfo} volumeInfo2 Volume info to be compared.
 * @return {number} Returns -1 if volume1 < volume2, returns 1 if volume2 >
 *     volume1, returns 0 if volume1 === volume2.
 * @private
 */
VolumeInfoListImpl.compareVolumeInfo_ = function(volumeInfo1, volumeInfo2) {
  var typeIndex1 =
      VolumeInfoListImpl.VOLUME_LIST_ORDER_.indexOf(volumeInfo1.volumeType);
  var typeIndex2 =
      VolumeInfoListImpl.VOLUME_LIST_ORDER_.indexOf(volumeInfo2.volumeType);
  if (typeIndex1 !== typeIndex2)
    return typeIndex1 < typeIndex2 ? -1 : 1;
  if (volumeInfo1.volumeType === VolumeManagerCommon.VolumeType.MEDIA_VIEW) {
    var mediaTypeIndex1 = VolumeInfoListImpl.MEDIA_VIEW_ROOT_ORDER_.indexOf(
        VolumeManagerCommon.getMediaViewRootTypeFromVolumeId(
            volumeInfo1.volumeId));
    var mediaTypeIndex2 = VolumeInfoListImpl.MEDIA_VIEW_ROOT_ORDER_.indexOf(
        VolumeManagerCommon.getMediaViewRootTypeFromVolumeId(
            volumeInfo2.volumeId));
    if (mediaTypeIndex1 !== mediaTypeIndex2)
      return mediaTypeIndex1 < mediaTypeIndex2 ? -1 : 1;
    return 0;
  }
  if (volumeInfo1.volumeId !== volumeInfo2.volumeId)
    return volumeInfo1.volumeId < volumeInfo2.volumeId ? -1 : 1;
  return 0;
};

VolumeInfoListImpl.prototype = {
  get length() { return this.model_.length; }
};

/** @override */
VolumeInfoListImpl.prototype.addEventListener = function(type, handler) {
  this.model_.addEventListener(type, handler);
};

/** @override */
VolumeInfoListImpl.prototype.removeEventListener = function(type, handler) {
  this.model_.removeEventListener(type, handler);
};

/** @override */
VolumeInfoListImpl.prototype.add = function(volumeInfo) {
  var index = this.findIndex(volumeInfo.volumeId);
  if (index !== -1)
    this.model_.splice(index, 1, volumeInfo);
  else
    this.model_.push(volumeInfo);
};

/** @override */
VolumeInfoListImpl.prototype.remove = function(volumeId) {
  var index = this.findIndex(volumeId);
  if (index !== -1)
    this.model_.splice(index, 1);
};

/** @override */
VolumeInfoListImpl.prototype.findIndex = function(volumeId) {
  for (var i = 0; i < this.model_.length; i++) {
    if (this.model_.item(i).volumeId === volumeId)
      return i;
  }
  return -1;
};

/** @override */
VolumeInfoListImpl.prototype.findByEntry = function(entry) {
  for (var i = 0; i < this.length; i++) {
    var volumeInfo = this.item(i);
    if (volumeInfo.fileSystem &&
        util.isSameFileSystem(volumeInfo.fileSystem, entry.filesystem)) {
      return volumeInfo;
    }
    // Additionally, check fake entries.
    for (var key in volumeInfo.fakeEntries_) {
      var fakeEntry = volumeInfo.fakeEntries_[key];
      if (util.isSameEntry(fakeEntry, entry))
        return volumeInfo;
    }
  }
  return null;
};

/** @override */
VolumeInfoListImpl.prototype.findByDevicePath = function(devicePath) {
  for (var i = 0; i < this.length; i++) {
    var volumeInfo = this.item(i);
    if (volumeInfo.devicePath &&
        volumeInfo.devicePath == devicePath) {
      return volumeInfo;
    }
  }
  return null;
};

/**
 * Returns a VolumInfo for the volume ID, or null if not found.
 *
 * @param {string} volumeId
 * @return {VolumeInfo} The volume's information, or null if not found.
 */
VolumeInfoListImpl.prototype.findByVolumeId = function(volumeId) {
  var index = this.findIndex(volumeId);
  return (index !== -1) ?
      /** @type {VolumeInfo} */ (this.model_.item(index)) :
      null;
};

/** @override */
VolumeInfoListImpl.prototype.whenVolumeInfoReady = function(volumeId) {
  return new Promise(function(fulfill) {
    var handler = function() {
      var info = this.findByVolumeId(volumeId);
      if (info) {
        fulfill(info);
        this.model_.removeEventListener('splice', handler);
      }
    }.bind(this);
    this.model_.addEventListener('splice', handler);
    handler();
  }.bind(this));
};

/** @override */
VolumeInfoListImpl.prototype.item = function(index) {
  return /** @type {!VolumeInfo} */ (this.model_.item(index));
};
