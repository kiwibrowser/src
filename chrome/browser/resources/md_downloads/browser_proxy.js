// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('downloads', function() {
  class BrowserProxy {
    /** @param {string} id ID of the download to cancel. */
    cancel(id) {
      chrome.send('cancel', [id]);
    }

    /** @param {string} id ID of the dangerous download to discard. */
    discardDangerous(id) {
      chrome.send('discardDangerous', [id]);
    }

    /** @param {string} url URL of a file to download. */
    download(url) {
      const a = document.createElement('a');
      a.href = url;
      a.setAttribute('download', '');
      a.click();
    }

    /** @param {string} id ID of the download that the user started dragging. */
    drag(id) {
      chrome.send('drag', [id]);
    }

    /**
     * Loads more downloads with the current search terms.
     * @param {!Array<string>} searchTerms
     */
    getDownloads(searchTerms) {
      chrome.send('getDownloads', searchTerms);
    }

    /** Opens the current local destination for downloads. */
    openDownloadsFolder() {
      chrome.send('openDownloadsFolderRequiringGesture');
    }

    /**
     * @param {string} id ID of the download to run locally on the user's box.
     */
    openFile(id) {
      chrome.send('openFileRequiringGesture', [id]);
    }

    /** @param {string} id ID the of the progressing download to pause. */
    pause(id) {
      chrome.send('pause', [id]);
    }

    /** @param {string} id ID of the finished download to remove. */
    remove(id) {
      chrome.send('remove', [id]);
    }

    /** Instructs the browser to clear all finished downloads. */
    clearAll() {
      chrome.send('clearAll');
    }

    /** @param {string} id ID of the paused download to resume. */
    resume(id) {
      chrome.send('resume', [id]);
    }

    /**
     * @param {string} id ID of the dangerous download to save despite
     *     warnings.
     */
    saveDangerous(id) {
      chrome.send('saveDangerousRequiringGesture', [id]);
    }

    /**
     * Shows the local folder a finished download resides in.
     * @param {string} id ID of the download to show.
     */
    show(id) {
      chrome.send('show', [id]);
    }

    /** Undo download removal. */
    undo() {
      chrome.send('undo');
    }
  }

  cr.addSingletonGetter(BrowserProxy);

  return {BrowserProxy: BrowserProxy};
});
