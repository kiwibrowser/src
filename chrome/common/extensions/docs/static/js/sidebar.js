// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Adds toggle controls to the sidebar list.
 *
 * Finds all elements marked as toggleable, and adds an onClick event to
 * collapse and expand the element's children.
 */
(function() {
  var sidebar = document.getElementById('gc-sidebar');
  if (!sidebar)
    return;

  Array.prototype.forEach.call(sidebar.querySelectorAll('[toggleable]'),
                               function(toggleable) {
    var button = toggleable.parentNode.querySelector('.button');
    var toggleIndicator = button.querySelector('.toggleIndicator');
    var isToggled = false;
    function toggle() {
      if (isToggled) {
        toggleable.classList.add('hidden');
        toggleIndicator.classList.remove('toggled');
      } else {
        toggleable.classList.remove('hidden');
        toggleIndicator.classList.add('toggled');
      }
      isToggled = !isToggled;
    }
    button.setAttribute('href', 'javascript:void(0)');
    button.addEventListener('click', toggle);
  });

})();
