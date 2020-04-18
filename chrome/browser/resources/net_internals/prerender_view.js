// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This view displays information related to Prerendering.
 */
var PrerenderView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function PrerenderView() {
    assertFirstConstructorCall(PrerenderView);

    // Call superclass's constructor.
    superClass.call(this, PrerenderView.MAIN_BOX_ID);

    g_browser.addPrerenderInfoObserver(this, true);
  }

  PrerenderView.TAB_ID = 'tab-handle-prerender';
  PrerenderView.TAB_NAME = 'Prerender';
  PrerenderView.TAB_HASH = '#prerender';

  // IDs for special HTML elements in prerender_view.html
  PrerenderView.MAIN_BOX_ID = 'prerender-view-tab-content';

  // Used in tests.
  PrerenderView.HISTORY_TABLE_ID = 'prerender-view-history-table';
  PrerenderView.ACTIVE_TABLE_ID = 'prerender-view-active-table';

  cr.addSingletonGetter(PrerenderView);

  PrerenderView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    onLoadLogFinish: function(data) {
      return this.onPrerenderInfoChanged(data.prerenderInfo);
    },

    onPrerenderInfoChanged: function(prerenderInfo) {
      if (!prerenderInfo)
        return false;
      var input = new JsEvalContext(prerenderInfo);
      jstProcess(input, $(PrerenderView.MAIN_BOX_ID));
      return true;
    }
  };

  return PrerenderView;
})();
