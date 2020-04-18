// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

Polymer({
  is: 'extensions-drop-overlay',

  properties: {
    /** @private {boolean} */
    dragEnabled: {
      type: Boolean,
      observer: 'dragEnabledChanged_',
    }
  },

  /** @override */
  created: function() {
    this.hidden = true;
    const dragTarget = document.documentElement;
    this.dragWrapperHandler_ =
        new extensions.DragAndDropHandler(true, dragTarget);
    // TODO(devlin): All these dragTarget listeners leak (they aren't removed
    // when the element is). This only matters in tests at the moment, but would
    // be good to fix.
    dragTarget.addEventListener('extension-drag-started', () => {
      this.hidden = false;
    });
    dragTarget.addEventListener('extension-drag-ended', () => {
      this.hidden = true;
    });
    dragTarget.addEventListener('drag-and-drop-load-error', (e) => {
      this.fire('load-error', e.detail);
    });
    this.dragWrapper_ =
        new cr.ui.DragWrapper(dragTarget, this.dragWrapperHandler_);
  },

  /**
   * @param {boolean} dragEnabled
   * @private
   */
  dragEnabledChanged_: function(dragEnabled) {
    this.dragWrapperHandler_.dragEnabled = dragEnabled;
  },
});
})();
