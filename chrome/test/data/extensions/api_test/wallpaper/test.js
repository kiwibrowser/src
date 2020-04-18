// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var pass = chrome.test.callbackPass;
var fail = chrome.test.callbackFail;

chrome.test.getConfig(function(config) {

  var baseURL = "http://a.com:" + config.testServer.port +
      "/extensions/api_test/wallpaper/";

  /*
   * Calls chrome.wallpaper.setWallpaper using an arraybuffer.
   * @param {string} filePath An extension relative file path.
   */
  var testSetWallpaperFromArrayBuffer = function (filePath) {
    // Loads an extension local file to an arraybuffer.
    var url = chrome.runtime.getURL(filePath);
    var wallpaperRequest = new XMLHttpRequest();
    wallpaperRequest.open('GET', url, true);
    wallpaperRequest.responseType = 'arraybuffer';
    try {
      wallpaperRequest.send(null);
      wallpaperRequest.onloadend = function(e) {
        if (wallpaperRequest.status === 200) {
          chrome.wallpaper.setWallpaper(
              {'data': wallpaperRequest.response,
               'layout': 'CENTER_CROPPED',
               'filename': 'test'},
              // Set wallpaper directly with an arraybuffer should pass.
              pass()
          );
        } else {
          chrome.test.fail('Failed to load local file: ' + filePath + '.');
        }
      };
    } catch (e) {
      console.error(e);
      chrome.test.fail('An error thrown when requesting wallpaper.');
    }
  };

  /*
   * Calls chrome.wallpaper.setWallpaper using an URL.
   * @param {string} relativeURL The relative URL of an online image.
   * @param {boolean} success True if expecting the API call success.
   * @param {string=} optExpectedError The expected error string if API call
   *     failed. An error string must be provided if success is set to false.
   */
  var testSetWallpaperFromURL = function (relativeURL,
                                          success,
                                          optExpectedError) {
    var url = baseURL + relativeURL;
    if (success) {
      chrome.wallpaper.setWallpaper(
          {'url': url,
           'layout': 'CENTER_CROPPED',
           'filename': 'test'},
           // A valid url should set wallpaper correctly.
           pass()
      );
    } else {
      if (optExpectedError == undefined) {
        chrome.test.fail('No expected error string is provided.');
        return;
      }
      chrome.wallpaper.setWallpaper(
         {'url': url,
          'layout': 'CENTER_CROPPED',
          'filename': 'test'},
          // Expect a failure.
          fail(optExpectedError));
    }
  };

  var testSetWallpaperThumbnail = function(relativeURL) {
    var url = baseURL + relativeURL;
    chrome.wallpaper.setWallpaper(
      { 'url': url,
        'layout': 'CENTER_CROPPED',
        'filename': 'test',
        'thumbnail': true
      }, pass(function(thumbnail) {
        var buffer = new Uint8Array(thumbnail);
        chrome.test.assertTrue(buffer.length > 0);
      }));
  };

  chrome.test.runTests([
    function setJpgWallpaperFromAppLocalFile() {
      testSetWallpaperFromArrayBuffer('test.jpg');
    },
    function setPngWallpaperFromAppLocalFile() {
      testSetWallpaperFromArrayBuffer('test.png');
    },
    function setJpgWallpaperFromURL () {
      testSetWallpaperFromURL('test.jpg', true);
    },
    function setPngWallpaperFromURL () {
      testSetWallpaperFromURL('test.png', true);
    },
    function setNoExistingWallpaperFromURL () {
      // test1.jpg doesn't exist. Expect a 404 error.
      var expectError =
          'Downloading wallpaper test1.jpg failed. The response code is 404.';
      testSetWallpaperFromURL('test1.jpg',
                              false,
                              expectError);
    },
    function newRequestCancelPreviousRequest() {
      // The first request should be canceled. The wallpaper in the first
      // request is chosen from one of the high-resolution built-in wallpapers
      // to make sure the first setWallpaper request hasn't finished yet when
      // the second request sends out.
      testSetWallpaperFromURL('test_image_high_resolution.jpg',
                              false,
                              'Set wallpaper was canceled.');
      testSetWallpaperFromURL('test.jpg', true);
    },
    function getThumbnailAferSetWallpaper() {
      testSetWallpaperThumbnail('test.jpg');
    }
  ]);
});
