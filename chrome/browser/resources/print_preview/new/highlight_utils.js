// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * @param {!HTMLElement} element The element to update. Element should have a
   *     shadow root.
   * @param {?RegExp} query The current search query
   * @param {boolean} wasHighlighted Whether the element was previously
   *     highlighted.
   * @return {boolean} Whether the element is highlighted after the update.
   */
  function updateHighlights(element, query, wasHighlighted) {
    if (wasHighlighted) {
      cr.search_highlight_utils.findAndRemoveHighlights(element);
      cr.search_highlight_utils.findAndRemoveBubbles(element);
    }

    if (!query)
      return false;

    let isHighlighted = false;
    element.shadowRoot.querySelectorAll('.searchable').forEach(childElement => {
      childElement.childNodes.forEach(node => {
        if (node.nodeType != Node.TEXT_NODE)
          return;

        const textContent = node.nodeValue.trim();
        if (textContent.length == 0)
          return;

        if (query.test(textContent)) {
          isHighlighted = true;
          // Don't highlight <select> nodes, yellow rectangles can't be
          // displayed within an <option>.
          if (node.parentNode.nodeName != 'OPTION') {
            cr.search_highlight_utils.highlight(node, textContent.split(query));
          } else {
            const selectNode = node.parentNode.parentNode;
            // The bubble should be parented by the select node's parent.
            // Note: The bubble's ::after element, a yellow arrow, will not
            // appear correctly in print preview without SPv175 enabled. See
            // https://crbug.com/817058.
            cr.search_highlight_utils.highlightControlWithBubble(
                /** @type {!HTMLElement} */ (assert(selectNode.parentNode)),
                textContent.match(query)[0]);
          }
        }
      });
    });
    return isHighlighted;
  }

  return {
    updateHighlights: updateHighlights,
  };
});
