// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function init() {
  var content = $('greeting');
  chrome.firstRunPrivate.getLocalizedStrings(function(strings) {
    loadTimeData.data = strings;
    i18nTemplate.process(document, loadTimeData);
    // Resizing and centering app's window.
    var bounds = {};
    bounds.width = content.offsetWidth;
    bounds.height = content.offsetHeight;
    bounds.left = Math.round(0.5 * (window.screen.availWidth - bounds.width));
    bounds.top = Math.round(0.5 * (window.screen.availHeight - bounds.height));
    appWindow.setBounds(bounds);
    appWindow.show();
  });
  var closeButton = content.getElementsByClassName('close-button')[0];
  // Make close unfocusable by mouse.
  closeButton.addEventListener('mousedown', function(e) {
    e.preventDefault();
  });
  closeButton.addEventListener('click', function(e) {
    appWindow.close();
    e.stopPropagation();
  });
  var tutorialButton = content.getElementsByClassName('next-button')[0];
  tutorialButton.addEventListener('click', function(e) {
    chrome.firstRunPrivate.launchTutorial();
    appWindow.close();
    e.stopPropagation();
  });

  // If spoken feedback is enabled, also show its tutorial.
  chrome.accessibilityFeatures.spokenFeedback.get({}, function(details) {
    if (details.value) {
      var chromeVoxId = 'mndnfokpggljbaajbnioimlmbfngpief';
      chrome.runtime.sendMessage(chromeVoxId, {openTutorial: true});
    }
  });
}

document.addEventListener('DOMContentLoaded', init);
