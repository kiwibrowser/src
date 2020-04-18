// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

(function() {

/**
 * Obtains track text.
 * @param {string} audioAppId Window ID.
 * @param {query} query Query for the track.
 * @return {Promise} Promise to be fulfilled with {title:string, artist:string}
 *     object.
 */
function getTrackText(audioAppId, query) {
  var titleElements = audioPlayerApp.callRemoteTestUtil(
      'queryAllElements',
      audioAppId,
      [query + ' > .data > .data-title']);
  var artistElements = audioPlayerApp.callRemoteTestUtil(
      'queryAllElements',
      audioAppId,
      [query + ' > .data > .data-artist']);
  return Promise.all([titleElements, artistElements]).then(function(data) {
    return {
      title: data[0][0] && data[0][0].text,
      artist: data[1][0] && data[1][0].text
    };
  });
}

/**
 * Tests if the audio player shows up for the selected image and that the audio
 * is loaded successfully.
 *
 * @param {string} path Directory path to be tested.
 */
function audioOpen(path) {
  var appId;
  var audioAppId;

  var expectedFilesBefore =
      TestEntryInfo.getExpectedRows(path == RootPath.DRIVE ?
          BASIC_DRIVE_ENTRY_SET : BASIC_LOCAL_ENTRY_SET).sort();
  var expectedFilesAfter =
      expectedFilesBefore.concat([ENTRIES.newlyAdded.getExpectedRow()]).sort();

  var caller = getCaller();
  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Select the song.
    function(results) {
      appId = results.windowId;

      // Add an additional audio file.
      addEntries(['local', 'drive'], [ENTRIES.newlyAdded], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFileListChange(appId, expectedFilesBefore.length).
          then(this.next);
    },
    function(actualFilesAfter) {
      chrome.test.assertEq(expectedFilesAfter, actualFilesAfter);
      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['Beautiful Song.ogg'], this.next);
    },
    // Wait for the audio player window.
    function(result) {
      chrome.test.assertTrue(result);
      audioPlayerApp.waitForWindow('audio_player.html').then(this.next);
    },
    // Wait for the changes of the player status.
    function(inAppId) {
      audioAppId = inAppId;
      audioPlayerApp.waitForElement(audioAppId, 'audio-player[playing]').
          then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/Beautiful%20Song.ogg',
          element.attributes.currenttrackurl);
      var query1 = 'audio-player /deep/ .track[index="0"][active]';
      var query2 = 'audio-player /deep/ .track[index="1"]:not([active])';
      repeatUntil(function() {
        var trackText1 = getTrackText(audioAppId, query1);
        var trackText2 = getTrackText(audioAppId, query2);
        return Promise.all([trackText1, trackText2]).then(function(tracks) {
          var expected = [
            {title: 'Beautiful Song', artist: 'Unknown Artist'},
            {title: 'newly added file', artist: 'Unknown Artist'}
          ];
          if (!chrome.test.checkDeepEq(expected, tracks)) {
            return pending(
                caller, 'Tracks are expected as: %j, but is %j.', expected,
                tracks);
          }
        });
      }).then(this.next, function(e) { chrome.test.fail(e); });
    },
    // Open another file.
    function() {
      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['newly added file.ogg'], this.next);
    },
    // Wait for the changes of the player status.
    function(result) {
      chrome.test.assertTrue(result, 'Fail to open the 2nd file');
      var query = 'audio-player' +
                  '[playing]' +
                  '[currenttrackurl$="newly%20added%20file.ogg"]';
      audioPlayerApp.waitForElement(audioAppId, query).then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/newly%20added%20file.ogg',
          element.attributes.currenttrackurl);
      var query1 = 'audio-player /deep/ .track[index="0"]:not([active])';
      var query2 = 'audio-player /deep/ .track[index="1"][active]';
      repeatUntil(function() {
        var trackText1 = getTrackText(audioAppId, query1);
        var trackText2 = getTrackText(audioAppId, query2);
        return Promise.all([trackText1, trackText2]).then(function(tracks) {
          var expected = [
            {title: 'Beautiful Song', artist: 'Unknown Artist'},
            {title: 'newly added file', artist: 'Unknown Artist'}
          ];
          if (!chrome.test.checkDeepEq(expected, tracks)) {
            return pending(
                caller, 'Tracks are expected as: %j, but is %j.', expected,
                tracks);
          }
        });
      }).then(this.next, function(e) { chrome.test.fail(e); });
    },
    // Wait for the changes of the player status.
    function() {
      // Close window
      audioPlayerApp.closeWindowAndWait(audioAppId).then(this.next);
    },
    // Wait for the audio player.
    function(result) {
      chrome.test.assertTrue(result, 'Fail to close the window');
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests if the audio player play the next file after the current file.
 *
 * @param {string} path Directory path to be tested.
 */
function audioAutoAdvance(path) {
  var appId;
  var audioAppId;

  var expectedFilesBefore =
      TestEntryInfo.getExpectedRows(path == RootPath.DRIVE ?
          BASIC_DRIVE_ENTRY_SET : BASIC_LOCAL_ENTRY_SET).sort();
  var expectedFilesAfter =
      expectedFilesBefore.concat([ENTRIES.newlyAdded.getExpectedRow()]).sort();

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Select the song.
    function(results) {
      appId = results.windowId;

      // Add an additional audio file.
      addEntries(['local', 'drive'], [ENTRIES.newlyAdded], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFileListChange(appId, expectedFilesBefore.length).
          then(this.next);
    },
    function(actualFilesAfter) {
      chrome.test.assertEq(expectedFilesAfter, actualFilesAfter);
      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['Beautiful Song.ogg'], this.next);
    },
    // Wait for the audio player window.
    function(result) {
      chrome.test.assertTrue(result);
      audioPlayerApp.waitForWindow('audio_player.html').then(this.next);
    },
    // Wait for the changes of the player status.
    function(inAppId) {
      audioAppId = inAppId;
      audioPlayerApp.waitForElement(audioAppId, 'audio-player[playing]').
          then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/Beautiful%20Song.ogg',
          element.attributes.currenttrackurl);

      // Wait for next song.
      var query = 'audio-player' +
                  '[playing]' +
                  '[currenttrackurl$="newly%20added%20file.ogg"]';
      audioPlayerApp.waitForElement(audioAppId, query).then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/newly%20added%20file.ogg',
          element.attributes.currenttrackurl);

      // Close window
      audioPlayerApp.closeWindowAndWait(audioAppId).then(this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests if the audio player play the next file after the current file.
 *
 * @param {string} path Directory path to be tested.
 */
function audioRepeatAllModeSingleFile(path) {
  var appId;
  var audioAppId;

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Select the song.
    function(results) {
      appId = results.windowId;

      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['Beautiful Song.ogg'], this.next);
    },
    // Wait for the audio player window.
    function(result) {
      chrome.test.assertTrue(result);
      audioPlayerApp.waitForWindow('audio_player.html').then(this.next);
    },
    // Wait for the changes of the player status.
    function(inAppId) {
      audioAppId = inAppId;
      audioPlayerApp.waitForElement(audioAppId, 'audio-player[playing]').
          then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/Beautiful%20Song.ogg',
          element.attributes.currenttrackurl);

      audioPlayerApp.callRemoteTestUtil(
          'fakeMouseClick',
          audioAppId,
          ['audio-player /deep/ repeat-button .no-repeat'],
          this.next);
    },
    function(result) {
      chrome.test.assertTrue(result, 'Failed to click the repeat button');

      var selector = 'audio-player[playing][playcount="1"]';
      audioPlayerApp.waitForElement(audioAppId, selector).then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/Beautiful%20Song.ogg',
          element.attributes.currenttrackurl);

      // Close window
      audioPlayerApp.closeWindowAndWait(audioAppId).then(this.next);
    },
    // Wait for the audio player.
    function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests if the audio player play the next file after the current file.
 *
 * @param {string} path Directory path to be tested.
 */
function audioNoRepeatModeSingleFile(path) {
  var appId;
  var audioAppId;

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Select the song.
    function(results) {
      appId = results.windowId;

      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['Beautiful Song.ogg'], this.next);
    },
    // Wait for the audio player window.
    function(result) {
      chrome.test.assertTrue(result);
      audioPlayerApp.waitForWindow('audio_player.html').then(this.next);
    },
    // Wait for the changes of the player status.
    function(inAppId) {
      audioAppId = inAppId;
      audioPlayerApp.waitForElement(audioAppId, 'audio-player[playing]').
          then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/Beautiful%20Song.ogg',
          element.attributes.currenttrackurl);

      var selector = 'audio-player[playcount="1"]:not([playing])';
      audioPlayerApp.waitForElement(audioAppId, selector).then(this.next);
    },
    // Get the source file name.
    function(element) {
      // Close window
      audioPlayerApp.closeWindowAndWait(audioAppId).then(this.next);
    },
    // Wait for the audio player.
    function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests if the audio player play the next file after the current file.
 *
 * @param {string} path Directory path to be tested.
 */
function audioRepeatOneModeSingleFile(path) {
  var appId;
  var audioAppId;

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Select the song.
    function(results) {
      appId = results.windowId;

      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['Beautiful Song.ogg'], this.next);
    },
    // Wait for the audio player window.
    function(result) {
      chrome.test.assertTrue(result);
      audioPlayerApp.waitForWindow('audio_player.html').then(this.next);
    },
    // Wait for the changes of the player status.
    function(inAppId) {
      audioAppId = inAppId;
      audioPlayerApp.waitForElement(audioAppId, 'audio-player[playing]').
          then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/Beautiful%20Song.ogg',
          element.attributes.currenttrackurl);

      audioPlayerApp.callRemoteTestUtil(
          'fakeMouseClick',
          audioAppId,
          ['audio-player /deep/ repeat-button .no-repeat'],
          this.next);
    },
    function() {
      audioPlayerApp.callRemoteTestUtil(
          'fakeMouseClick',
          audioAppId,
          ['audio-player /deep/ repeat-button .repeat-all'],
          this.next);
    },
    function(result) {
      chrome.test.assertTrue(result, 'Failed to click the repeat button');

      var selector = 'audio-player[playing][playcount="1"]';
      audioPlayerApp.waitForElement(audioAppId, selector).then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/Beautiful%20Song.ogg',
          element.attributes.currenttrackurl);

      // Close window
      audioPlayerApp.closeWindowAndWait(audioAppId).then(this.next);
    },
    // Wait for the audio player.
    function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests if the audio player play the next file after the current file.
 *
 * @param {string} path Directory path to be tested.
 */
function audioRepeatAllModeMultipleFile(path) {
  var appId;
  var audioAppId;

  var expectedFilesBefore =
      TestEntryInfo.getExpectedRows(path == RootPath.DRIVE ?
          BASIC_DRIVE_ENTRY_SET : BASIC_LOCAL_ENTRY_SET);
  var expectedFilesAfter =
      expectedFilesBefore.concat([ENTRIES.newlyAdded.getExpectedRow()]);

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Select the song.
    function(results) {
      appId = results.windowId;

      // Add an additional audio file.
      addEntries(['local', 'drive'], [ENTRIES.newlyAdded], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, expectedFilesAfter).then(this.next);
    },
    function(/* no result */) {
      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['newly added file.ogg'], this.next);
    },
    // Wait for the audio player window.
    function(result) {
      chrome.test.assertTrue(result);
      audioPlayerApp.waitForWindow('audio_player.html').then(this.next);
    },
    // Wait for the changes of the player status.
    function(inAppId) {
      audioAppId = inAppId;
      audioPlayerApp.waitForElement(audioAppId, 'audio-player[playing]').
          then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/newly%20added%20file.ogg',
          element.attributes.currenttrackurl);

      audioPlayerApp.callRemoteTestUtil(
          'fakeMouseClick',
          audioAppId,
          ['audio-player /deep/ repeat-button .no-repeat'],
          this.next);
    },
    function(result) {
      chrome.test.assertTrue(result, 'Failed to click the repeat button');

      // Wait for next song.
      var query = 'audio-player' +
                  '[playing]' +
                  '[currenttrackurl$="Beautiful%20Song.ogg"]';
      audioPlayerApp.waitForElement(audioAppId, query).then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/Beautiful%20Song.ogg',
          element.attributes.currenttrackurl);

      // Close window
      audioPlayerApp.closeWindowAndWait(audioAppId).then(this.next);
    },
    // Wait for the audio player.
    function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests if the audio player play the next file after the current file.
 *
 * @param {string} path Directory path to be tested.
 */
function audioNoRepeatModeMultipleFile(path) {
  var appId;
  var audioAppId;

  var expectedFilesBefore =
      TestEntryInfo.getExpectedRows(path == RootPath.DRIVE ?
          BASIC_DRIVE_ENTRY_SET : BASIC_LOCAL_ENTRY_SET);
  var expectedFilesAfter =
      expectedFilesBefore.concat([ENTRIES.newlyAdded.getExpectedRow()]);

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Select the song.
    function(results) {
      appId = results.windowId;

      // Add an additional audio file.
      addEntries(['local', 'drive'], [ENTRIES.newlyAdded], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, expectedFilesAfter).then(this.next);
    },
    function(/* no result */) {
      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['newly added file.ogg'], this.next);
    },
    // Wait for the audio player window.
    function(result) {
      chrome.test.assertTrue(result);
      audioPlayerApp.waitForWindow('audio_player.html').then(this.next);
    },
    // Wait for the changes of the player status.
    function(inAppId) {
      audioAppId = inAppId;
      audioPlayerApp.waitForElement(audioAppId, 'audio-player[playing]').
          then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/newly%20added%20file.ogg',
          element.attributes.currenttrackurl);

      // Wait for next song.
      var query = 'audio-player:not([playing])';
      audioPlayerApp.waitForElement(audioAppId, query).then(this.next);
    },
    // Get the source file name.
    function(element) {
      // Close window
      audioPlayerApp.closeWindowAndWait(audioAppId).then(this.next);
    },
    // Wait for the audio player.
    function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

/**
 * Tests if the audio player play the next file after the current file.
 *
 * @param {string} path Directory path to be tested.
 */
function audioRepeatOneModeMultipleFile(path) {
  var appId;
  var audioAppId;

  var expectedFilesBefore =
      TestEntryInfo.getExpectedRows(path == RootPath.DRIVE ?
          BASIC_DRIVE_ENTRY_SET : BASIC_LOCAL_ENTRY_SET);
  var expectedFilesAfter =
      expectedFilesBefore.concat([ENTRIES.newlyAdded.getExpectedRow()]);

  StepsRunner.run([
    function() {
      setupAndWaitUntilReady(null, path, this.next);
    },
    // Select the song.
    function(results) {
      appId = results.windowId;

      // Add an additional audio file.
      addEntries(['local', 'drive'], [ENTRIES.newlyAdded], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.waitForFiles(appId, expectedFilesAfter).then(this.next);
    },
    function(/* no result */) {
      remoteCall.callRemoteTestUtil(
          'openFile', appId, ['newly added file.ogg'], this.next);
    },
    // Wait for the audio player window.
    function(result) {
      chrome.test.assertTrue(result);
      audioPlayerApp.waitForWindow('audio_player.html').then(this.next);
    },
    // Wait for the changes of the player status.
    function(inAppId) {
      audioAppId = inAppId;
      audioPlayerApp.waitForElement(audioAppId, 'audio-player[playing]').
          then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/newly%20added%20file.ogg',
          element.attributes.currenttrackurl);

      audioPlayerApp.callRemoteTestUtil(
          'fakeMouseClick',
          audioAppId,
          ['audio-player /deep/ repeat-button .no-repeat'],
          this.next);
    },
    function() {
      audioPlayerApp.callRemoteTestUtil(
          'fakeMouseClick',
          audioAppId,
          ['audio-player /deep/ repeat-button .repeat-all'],
          this.next);
    },
    function(result) {
      chrome.test.assertTrue(result, 'Failed to click the repeat button');

      var selector = 'audio-player[playing][playcount="1"]';
      audioPlayerApp.waitForElement(audioAppId, selector).then(this.next);
    },
    // Get the source file name.
    function(element) {
      chrome.test.assertEq(
          'filesystem:chrome-extension://' + AUDIO_PLAYER_APP_ID + '/' +
              'external' + path + '/newly%20added%20file.ogg',
          element.attributes.currenttrackurl);

      // Close window
      audioPlayerApp.closeWindowAndWait(audioAppId).then(this.next);
    },
    // Wait for the audio player.
    function(result) {
      chrome.test.assertTrue(result);
      checkIfNoErrorsOccured(this.next);
    }
  ]);
}

testcase.audioOpenDownloads = function() {
  audioOpen(RootPath.DOWNLOADS);
};

testcase.audioOpenDrive = function() {
  audioOpen(RootPath.DRIVE);
};

testcase.audioAutoAdvanceDrive = function() {
  audioAutoAdvance(RootPath.DRIVE);
};

testcase.audioRepeatAllModeSingleFileDrive = function() {
  audioRepeatAllModeSingleFile(RootPath.DRIVE);
};

testcase.audioNoRepeatModeSingleFileDrive = function() {
  audioNoRepeatModeSingleFile(RootPath.DRIVE);
};

testcase.audioRepeatOneModeSingleFileDrive = function() {
  audioRepeatOneModeSingleFile(RootPath.DRIVE);
};

testcase.audioRepeatAllModeMultipleFileDrive = function() {
  audioRepeatAllModeMultipleFile(RootPath.DRIVE);
};

testcase.audioNoRepeatModeMultipleFileDrive = function() {
  audioNoRepeatModeMultipleFile(RootPath.DRIVE);
};

testcase.audioRepeatOneModeMultipleFileDrive = function() {
  audioRepeatOneModeMultipleFile(RootPath.DRIVE);
};

})();
