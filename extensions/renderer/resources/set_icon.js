// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var SetIconCommon = requireNative('setIcon').SetIconCommon;

function loadImagePath(path, callback) {
  var img = new Image();
  img.onerror = function() {
    console.error('Could not load action icon \'' + path + '\'.');
  };
  img.onload = function() {
    var canvas = document.createElement('canvas');
    canvas.width = img.width;
    canvas.height = img.height;

    var canvas_context = canvas.getContext('2d');
    canvas_context.clearRect(0, 0, canvas.width, canvas.height);
    canvas_context.drawImage(img, 0, 0, canvas.width, canvas.height);
    var imageData = canvas_context.getImageData(0, 0, canvas.width,
                                                canvas.height);
    callback(imageData);
  };
  img.src = path;
}

function smellsLikeImageData(imageData) {
  // See if this object at least looks like an ImageData element.
  // Unfortunately, we cannot use instanceof because the ImageData
  // constructor is not public.
  //
  // We do this manually instead of using JSONSchema to avoid having these
  // properties show up in the doc.
  return (typeof imageData == 'object') && ('width' in imageData) &&
         ('height' in imageData) && ('data' in imageData);
}

function verifyImageData(imageData) {
  if (!smellsLikeImageData(imageData)) {
    throw new Error(
        'The imageData property must contain an ImageData object or' +
        ' dictionary of ImageData objects.');
  }
}

/**
 * Normalizes |details| to a format suitable for sending to the browser,
 * for example converting ImageData to a binary representation.
 *
 * @param {ImageDetails} details
 *   The ImageDetails passed into an extension action-style API.
 * @param {Function} callback
 *   The callback function to pass processed imageData back to. Note that this
 *   callback may be called reentrantly.
 */
function setIcon(details, callback) {
  // Note that iconIndex is actually deprecated, and only available to the
  // pageAction API.
  // TODO(kalman): Investigate whether this is for the pageActions API, and if
  // so, delete it.
  if ('iconIndex' in details) {
    callback(details);
    return;
  }

  if ('imageData' in details) {
    if (smellsLikeImageData(details.imageData)) {
      var imageData = details.imageData;
      details.imageData = {};
      details.imageData[imageData.width.toString()] = imageData;
    } else if (typeof details.imageData == 'object' &&
               Object.getOwnPropertyNames(details.imageData).length !== 0) {
      for (var sizeKey in details.imageData) {
        verifyImageData(details.imageData[sizeKey]);
      }
    } else {
      verifyImageData(false);
    }

    callback(SetIconCommon(details));
    return;
  }

  if ('path' in details) {
    if (typeof details.path == 'object') {
      details.imageData = {};
      var detailKeyCount = 0;
      for (var iconSize in details.path) {
        ++detailKeyCount;
        loadImagePath(details.path[iconSize], function(size, imageData) {
          details.imageData[size] = imageData;
          if (--detailKeyCount == 0)
            callback(SetIconCommon(details));
        }.bind(null, iconSize));
      }
      if (detailKeyCount == 0)
        throw new Error('The path property must not be empty.');
    } else if (typeof details.path == 'string') {
      details.imageData = {};
      loadImagePath(details.path, function(imageData) {
        details.imageData[imageData.width.toString()] = imageData;
        delete details.path;
        callback(SetIconCommon(details));
      });
    }
    return;
  }
  throw new Error('Either the path or imageData property must be specified.');
}

exports.$set('setIcon', setIcon);
