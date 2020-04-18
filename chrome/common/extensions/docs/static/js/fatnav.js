// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Adds toggle controls to the fat navbar.
 */

(function() {
var isTouch = (('ontouchstart' in window) || (navigator.msMaxTouchPoints > 0));
var isLargerThanPhoneQuery = window.matchMedia('screen and (min-width: 581px)');

var fatNav = document.querySelector('#fatnav');
var search = document.querySelector('#search');
var mobileNavCollasper = document.querySelector('#topnav .collase-icon');

var catLinks = fatNav.querySelectorAll('.category > a');
var catPath = findCategoryPath();

function hideActive(parentNode) {
  //parentNode.classList.remove('active');

  [].forEach.call(parentNode.querySelectorAll('.active'), function(el, i) {
    el.classList.remove('active');
  });
}

function findCategoryPath() {
  var sel = document.querySelector('.inline-site-toc .selected');
  if (!sel || !sel.parentElement.previousElementSibling)
    return;
  return sel.parentElement.previousElementSibling.getAttribute('href');
}

function findPillar(el) {
  var p = el;
  while (p && !p.classList.contains('pillar')) {
    p = p.parentElement;
  }
  return p;
}

// Clicking outside the fatnav.
document.body.addEventListener('click', function(e) {
  hideActive(fatNav);
});

// Fatnav activates onclick and closes on mouseleave.
var pillars = document.querySelectorAll('.pillar');
[].forEach.call(pillars, function(pillar, i) {
  pillar.addEventListener('click', function(e) {
    if (e.target.classList.contains('toplevel')) {
      e.stopPropagation(); // prevent body handler from being called.
      var wasAlreadyOpen = this.classList.contains('active');
      hideActive(fatNav); // de-activate other fatnav items.
      wasAlreadyOpen ? this.classList.remove('active') :
                       this.classList.add('active');
    }
  });
});

// Search button is used in tablet & desktop mode.
// In phone mode search is embedded in the menu.
search.addEventListener('click', function(e) {
  if (!isLargerThanPhoneQuery.matches)
    return;
  e.stopPropagation();

  // Only toggle if magnifying glass is clicked.
  if (e.target.localName == 'img') {
    var wasAlreadyOpen = this.classList.contains('active');
    hideActive(fatNav); // de-activate other fatnav items.
    wasAlreadyOpen ? this.classList.remove('active') :
                     this.classList.add('active');
    if (!wasAlreadyOpen) {
      var searchField = document.getElementById('chrome-docs-cse-input');
      var cse = google && google.search && google.search.cse &&
                google.search.cse.element.getElement('results') || null;
      if (cse)
        cse.clearAllResults();
      searchField.select();
      searchField.focus();
    }
  }
});

// In phone mode, show the fatnav when the menu button is clicked.
mobileNavCollasper.addEventListener('click', function(e) {
  if (isLargerThanPhoneQuery.matches)
    return;
  e.stopPropagation();
  fatNav.classList.toggle('active');
  this.classList.toggle('active');
});

if (!isTouch) {
  // Hitting ESC hides fatnav menus.
  document.body.addEventListener('keydown', function(e) {
    if (e.keyCode == 27) { // ESC
      hideActive(fatNav);
    }
  });
}

// Highlight selected menu item based on the current URL
for (var i = 0, a; a = catLinks[i]; i++) {
  var href = a.getAttribute('href');
  if (href === window.location.pathname || catPath && href === catPath) {
    a.classList.add('highlight');
    p = findPillar(a);
    if (p) {
      p.classList.add('highlight');
    }
    break;
  }
}

})();
