// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This view displays a summary of the state of each QUIC session, and
 * has links to display them in the events tab.
 */
var QuicView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function QuicView() {
    assertFirstConstructorCall(QuicView);

    // Call superclass's constructor.
    superClass.call(this, QuicView.MAIN_BOX_ID);

    g_browser.addQuicInfoObserver(this, true);
  }

  QuicView.TAB_ID = 'tab-handle-quic';
  QuicView.TAB_NAME = 'QUIC';
  QuicView.TAB_HASH = '#quic';

  // IDs for special HTML elements in quic_view.html
  QuicView.MAIN_BOX_ID = 'quic-view-tab-content';

  cr.addSingletonGetter(QuicView);

  QuicView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    onLoadLogFinish: function(data) {
      return this.onQuicInfoChanged(data.quicInfo);
    },

    /**
     * If there are any sessions, display a single table with
     * information on each QUIC session.  Otherwise, displays "None".
     */
    onQuicInfoChanged: function(quicInfo) {
      if (!quicInfo)
        return false;
      var input = new JsEvalContext(quicInfo);
      jstProcess(input, $(QuicView.MAIN_BOX_ID));
      return true;
    },
  };

  return QuicView;
})();
