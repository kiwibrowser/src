// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'welcome-win10',

  properties: {
    // Determines if the combined variant should be displayed. The combined
    // variant includes instructions on how to pin Chrome to the taskbar.
    isCombined: {
      type: Boolean,
      value: false,
    },

    // Indicates if the accelerated flow is enabled.
    isAccelerated: {
      type: Boolean,
      value: function() {
        return loadTimeData.getBoolean('acceleratedFlowEnabled');
      },
      reflectToAttribute: true,
    },
  },

  receivePinnedState_: function(isPinnedToTaskbar) {
    // Allow overriding of the result via a query parameter.
    // TODO(pmonette): Remove these checks when they are no longer needed.
    const VARIANT_KEY = 'variant';
    const VARIANT_TYPE_MAP = {'defaultonly': false, 'combined': true};
    var params = new URLSearchParams(location.search);
    if (params.has(VARIANT_KEY) && params.get(VARIANT_KEY) in VARIANT_TYPE_MAP)
      this.isCombined = VARIANT_TYPE_MAP[params.get(VARIANT_KEY)];
    else
      this.isCombined = !isPinnedToTaskbar;

    // Show the module.
    this.style.opacity = 1;
  },

  ready: function() {
    // The accelerated flow can be overridden with a query parameter.
    const FLOWTYPE_KEY = 'flowtype';
    const FLOW_TYPE_MAP = {'regular': false, 'accelerated': true};
    var params = new URLSearchParams(location.search);
    if (params.has(FLOWTYPE_KEY)) {
      if (params.get(FLOWTYPE_KEY) in FLOW_TYPE_MAP) {
        this.isAccelerated = FLOW_TYPE_MAP[params.get(FLOWTYPE_KEY)];
      } else {
        console.error(
            'Found invalid value for the \'flowtype\' parameter: %s',
            params.get(FLOWTYPE_KEY));
      }
    }

    // Asynchronously check if Chrome is pinned to the taskbar.
    cr.sendWithPromise('getPinnedToTaskbarState')
        .then(this.receivePinnedState_.bind(this));
  },

  computeClasses: function(isCombined) {
    return isCombined ? 'section expandable expanded' : 'section';
  },

  onContinue: function() {
    chrome.send('handleContinue');
  },

  onOpenSettings: function() {
    chrome.send('handleSetDefaultBrowser');
  },

  onToggle: function() {
    if (!this.isCombined)
      return;
    var sections = this.shadowRoot.querySelectorAll('.section.expandable');
    sections.forEach(function(section) {
      var isExpanded = section.classList.toggle('expanded');
      section.querySelector('[role~="button"]')
          .setAttribute('aria-expanded', isExpanded);
    });
  }
});
