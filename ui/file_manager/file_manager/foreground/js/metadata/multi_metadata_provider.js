// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!FileSystemMetadataProvider} fileSystemMetadataProvider
 * @param {!ExternalMetadataProvider} externalMetadataProvider
 * @param {!ContentMetadataProvider} contentMetadataProvider
 * @param {!VolumeManagerCommon.VolumeInfoProvider} volumeManager
 * @constructor
 * @extends {MetadataProvider}
 * @struct
 */
function MultiMetadataProvider(
    fileSystemMetadataProvider,
    externalMetadataProvider,
    contentMetadataProvider,
    volumeManager) {
  MetadataProvider.call(
      this,
      FileSystemMetadataProvider.PROPERTY_NAMES.concat(
          ExternalMetadataProvider.PROPERTY_NAMES).concat(
              ContentMetadataProvider.PROPERTY_NAMES));

  /**
   * @private {!FileSystemMetadataProvider}
   * @const
   */
  this.fileSystemMetadataProvider_ = fileSystemMetadataProvider;

  /**
   * @private {!ExternalMetadataProvider}
   * @const
   */
  this.externalMetadataProvider_ = externalMetadataProvider;

  /**
   * @private {!ContentMetadataProvider}
   * @const
   */
  this.contentMetadataProvider_ = contentMetadataProvider;

  /**
   * @private {!VolumeManagerCommon.VolumeInfoProvider}
   * @const
   */
  this.volumeManager_ = volumeManager;
}

MultiMetadataProvider.prototype.__proto__ = MetadataProvider.prototype;

/**
 * Obtains metadata for entries.
 * @param {!Array<!MetadataRequest>} requests
 * @return {!Promise<!Array<!MetadataItem>>}
 */
MultiMetadataProvider.prototype.get = function(requests) {
  var fileSystemRequests = [];
  var externalRequests = [];
  var contentRequests = [];
  var fallbackContentRequests = [];
  requests.forEach(function(request) {
    // Group property names.
    var fileSystemPropertyNames = [];
    var externalPropertyNames = [];
    var contentPropertyNames = [];
    var fallbackContentPropertyNames = [];
    for (var i = 0; i < request.names.length; i++) {
      var name = request.names[i];
      var isFileSystemProperty =
          FileSystemMetadataProvider.PROPERTY_NAMES.indexOf(name) !== -1;
      var isExternalProperty =
          ExternalMetadataProvider.PROPERTY_NAMES.indexOf(name) !== -1;
      var isContentProperty =
          ContentMetadataProvider.PROPERTY_NAMES.indexOf(name) !== -1;
      assert(isFileSystemProperty || isExternalProperty || isContentProperty);
      assert(!(isFileSystemProperty && isContentProperty));
      // If the property can be obtained both from ExternalProvider and from
      // ContentProvider, we can obtain the property from ExternalProvider
      // without fetching file content. On the other hand, the values from
      // ExternalProvider may be out of sync if the file is 'dirty'. Thus we
      // fallback to ContentProvider if the file is dirty. See below.
      if (isExternalProperty && isContentProperty) {
        externalPropertyNames.push(name);
        fallbackContentPropertyNames.push(name);
        continue;
      }
      if (isFileSystemProperty)
        fileSystemPropertyNames.push(name);
      if (isExternalProperty)
        externalPropertyNames.push(name);
      if (isContentProperty)
        contentPropertyNames.push(name);
    }
    var volumeInfo = this.volumeManager_.getVolumeInfo(request.entry);
    var addRequests = function(list, names) {
      if (names.length)
        list.push(new MetadataRequest(request.entry, names));
    };
    if (volumeInfo &&
        (volumeInfo.volumeType === VolumeManagerCommon.VolumeType.DRIVE ||
         volumeInfo.volumeType === VolumeManagerCommon.VolumeType.PROVIDED)) {
      // Because properties can be out of sync just after sync completion
      // even if 'dirty' is false, it refers 'present' here to switch the
      // content and the external providers.
      if (fallbackContentPropertyNames.length &&
          externalPropertyNames.indexOf('present') === -1) {
        externalPropertyNames.push('present');
      }
      addRequests(externalRequests, externalPropertyNames);
      addRequests(contentRequests, contentPropertyNames);
      addRequests(fallbackContentRequests, fallbackContentPropertyNames);
    } else {
      addRequests(fileSystemRequests, fileSystemPropertyNames);
      addRequests(
          contentRequests,
          contentPropertyNames.concat(fallbackContentPropertyNames));
    }
  }.bind(this));

  var get = function(provider, inRequests) {
    return provider.get(inRequests).then(function(results) {
      return {
        requests: inRequests,
        results: results
      };
    });
  };
  var fileSystemPromise = get(
      this.fileSystemMetadataProvider_, fileSystemRequests);
  var externalPromise = get(this.externalMetadataProvider_, externalRequests);
  var contentPromise = get(this.contentMetadataProvider_, contentRequests);
  var fallbackContentPromise = externalPromise.then(
      function(requestsAndResults) {
        var requests = requestsAndResults.requests;
        var results = requestsAndResults.results;
        var dirtyMap = [];
        for (var i = 0; i < results.length; i++) {
          dirtyMap[requests[i].entry.toURL()] = results[i].present;
        }
        return get(
            this.contentMetadataProvider_,
            fallbackContentRequests.filter(
                function(request) {
                  return dirtyMap[request.entry.toURL()];
                }));
      }.bind(this));

  // Merge results.
  return Promise.all([
    fileSystemPromise,
    externalPromise,
    contentPromise,
    fallbackContentPromise
  ]).then(function(resultsList) {
    var integratedResults = {};
    for (var i = 0; i < resultsList.length; i++) {
      var inRequests = resultsList[i].requests;
      var results = resultsList[i].results;
      assert(inRequests.length === results.length);
      for (var j = 0; j < results.length; j++) {
        var url = inRequests[j].entry.toURL();
        integratedResults[url] = integratedResults[url] || new MetadataItem();
        for (var name in results[j]) {
          integratedResults[url][name] = results[j][name];
        }
      }
    }
    return requests.map(function(request) {
      return integratedResults[request.entry.toURL()] || new MetadataItem();
    });
  });
};
