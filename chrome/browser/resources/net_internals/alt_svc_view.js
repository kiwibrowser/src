// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This view displays the Alt-Svc mappings.
 */
var AltSvcView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function AltSvcView() {
    assertFirstConstructorCall(AltSvcView);

    // Call superclass's constructor.
    superClass.call(this, AltSvcView.MAIN_BOX_ID);

    g_browser.addAltSvcMappingsObserver(this, true);
  }

  AltSvcView.TAB_ID = 'tab-handle-alt-svc';
  AltSvcView.TAB_NAME = 'Alt-Svc';
  AltSvcView.TAB_HASH = '#alt-svc';

  // IDs for special HTML elements in alt_svc_view.html
  AltSvcView.MAIN_BOX_ID = 'alt-svc-view-tab-content';
  AltSvcView.ALTERNATE_PROTOCOL_MAPPINGS_ID =
      'alt-svc-view-alternate-protocol-mappings';

  cr.addSingletonGetter(AltSvcView);

  AltSvcView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    onLoadLogFinish: function(data) {
      // TODO(rch): Remove the check for spdyAlternateProtocolMappings after
      // M53 (It was renamed to altSvcMappings in M50).
      return this.onAltSvcMappingsChanged(
          data.altSvcMappings || data.spdyAlternateProtocolMappings);
    },

    /**
     * Displays the alternate service mappings.
     */
    onAltSvcMappingsChanged: function(altSvcMappings) {
      if (!altSvcMappings)
        return false;
      var input = new JsEvalContext({altSvcMappings: altSvcMappings});
      jstProcess(input, $(AltSvcView.ALTERNATE_PROTOCOL_MAPPINGS_ID));
      return true;
    }
  };

  return AltSvcView;
})();
