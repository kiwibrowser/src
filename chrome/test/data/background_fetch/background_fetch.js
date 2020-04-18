// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Background Fetch Id to use when its value is not significant.
const kBackgroundFetchId = 'bg-fetch-id';
const kBackgroundFetchResource = [ '/background_fetch/types_of_cheese.txt' ];
const kIcon = [
  {
    src: '/notifications/icon.png',
    sizes: '100x100',
    type: 'image/png'
  }
];

function RegisterServiceWorker() {
  navigator.serviceWorker.register('sw.js').then(() => {
    sendResultToTest('ok - service worker registered');
  }).catch(sendErrorToTest);
}

// Starts a Background Fetch request for a single to-be-downloaded file.
function StartSingleFileDownload() {
  navigator.serviceWorker.ready.then(swRegistration => {
    const options = {
      icons: kIcon,
      title: 'Single-file Background Fetch'
    };

    return swRegistration.backgroundFetch.fetch(
        kBackgroundFetchId, kBackgroundFetchResource, options);
  }).then(bgFetchRegistration => {
    sendResultToTest('ok');
  }).catch(sendErrorToTest);
}

// Starts a Background Fetch request for a single to-be-downloaded file, with
// downloadTotal greater than the actual size.
function StartSingleFileDownloadWithBiggerThanActualDownloadTotal() {
  navigator.serviceWorker.ready.then(swRegistration => {
    const options = {
      icons: kIcon,
      title: 'Single-file Background Fetch with downloadTotal too high',
      downloadTotal: 1000,
    };

    return swRegistration.backgroundFetch.fetch(
        kBackgroundFetchId, kBackgroundFetchResource, options);
  }).then(bgFetchRegistration => {
    sendResultToTest('ok');
  }).catch(sendErrorToTest);
}

// Starts a Background Fetch request for a single to-be-downloaded file, with
// downloadTotal smaller than the actual size.
function StartSingleFileDownloadWithSmallerThanActualDownloadTotal() {
  navigator.serviceWorker.ready.then(swRegistration => {
    const options = {
      icons: kIcon,
      title: 'Single-file Background Fetch with downloadTotal too low',
      downloadTotal: 80,
    };

    return swRegistration.backgroundFetch.fetch(
        kBackgroundFetchId, kBackgroundFetchResource, options);
  }).then(bgFetchRegistration => {
    sendResultToTest('ok');
  }).catch(sendErrorToTest);
}

// Starts a Background Fetch request for a single to-be-downloaded file, with
// downloadTotal equal to the actual size (in bytes).
function StartSingleFileDownloadWithCorrectDownloadTotal() {
  navigator.serviceWorker.ready.then(swRegistration => {
    const options = {
      icons: kIcon,
      title: 'Single-file Background Fetch with accurate downloadTotal',
      downloadTotal: 82,
    };

    return swRegistration.backgroundFetch.fetch(
        kBackgroundFetchId, kBackgroundFetchResource, options);
  }).then(bgFetchRegistration => {
    sendResultToTest('ok');
  }).catch(sendErrorToTest);
}
