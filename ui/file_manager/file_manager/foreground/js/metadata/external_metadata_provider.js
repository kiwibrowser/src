// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Metadata provider for FileEntry#getMetadata.
 * TODO(hirono): Rename thumbnailUrl with externalThumbnailUrl.
 *
 * @constructor
 * @extends {MetadataProvider}
 * @struct
 */
function ExternalMetadataProvider() {
  MetadataProvider.call(this, ExternalMetadataProvider.PROPERTY_NAMES);
}

/**
 * @const {!Array<string>}
 */
ExternalMetadataProvider.PROPERTY_NAMES = [
  'availableOffline', 'availableWhenMetered', 'contentMimeType',
  'croppedThumbnailUrl', 'customIconUrl', 'dirty', 'externalFileUrl', 'hosted',
  'imageHeight', 'imageRotation', 'imageWidth', 'modificationTime',
  'modificationByMeTime', 'pinned', 'present', 'shared', 'sharedWithMe', 'size',
  'thumbnailUrl'
];

ExternalMetadataProvider.prototype.__proto__ = MetadataProvider.prototype;

/**
 * @override
 */
ExternalMetadataProvider.prototype.get = function(requests) {
  if (!requests.length)
    return Promise.resolve([]);
  return new Promise(function(fulfill) {
    var entries = requests.map(function(request) {
      return request.entry;
    });
    var nameMap = {};
    for (var i = 0; i < requests.length; i++) {
      for (var j = 0; j < requests[i].names.length; j++) {
        nameMap[requests[i].names[j]] = true;
      }
    }
    chrome.fileManagerPrivate.getEntryProperties(
        entries,
        Object.keys(nameMap),
        function(results) {
          if (!chrome.runtime.lastError)
            fulfill(this.convertResults_(requests, nameMap, results));
          else
            fulfill(requests.map(function() { return new MetadataItem(); }));
        }.bind(this));
  }.bind(this));
};

/**
 * @param {!Array<!MetadataRequest>} requests
 * @param {!Object<boolean>} nameMap
 * @param {!Array<!EntryProperties>} propertiesList
 * @return {!Array<!MetadataItem>}
 */
ExternalMetadataProvider.prototype.convertResults_ =
    function(requests, nameMap, propertiesList) {
  var results = [];
  for (var i = 0; i < propertiesList.length; i++) {
    var prop = propertiesList[i];
    var item = new MetadataItem();
    if (prop.availableOffline !== undefined || nameMap['availableOffline'])
      item.availableOffline = prop.availableOffline;
    if (prop.availableWhenMetered !== undefined ||
        nameMap['availableWhenMetered'])
      item.availableWhenMetered = prop.availableWhenMetered;
    if (prop.contentMimeType !== undefined || nameMap['contentMimeType'])
      item.contentMimeType = prop.contentMimeType || '';
    if (prop.croppedThumbnailUrl !== undefined ||
        nameMap['croppedThumbnailUrl'])
      item.croppedThumbnailUrl = prop.croppedThumbnailUrl;
    if (prop.customIconUrl !== undefined || nameMap['customIconUrl'])
      item.customIconUrl = prop.customIconUrl || '';
    if (prop.dirty !== undefined || nameMap['dirty'])
      item.dirty = prop.dirty;
    if (prop.externalFileUrl !== undefined || nameMap['externalFileUrl'])
      item.externalFileUrl = prop.externalFileUrl;
    if (prop.hosted !== undefined || nameMap['hosted'])
      item.hosted = prop.hosted;
    if (prop.imageHeight !== undefined || nameMap['imageHeight'])
      item.imageHeight = prop.imageHeight;
    if (prop.imageRotation !== undefined || nameMap['imageRotation'])
      item.imageRotation = prop.imageRotation;
    if (prop.imageWidth !== undefined || nameMap['imageWidth'])
      item.imageWidth = prop.imageWidth;
    if (prop.modificationTime !== undefined || nameMap['modificationTime'])
      item.modificationTime = new Date(prop.modificationTime);
    if (prop.modificationByMeTime !== undefined ||
        nameMap['modificationByMeTime'])
      item.modificationByMeTime = new Date(prop.modificationByMeTime);
    if (prop.pinned !== undefined || nameMap['pinned'])
      item.pinned = prop.pinned;
    if (prop.present !== undefined || nameMap['present'])
      item.present = prop.present;
    if (prop.shared !== undefined || nameMap['shared'])
      item.shared = prop.shared;
    if (prop.sharedWithMe !== undefined || nameMap['sharedWithMe'])
      item.sharedWithMe = prop.sharedWithMe;
    if (prop.size !== undefined || nameMap['size'])
      item.size = requests[i].entry.isFile ? (prop.size || 0) : -1;
    if (prop.thumbnailUrl !== undefined || nameMap['thumbnailUrl'])
      item.thumbnailUrl = prop.thumbnailUrl;
    results.push(item);
  }
  return results;
};
