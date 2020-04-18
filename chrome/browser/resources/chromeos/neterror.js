// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function toggleHelpBox() {
  var helpBoxOuter = $('details');
  helpBoxOuter.classList.toggle('hidden');
  var detailsButton = $('details-button');
  if (helpBoxOuter.classList.contains('hidden')) {
    detailsButton.innerText = detailsButton.detailsText;
  } else {
    detailsButton.innerText = detailsButton.hideDetailsText;
  }
}

function diagnoseErrors() {
  window.domAutomationController.send('open_connectivity_diagnostics');
}

// Subframes use a different layout but the same html file.  This is to make it
// easier to support platforms that load the error page via different
// mechanisms (Currently just iOS).
if (window.top.location != window.location)
  document.documentElement.setAttribute('subframe', '');

// Re-renders the error page using |strings| as the dictionary of values.
// Used by NetErrorTabHelper to update DNS error pages with probe results.
function updateForDnsProbe(strings) {
  var context = new JsEvalContext(strings);
  jstProcess(context, $('t'));
}

// Given the classList property of an element, adds an icon class to the list
// and removes the previously-
function updateIconClass(classList, newClass) {
  var oldClass;

  if (classList.hasOwnProperty('last_icon_class')) {
    oldClass = classList['last_icon_class'];
    if (oldClass == newClass)
      return;
  }

  classList.add(newClass);
  if (oldClass !== undefined)
    classList.remove(oldClass);

  classList['last_icon_class'] = newClass;

  if (newClass == 'icon-offline') {
    document.body.classList.add('offline');
    new Runner('.interstitial-wrapper');
  }
}
