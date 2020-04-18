// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Runs a test to ensure slideshow traverses images automatically
 *
 * @param {string} testVolumeName Test volume name passed to the addEntries
 *     function. Either 'drive' or 'local'.
 * @param {VolumeManagerCommon.VolumeType} volumeType Volume type.
 * @return {Promise} Promise to be fulfilled on success.
 */
function slideshowTraversal(testVolumeName, volumeType) {
  // Setup
  var testEntries = [ENTRIES.desktop, ENTRIES.image3];
  var launchedPromise = launch(
      testVolumeName, volumeType, testEntries, testEntries.slice(0, 1));
  var appId;
  return launchedPromise.then(function(args) {
    appId = args.appId;
    return gallery.waitForElement(appId, '.gallery[mode="slide"]');
  }).then(function() {
    return gallery.waitForSlideImage(appId, 800, 600, 'My Desktop Background');
  }).then(function() {
    // Start slideshow.
    return gallery.waitAndClickElement(appId, '.slideshow.icon-button');
  }).then(function() {
    // Image will change automatically.
    return gallery.waitForSlideImage(appId, 640, 480, 'image3');
  }).then(function() {
    // Will rollover to the start again.
    return gallery.waitForSlideImage(appId, 800, 600, 'My Desktop Background');
  });
}

/**
 * Runs a test to ensure pausing slideshow stops images rolling
 *
 * @param {string} testVolumeName Test volume name passed to the addEntries
 *     function. Either 'drive' or 'local'.
 * @param {VolumeManagerCommon.VolumeType} volumeType Volume type.
 * @return {Promise} Promise to be fulfilled on success.
 */
function stopStartSlideshow(testVolumeName, volumeType) {
  var testEntries = [ENTRIES.desktop, ENTRIES.image3];
  var launchedPromise = launch(
      testVolumeName, volumeType, testEntries, testEntries.slice(0, 1));
  var appId;
  return launchedPromise.then(function(args) {
    appId = args.appId;
    return gallery.waitForElement(appId, '.gallery[mode="slide"]');
  }).then(function() {
    return gallery.waitForSlideImage(appId, 800, 600, 'My Desktop Background');
  }).then(function() {
    // Start slideshow.
    return gallery.waitAndClickElement(appId, '.slideshow.icon-button');
  }).then(function() {
    // Image will change automatically.
    return gallery.waitForSlideImage(appId, 640, 480, 'image3');
  }).then(function() {
    // Pause.
    return gallery.waitAndClickElement(appId, '.slideshow-play');
  }).then(function() {
    // Wait some time.
    return wait(3000);
  }).then(function() {
    // Check image is the same.
    return gallery.waitForSlideImage(appId, 640, 480, 'image3');
  }).then(function() {
    // Play.
    return gallery.waitAndClickElement(appId, '.slideshow-play');
  }).then(function() {
    // Check image changes.
    return gallery.waitForSlideImage(appId, 800, 600, 'My Desktop Background');
  });
}

/**
 * The slideshowTraversal test for Downloads.
 * @return {Promise} Promise to be fulfilled on success.
 */
testcase.slideshowTraversalOnDownloads = function() {
  return slideshowTraversal('local', 'downloads');
};

/**
 * The slideshowTraversal test for Drive.
 * @return {Promise} Promise to be fulfilled on success.
 */
testcase.slideshowTraversalOnDrive = function() {
 return slideshowTraversal('drive', 'drive');
};

/**
 * The stopStartSlideshow test for Downloads.
 * @return {Promise} Promise to be fulfilled on success.
 */
testcase.stopStartSlideshowOnDownloads = function() {
  return stopStartSlideshow('local', 'downloads');
};

/**
 * The stopStartSlideshow test for Drive.
 * @return {Promise} Promise to be fulfilled on success.
 */
testcase.stopStartSlideshowOnDrive = function() {
  return stopStartSlideshow('drive', 'drive');
};