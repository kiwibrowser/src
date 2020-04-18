// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This function takes an object |imageSpec| with the key |path| -
// corresponding to the internet URL to be translated - and optionally
// |width| and |height| which are the maximum dimensions to be used when
// converting the image.
function loadImageData(imageSpec, callbacks) {
  var path = imageSpec.path;
  var img = new Image();
  if (typeof callbacks.onerror === 'function') {
    img.onerror = function() {
      callbacks.onerror({ problem: 'could_not_load', path: path });
    };
  }
  img.onload = function() {
    var canvas = document.createElement('canvas');

    if (img.width <= 0 || img.height <= 0) {
      callbacks.onerror({ problem: 'image_size_invalid', path: path});
      return;
    }

    var scaleFactor = 1;
    if (imageSpec.width && imageSpec.width < img.width)
      scaleFactor = imageSpec.width / img.width;

    if (imageSpec.height && imageSpec.height < img.height) {
      var heightScale = imageSpec.height / img.height;
      if (heightScale < scaleFactor)
        scaleFactor = heightScale;
    }

    canvas.width = img.width * scaleFactor;
    canvas.height = img.height * scaleFactor;

    var canvas_context = canvas.getContext('2d');
    canvas_context.clearRect(0, 0, canvas.width, canvas.height);
    canvas_context.drawImage(img, 0, 0, canvas.width, canvas.height);
    try {
      var imageData = canvas_context.getImageData(
          0, 0, canvas.width, canvas.height);
      if (typeof callbacks.oncomplete === 'function') {
        callbacks.oncomplete(
            imageData.width, imageData.height, imageData.data.buffer);
      }
    } catch (e) {
      if (typeof callbacks.onerror === 'function') {
        callbacks.onerror({ problem: 'data_url_unavailable', path: path });
      }
    }
  }
  img.src = path;
}

function on_complete_index(index, err, loading, finished, callbacks) {
  return function(width, height, imageData) {
    delete loading[index];
    finished[index] = { width: width, height: height, data: imageData };
    if (err)
      callbacks.onerror(index);
    if ($Object.keys(loading).length == 0)
      callbacks.oncomplete(finished);
  }
}

function loadAllImages(imageSpecs, callbacks) {
  var loading = {}, finished = [],
      index, pathname;

  for (var index = 0; index < imageSpecs.length; index++) {
    loading[index] = imageSpecs[index];
    loadImageData(imageSpecs[index], {
      oncomplete: on_complete_index(index, false, loading, finished, callbacks),
      onerror: on_complete_index(index, true, loading, finished, callbacks)
    });
  }
}

exports.$set('loadImageData', loadImageData);
exports.$set('loadAllImages', loadAllImages);
