// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Intializes the platform chooser, the widget in the top left corner with the
// 'Apps...Extensions' dropdown.
(function() {

var platformChooser = document.getElementById('platform-chooser-popup');
Array.prototype.forEach.call(platformChooser.getElementsByTagName('button'),
                             function(button) {
  button.addEventListener('click', function(event) {
    window.location.assign(button.getAttribute('data-href'));
    event.stopPropagation();
  });
});

})()
