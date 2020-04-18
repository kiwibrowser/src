// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

// A simple popup manager.
var activePopup = null;

function init() {
  // Set up the buttons to toggle the popup.
  Array.prototype.forEach.call(document.getElementsByTagName('button'),
                               function(button) {
    var popupId = button.getAttribute('data-menu');
    if (popupId == null)
      return;
    var popup = document.getElementById(popupId);
    if (popup == null)
      throw new Error('No element with id "' + popupId + '" for popup');
    button.addEventListener('click', function(event) {
      toggle(popup);
      event.stopPropagation();
    });
  });
  // Make clicking anywhere else or pressing escape on the page hide the popup.
  document.body.addEventListener('click', function() {
    hideActive();
  });
  document.body.addEventListener('keydown', function(event) {
    if (event.keyCode == 27)
      hideActive();
  });
}

function toggle(popup) {
  if (hideActive() == popup)
    return;
  popup.style.display = 'block';
  activePopup = popup;
}

function hideActive() {
  if (activePopup == null)
    return;
  activePopup.style.display = ''
  var wasActive = activePopup;
  activePopup = null;
  return wasActive;
}

init();

}());
