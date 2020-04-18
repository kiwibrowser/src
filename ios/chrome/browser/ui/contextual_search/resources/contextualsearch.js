// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Support code for the Contextual Search feature. Given a tap
 * location, locates and highlights the word at that location, and retuns
 * some contextual data about the selection.
 *
 */


/**
 * Namespace for this file.  Depends on __gCrWeb having already been injected.
 */
__gCrWeb['contextualSearch'] = {};

/* Anyonymizing block */
new function() {

/**
 * Utility loggging function.
 * @param {string} message Text of log message to be sent to application.
 */
__gCrWeb['contextualSearch'].cxlog = function(message) {
  if (Context.debugMode) {
    console.log('[CS] ' + message);
  }
};

/**
 * Enables or disabled the selection change notification forward to the
 * controller.
 * @param {boolean} enabled whether to turn on (true) or off (false).
 */
__gCrWeb['contextualSearch'].enableSelectionChangeListener = function(enabled) {
  if (enabled) {
    document.addEventListener('selectionchange',
                              Context.selectionChanged,
                              true);
  } else {
    document.removeEventListener('selectionchange',
                                 Context.selectionChanged,
                                 true);
  }
};

__gCrWeb['contextualSearch'].getMutatedElementCount = function() {
  return Context.mutationCount;
};

/**
 * Enables the DOM mutation event listener.
 * @param {number} delay after a mutation event before a tap event can be
 * handled.
 */
__gCrWeb['contextualSearch'].setMutationObserverDelay = function(delay) {
  Context.DOMMutationTimeoutMillisecond = delay;

  // select the target node
  var target = document.body;

  // create an observer instance
  Context.DOMMutationObserver = new MutationObserver(function(mutations) {
    // Clear any mutation records older than |DOMMutationTimeoutMillisecond|.
    // Only do this every |DOMMutationTimeoutMillisecond|s to avoid thrashing
    // on pages with (for example) continuous animations.

    var d = new Date();
    if ((d.getTime() - Context.lastMutationPrune) >
        Context.DOMMutationTimeoutMillisecond) {
      Context.processMutations(function(mutationId, mutationTime) {
        if ((d.getTime() - mutationTime) <=
            Context.DOMMutationTimeoutMillisecond) {
          Context.clearMutation(mutationId);
        }
        return false;
      });
      Context.lastMutationPrune = d.getTime();
    }

    mutations.forEach(function(mutation) {
      // Don't count mutations with invalid targets.
      if (!mutation.target) {
        return;
      }
      // Don't count mutations to invisible elements.
      if (mutation.type != 'characterData' && !mutation.target.offsetParent) {
        return;
      }
      // Don't count attribute mutations where the attribute's current and
      // old values are the same. Also don't count attribute mutation where
      // the current and old values are both "false-ish" (so changing a null to
      // an empty string has no effect).
      if (mutation.type == 'attributes') {
        if (mutation.target.getAttribute(mutation.attributeName) ==
            mutation.oldValue) {
          return;
        }
        if (!mutation.target.getAttribute(mutation.attributeName) &&
            !mutation.oldValue) {
          return;
        }
        // Don't count the attribute mutation from setting an ID for tracking or
        // if it is unsetting the ID.
        if (mutation.attributeName == 'id' &&
            (mutation.target.id.match(/__CTXSM___\d__\d+/) ||
             mutation.target.id == '')) {
          return;
        }
      }

      if (mutation.type == 'characterData' &&
          mutation.target.textContent == mutation.oldValue) {
        return;
      }

      // If this is the first mutation after tap, and the mutation target
      // intersects with the highlighted elements, forward it to the Chrome
      // application to check if CS must be dismissed.
      if (Context.highlightRange &&
          !Context.mutationEventForwarded &&
          Context.highlightRange.intersectsNode(mutation.target)) {
        Context.mutationEventForwarded = true;
        __gCrWeb.message.invokeOnHost(
            {'command' : 'contextualSearch.mutationEvent'});
      }

      Context.lastDOMMutationMillisecond = d.getTime();
      // If the mutated item isn't an element, find its parent.
      // If the element doesn't have an ID, assign one to it.
      var idContainer = mutation.target;
      if (mutation.type == 'characterData') {
        idContainer = idContainer.parentElement;
      }
      var id = idContainer.id;
      if (!id) {
        id = Context.newID(Context.lastDOMMutationMillisecond);
        idContainer.id = id;
      }
      Context.recordMutation(id, Context.lastDOMMutationMillisecond);
    });
  });

  // configuration of the observer:
  var config = {
    attributes: true,
    characterData: true,
    subtree: true,
    attributeOldValue: true,
    characterDataOldValue: true
  };

  // pass in the target node, as well as the observer options
  Context.DOMMutationObserver.observe(target, config);
};

/**
 * Enables the body touch end event listener. This will catch touch events that
 * don't call preventDefault.
 * @param {number} delay after a mutation event before a tap event can be
 * handled.
 */
__gCrWeb['contextualSearch'].setBodyTouchListenerDelay = function(delay) {
  Context.touchEventTimeoutMillisecond = delay;
  Context.bodyTouchEndEventListener = function(event) {
      if (!event.defaultPrevented) {
        var d = new Date();
        Context.lastTouchEventMillisecond = d.getTime();
      } else {
        __gCrWeb['contextualSearch'].cxlog('Touch default prevented');
      }
  };
  document.body.addEventListener('touchend', Context.bodyTouchEndEventListener,
      false);
};

/**
 * Disables the DOM mutation listener.
 */
__gCrWeb['contextualSearch'].disableMutationObserver = function() {
  Context.DOMMutationObserver.disconnect();
  Context.DOMMutationObserver = null;
  Context.lastDOMMutationMillisecond = 0;
};

/**
 * Disables the body touchend listener.
 */
__gCrWeb['contextualSearch'].disableBodyTouchListener = function() {
  document.body.removeEventListener('touchend',
      Context.bodyTouchEndEventListener, false);
  Context.lastTouchEventMillisecond = 0;
};

/**
 * Expands the highlight to [startOffset, endOffset] in the surrounding range.
 * @param {number} startOffset first character to include in the range.
 * @param {number} endOffset last character to include in the range.
 * @return {JSON} new highlighted rects.
 */
__gCrWeb['contextualSearch'].expandHighlight =
    function(startOffset, endOffset) {
  if ((startOffset == Context.surroundingRange.highlightStartOffset &&
       endOffset == Context.surroundingRange.highlightEndOffset) ||
      startOffset > endOffset) {
    return;
  }
  var range = Context.createSurroundingRange(startOffset, endOffset);
  Context.highlightRange = range;
  return __gCrWeb['contextualSearch'].highlightRects();
};

/**
 * Returns the rects to draw the current highlight.
 * @return {JSON} hilighted rects.
 */
__gCrWeb['contextualSearch'].highlightRects = function() {
  return __gCrWeb.stringify(
      {'rects' : Context.getHighlightRects(),
          'size': {'width' : document.documentElement.scrollWidth,
                   'height' : document.documentElement.scrollHeight
      }});
};

/**
 * Clears the current highlight.
 */
__gCrWeb['contextualSearch'].clearHighlight = function() {
  Context.highlightRange = null;
};

/**
 * Retrieve the currently highlighted string.
 * This is used for test purposes only.
 * @return {string} the currently highlighted string.
 */
__gCrWeb['contextualSearch'].retrieveHighlighted = function() {
  return Context.rangeToString(Context.highlightRange);
};

/**
 * Prepares Contextual Search for a given point in the window. This method
 * will find which word is located at the given point, extract the context
 * data for that word, and (if a word was located), pass the context data
 * back to the calling application.
 * @param {number} x The point's x coordinate as a ratio of page width.
 * @param {number} y The point's y coordinate as a ratio of page height.
 * @return {object} Empty if no word was found, or the context, x and y if
 *    a word was found.
 */
__gCrWeb['contextualSearch'].handleTapAtPoint = function(x, y) {
  var tapResults;
  if (window.getSelection().toString()) {
    tapResults = new ContextData();
    tapResults.error = 'Failed: selection is not empty.';
    return __gCrWeb.stringify({'context' : tapResults.returnContext()});
  }
  var d = new Date();
  var lastTouchDelta = d.getTime() - Context.lastTouchEventMillisecond;
  if (Context.touchEventTimeoutMillisecond &&
      lastTouchDelta > Context.touchEventTimeoutMillisecond) {
    tapResults = new ContextData();
    tapResults.error = 'Failed: last touch was ' + lastTouchDelta +
        'ms ago (>' + Context.touchEventTimeoutMillisecond + 'ms timeout)';
  } else {
    var absoluteX =
        x * document.documentElement.scrollWidth - document.body.scrollLeft;
    var absoluteY =
        y * document.documentElement.scrollHeight - document.body.scrollTop;
    tapResults = Context.getContextDataFromPoint(absoluteX, absoluteY);

    if (!tapResults.error) {
      var range = tapResults.range;
      if (!range) {
        tapResults.error = 'Failed: context data range was empty';
      } else if (tapResults.surroundingText.length < tapResults.offsetEnd) {
        tapResults.error =
            'Failed: surrounding text is shorter than text offset';
      } else if (tapResults.getSelectedText() != tapResults.selectedText) {
        tapResults.error = 'Failed: offsets do not match selected text: (' +
            tapResults.getSelectedText() + ') vs. (' +
            tapResults.selectedText + ')';
      }
    }
  }
  Context.mutationEventForwarded = false;
  return __gCrWeb.stringify({'context' : tapResults.returnContext()});
};

//------------------------------------------------------------------------------
// ContextData
//------------------------------------------------------------------------------

/**
 * @constructor
 */
var ContextData = function() {};

/**
 * An error message, if any, associated with the context.
 * @type {?string}
 */
ContextData.prototype.error = null;

/**
 * The range containing the selected text.
 * @type {?Range}
 */
ContextData.prototype.range = null;

/**
 * The URL from where the context was extracted.
 * @type {?string}
 */
ContextData.prototype.url = null;

/**
 * The selected text.
 * @type {?string}
 */
ContextData.prototype.selectedText = null;

/**
 * The surrounding text.
 * @type {?string}
 */
ContextData.prototype.surroundingText = null;

/**
 * The start position of the selected text relative to the surrounding text.
 * @type {?number}
 */
ContextData.prototype.offsetStart = null;

/**
 * The end position of the selected text relative to the surrounding text.
 * @type {?number}
 */
ContextData.prototype.offsetEnd = null;

/**
 * The rewritten query.
 * @type {?string}
 */
ContextData.prototype.rewrittenQuery = null;

/**
 * Gets the search query for the context.
 * @return {?string} The search query.
 */
ContextData.prototype.getQuery = function() {
  return this.rewrittenQuery || this.selectedText;
};

/**
 * @return {string} The part of the surrounding text before the selected text.
 */
ContextData.prototype.getTextBefore = function() {
  var selectedText = this.selectedText;
  var surroundingText = this.surroundingText;
  var result = '';
  if (!this.rewrittenQuery && surroundingText) {
    result = surroundingText.substring(0, this.offsetStart);
  }
  return result;
};

/**
 * @return {string} The part of the surrounding text after the selected text.
 */
ContextData.prototype.getTextAfter = function() {
  var selectedText = this.selectedText;
  var surroundingText = this.surroundingText;
  var result = '';
  if (!this.rewrittenQuery && selectedText && surroundingText) {
    result = surroundingText.substring(this.offsetEnd);
  }
  return result;
};

/**
 * @return {string} The selected text as indicated by offsetStart and
 * offsetEnd. This should be the same as selectedText
 */
ContextData.prototype.getSelectedText = function() {
  var surroundingText = this.surroundingText;
  var result = '';
  if (!this.rewrittenQuery && surroundingText) {
    result = surroundingText.substring(this.offsetStart, this.offsetEnd);
  }
  return result;
};

/**
 * @return {JSONDictionary} Context data assembeld for return to native app.
 */
ContextData.prototype.returnContext = function() {
  var context = {'url' : this.url,
    'selectedText' : this.selectedText,
    'surroundingText' : this.surroundingText,
    'offsetStart' : this.offsetStart,
    'offsetEnd' : this.offsetEnd,
    'rects': Context.getHighlightRects()
  };
  if (this.error) {
    context['error'] = this.error;
  }
  return context;
};

//------------------------------------------------------------------------------
// Context
//------------------------------------------------------------------------------

var Context = {};

/**
 * Whether to send log output to the host.
 * @type {bool}
 */
Context.debugMode = false;

/**
 * The maximium amount of time that should be spent searching for a text range,
 * in milliseconds. If the search does not finish within the specified value,
 * it should terminate without returning a result.
 * @const {number}
 */
Context.GET_RANGE_TIMEOUT_MS = 50;

/**
 * Number of surrounding sentences when calculating the surrounding text.
 * @const {number}
 */
Context.NUMBER_OF_CHARS_IN_SURROUNDING_SENTENCES = 1500;

/**
 * Maximum number of chars for a selection to trigger the search.
 * @const {number}
 */
Context.MAX_NUMBER_OF_CHARS_IN_SELECTION = 100;

/**
 * Last range returned by Context.extractSurroundingDataFromRange.
 * @type {JSONObject} Contains startContainer, endContainer, startOffset,
 * endOffset of the surrounding range and relative position of the tapped word.
 */
Context.surroundingRange = null;

/**
 * The range that is curently highlighted.
 * @type {Range}
 */
Context.highlightRange = null;

/**
 * A boolean to check if a mutation event has been forwarded after the latest
 * tap.
 * @type {boolean}
 */
Context.mutationEventForwarded = false;

/**
 * A Regular Expression that matches Unicode word characters.
 * @type {RegExp}
 */
Context.reWordCharacter_ =
    /[\u00C0-\u1FFF\u2C00-\uD7FF\w]/;

/**
 * An observer of the DOM mutation.
 * @type {MutationObserver}
 */
Context.DOMMutationObserver = null;

/**
 * The date of the last DOM mutation (in ms).
 * @type {number}
 */
Context.lastDOMMutationMillisecond = 0;

/**
 * A hash of timestamps keyed by element-id.
 * @type {Object}
 */
Context.mutatedElements = {};

/**
 * A running count of tracked mutated objects.
 * @type {number}
 */
Context.mutationCount = 0;

/**
 * Date of the last time the mutation list was pruned of old entries (in ms).
 * @type {number}
 */
Context.lastMutationPrune = 0;

/**
 * An incrementing integer for generating temporary element ids when needed.
 * @type {number}
 */
Context.mutationIdCounter = 0;

/**
 * A snapshot of the previous text selection (if any), used to determine if a
 * selection change is a new selection or not. previousSelection stores the
 * anchor and focus nodes and offsets of the previously-reported selection.
 * @type {Object}
 */
Context.previousSelection = null;

/**
 * Generates a string suitable for use as a temporary element id.
 * @param {string} nonce A string that varies based on the current time.
 * @return {string} A string to be used as an element id.
 */
Context.newID = function(nonce) {
  return '__CTXSM___' + (Context.mutationIdCounter++) + '__' + nonce;
};

/**
 * The timeout of DOM mutation after which a contextual search can be triggered
 * (in ms)
 * @type {number}
 */
Context.DOMMutationTimeoutMillisecond = 200;

/**
 * An observer of body to catch unhandled touch events.
 * @type {EventListener}
 */
Context.bodyTouchEndEventListener = null;

/**
 * The date of the last unhandled touch event (in ms).
 * @type {number}
 */
Context.lastTouchEventMillisecond = 0;

/**
 * The timeout of DOM mutation after which a contextual search can be triggered
 * (in ms)
 * @type {number}
 */
Context.touchEventTimeoutMillisecond = 0;

/**
 * List of node types whose contents should not be parsed by Contextual Search.
 * @type {Array.<string>}
 */
Context['invalidElements_'] = [
  'A',
  'APPLET',
  'AREA',
  'AUDIO',
  'BUTTON',
  'CANVAS',
  'EMBED',
  'FRAME',
  'FRAMESET',
  'IFRAME',
  'IMG',
  'INPUT',
  'KEYGEN',
  'LABEL',
  'MAP',
  'OBJECT',
  'OPTGROUP',
  'OPTION',
  'PROGRESS',
  'SCRIPT',
  'SELECT',
  'TEXTAREA',
  'VIDEO'
];

/**
 * List of ARIA roles that define widgets.
 * For more info, see: http://www.w3.org/TR/wai-aria/roles#widget_roles
 * @type {Array.<string>}
 */
Context['widgetRoles_'] = [
  'alert',
  'alertdialog',
  'button',
  'checkbox',
  'dialog',
  'gridcell',
  'link',
  'log',
  'marquee',
  'menuitem',
  'menuitemcheckbox',
  'menuitemradio',
  'option',
  'progressbar',
  'radio',
  'scrollbar',
  'slider',
  'spinbutton',
  'status',
  'tab',
  'tabpanel',
  'textbox',
  'timer',
  'tooltip',
  'treeitem'
];

/**
 * List of ARIA roles that define composite widgets.
 * For more info, see: http://www.w3.org/TR/wai-aria/roles#widget_roles
 * @type {Array.<string>}
 */
Context['compositeWidgetRoles_'] = [
  'combobox',
  'grid',
  'listbox',
  'menu',
  'menubar',
  'radiogroup',
  'tablist',
  'tree',
  'treegrid'
];

/**
 * Mutation record handling
 */

/**
 * Records a DOM mutation.
 * @param {string} mutationId The id of the mutated DOM element.
 * @param {number} mutationTime The time of the mutation.
 */
Context.recordMutation = function(mutationId, mutationTime) {
  Context.mutatedElements[mutationId] = mutationTime;
  Context.mutationCount += 1;
};

/**
 * Clears the record of a DOM mutation.
 * @param {string} mutationId The id of the mutated DOM element.
 */
Context.clearMutation = function(mutationId) {
  delete Context.mutatedElements[mutationId];
  Context.mutationCount -= 1;
};

/**
 * Performs some operation on all recorded mutations, passing the mutated node
 * id and mutation time into func.
 * @param {function} func The function to apply to the recorded mutations.
 */
Context.processMutations = function(func) {
  for (mutationId in Context.mutatedElements) {
    var mutationTime = Context.mutatedElements[mutationId];
    if (func(mutationId, mutationTime)) {
      break;
    }
  }
};

/**
 * Returns whether the selection is valid to trigger a contextual search.
 * An invalid selection is a selection that is either too long or contains a
 * single latin character (there are some site that use x's or o's as crosses or
 * circles), or is included or contains invalid elements.
 * @param {selection} selection The current selection to test.
 * @return {boolean} Whether selection should trigger contextual search.
 */
Context.isSelectionValid = function(selection) {
  var selectionText = selection.toString();
  var length = selectionText.length;
  if (length > Context.MAX_NUMBER_OF_CHARS_IN_SELECTION) {
    return false;
  }
  if (length == 1 && selectionText.codePointAt(0) < 256) {
    return false;
  }

  var rangeCount = selection.rangeCount;
  for (var rangeIndex = 0; rangeIndex < rangeCount; rangeIndex++) {
    // Test if the selection is inside an invalid element.
    var range = window.getSelection().getRangeAt(rangeIndex);
    var element = range.commonAncestorContainer;
    while (element) {
      if (element.nodeType == element.ELEMENT_NODE &&
          !Context.isValidElement(element)) {
        return false;
      }
      element = element.parentElement;
    }

    // Test if the selection contains an invalid element.
    var startNode = range.startContainer.childNodes[range.startOffset] ||
    range.startContainer;
    var endNode = range.endContainer.childNodes[range.endOffset] ||
    range.endContainer;
    element = startNode;
    while (element) {
      if (element.nodeType == element.ELEMENT_NODE &&
          !Context.isValidElement(element)) {
        return false;
      }
      element = Context.getNextNode(element, endNode, false);
    }
  }
  return true;
};

/**
 * Forwards the selection changed notification to the controller class.
 */
Context.selectionChanged = function() {
  var newSelection = window.getSelection();
  if (!newSelection.toString()) {
    Context.previousSelection = null;
    return;
  }
  var updated = false;
  if (Context.previousSelection) {
    updated =
        (Context.previousSelection.anchorNode == newSelection.anchorNode &&
         Context.previousSelection.anchorOffset == newSelection.anchorOffset) ||
        (Context.previousSelection.focusNode == newSelection.focusNode &&
         Context.previousSelection.focusOffset == newSelection.focusOffset);
  }
  var selectionText = newSelection.toString();
  var valid = true;
  if (!Context.isSelectionValid(newSelection)) {
    // Mark selection as invalid.
    selectionText = '';
    valid = false;
  }

  __gCrWeb.message.invokeOnHost(
      {'command' : 'contextualSearch.selectionChanged',
          'text' : selectionText,
       'updated' : updated,
         'valid' : valid
  });

  // Snapshot the selection for comparison.
  Context.previousSelection = {
    'anchorNode' : newSelection.anchorNode,
    'anchorOffset' : newSelection.anchorOffset,
    'focusNode' : newSelection.focusNode,
    'focusOffset' : newSelection.focusOffset
  };
};

/**
 * Gets the data necessary to create a Contextual Search from a given point
 * in the window.
 * @param {number} x The point's x coordinate.
 * @param {number} y The point's y coordinate.
 * @return {ContextData} The object describing the context.
 */
Context.getContextDataFromPoint = function(x, y) {
  var contextData = Context.contextFromPoint(x, y);

  if (contextData.error) {
    return contextData;
  }
  var range = contextData.range = Context.getWordRangeFromPoint(x, y);
  if (range) {
    contextData.selectedText = range.toString();
    contextData.url = location.href;
    Context.extractSurroundingDataFromRange(contextData, range);
    Context.highlightRange = range;
  }

  return contextData;
};

/**
 * Checks whether the context in a given point is valid. A context will be
 * valid when the element at the given point is not interactive or editable.
 * @param {number} x The point's x coordinate.
 * @param {number} y The point's y coordinate.
 * @return {ContextData} Context data from the point, an error set if invalid.
 * @private
 */
Context.contextFromPoint = function(x, y) {
  // TODO(crbug.com/711350): Evaluate whether this should use context_menu.js's
  // elementFromPoint_() instead?
  var contextData = new ContextData();
  var element = document.elementFromPoint(x, y);
  if (!element) {
    contextData.error = "Failed: Couldn't locate an element at " + x + ', ' + y;
    return contextData;
  }

  var d = new Date();
  var lastDOM = d.getTime() - Context.lastDOMMutationMillisecond;
  if (lastDOM <= Context.DOMMutationTimeoutMillisecond) {
    Context.processMutations(function(mutationId, mutationTime) {
        var mutatedElement = document.getElementById(mutationId);
        if (!mutatedElement) {
          Context.clearMutation(mutationId);
        } else {
          var lastElementMutation = d.getTime() - mutationTime;
          if (lastElementMutation < 0 ||
              (lastElementMutation > Context.DOMMutationTimeoutMillisecond)) {
            return false;  // mutation expired, continue.
          }
          if (element.contains(mutatedElement) ||
              mutatedElement.contains(element)) {
            contextData.error = 'Failed: Tap was in element mutated ' +
                lastElementMutation + 'ms ago (<' +
                Context.DOMMutationTimeoutMillisecond + 'ms interval)';
            return true; // break from processing mutations
          }
        }
        return false; // continue processing mutations
    });
    if (contextData.error)
      return contextData;
  }

  while (element) {
    if (element.nodeType == element.ELEMENT_NODE &&
        !Context.isValidElement(element, false)) {
      contextData.error =
        'Failed: Tap was in an invalid (' + element.nodeName + ') element';
      return contextData;
    }
    element = element.parentElement;
  }

  return contextData;
};

/**
 * Checks whether the given element can be used as a touch target.
 * @see Context.isValidContextFromPoint_
 * @param {Element} element The element in question.
 * @param {boolean} forDisplay Whether we are testing if an element is valid
 * for tap handling (false) or for display (true).
 * @return {boolean} Whether the element is a valid context.
 * @private
 */
Context.isValidElement = function(element, forDisplay) {

  if (element.nodeName == 'A') {
    return forDisplay;
  }

  if (Context.invalidElements_.indexOf(element.nodeName) != -1) {
    __gCrWeb['contextualSearch'].cxlog(
        'Failed: ' + element.nodeName + ' element was invalid');
    return false;
  }

  if (element.getAttribute('contenteditable')) {
    __gCrWeb['contextualSearch'].cxlog(
        'Failed: ' + element.nodeName + ' element was editable');
    return false;
  }

  var role = element.getAttribute('role');
  if (Context.widgetRoles_.indexOf(role) != -1 ||
      Context.compositeWidgetRoles_.indexOf(role) != -1) {
    __gCrWeb['contextualSearch'].cxlog(
        'Failed: ' + element.nodeName + ' role ' + role + ' was invalid');
    return false;
  }

  if (forDisplay) {
    var style = window.getComputedStyle(element);
    if (style.display === 'none') {
      __gCrWeb['contextualSearch'].cxlog(
          'Failed: ' + element.nodeName + ' hidden');
      return false;
    }
  }

  return true;
};

/**
 * Gets the word range located at a given point. This method will find the
 * word whose bounding rectangle contains the given point.
 * @param {number} x The point's x coordinate.
 * @param {number} y The point's y coordinate.
 * @return {Range} The word range at the given point.
 * @private
 */
Context.getWordRangeFromPoint = function(x, y) {
  var element = document.elementFromPoint(x, y);
  var range = null;
  try {
    range = Context.findWordRangeFromPointRecursive(element, x, y);
  } catch (e) {
    __gCrWeb['contextualSearch'].cxlog(
        'Recursive word find failed: ' + e.message);
  }

  return range;
};

/**
 * Recursively gets the word range located at a given point.
 * @see Context.getWordRangeFromPoint_
 * @param {Node} node The node being inspected.
 * @param {number} x The point's x coordinate.
 * @param {number} y The point's y coordinate.
 * @return {Range} The word range at the given point.
 * @private
 */
Context.findWordRangeFromPointRecursive = function(node, x, y) {
  if (!node) {
    return null;
  }

  if (node.nodeType == node.TEXT_NODE) {
    var position = Context.findCharacterPositionInTextFromPoint(node, x, y);
    if (position == -1) {
      return null;
    }

    var range = node.ownerDocument.createRange();
    range.setStart(node, position);
    range.setEnd(node, position);
    range.expand('word');

    if (Context.rangeContainsPoint(range, x, y)) {
      return range;
    }

    if (range) {
      range.detach();
    }
  } else if (node.nodeType == node.ELEMENT_NODE) {
    var childNodes = node.childNodes;
    var childNodesLength = childNodes.length;
    for (var i = 0, length = childNodesLength; i < length; i++) {
      var childNode = childNodes[i];
      var range = childNode.ownerDocument.createRange();
      range.selectNodeContents(childNode);

      if (Context.rangeContainsPoint(range, x, y)) {
        range.detach();
        return Context.findWordRangeFromPointRecursive(childNode, x, y);
      } else {
        range.detach();
      }
    }
  }

  return null;
};

/**
 * Gets the position of the character range located at a given point. This
 * method will find the single character whose bounding rectangle contains
 * the given point and return the position of that character in the text
 * node string. If not character is found this method returns -1.
 * @see Context.findWordRangeFromPointRecursive_
 * @param {number} x The point's x coordinate.
 * @param {number} y The point's y coordinate.
 * @param {Text} node The text node being inspected.
 * @return {number} The position of the character in the text node string.
 * @private
 */
Context.findCharacterPositionInTextFromPoint = function(node, x, y) {
  var startTime = new Date().getTime();

  var start = 0;
  var end = node.textContent.length - 1;

  // Performs a binary search to find a single character whose bouding
  // rectangle contains the given point.
  var range = document.createRange();
  while (!found || (end - start + 1) > 1) {
    if ((new Date().getTime() - startTime) > Context.GET_RANGE_TIMEOUT_MS) {
      __gCrWeb['contextualSearch'].cxlog('Timed out!');
      break;
    }

    var middle = Math.floor((start + end) / 2);
    range.setStart(node, start);
    range.setEnd(node, middle + 1);  // + 1 because end point is non-inclusive.

    var found = Context.rangeContainsPoint(range, x, y);
    if (found) {
      end = middle;
    } else {
      start = middle + 1;
    }
  }

  if (found) {
    var text = range.toString();
    // Tests if the character is actually a word character (a letter, digit,
    // underscore, or any Unicode letter). If the character is not a word
    // character it means the given point is in a whitespace, punctuation or
    // other non-relevant characters, and in this case it should not be
    // considered a successful finding.
    if (!Context.reWordCharacter_.test(text)) {
      found = false;
    }
  }

  range.detach();

  return found ? start : -1;
};

/**
 * Gets the surrounding data from a given range.
 * @param {ContextData} contextData Object where the data will be written.
 * @param {Range} range A text range.
 * @private
 */
Context.extractSurroundingDataFromRange = function(contextData, range) {
  var surroundingRange = range.cloneRange();
  var length = surroundingRange.toString().length;
  while (length < Context.NUMBER_OF_CHARS_IN_SURROUNDING_SENTENCES) {
    surroundingRange.expand('sentence');
    var oldLength = length;
    length = surroundingRange.toString().length;
    if (oldLength == length) {
      break;
    }
  }

  var textNodeType = range.startContainer.TEXT_NODE;
  var selectionStartOffset = 0;
  var selectionStartNode = range.startContainer;
  if (range.startContainer.nodeType == textNodeType) {
    selectionStartOffset = range.startOffset;
  } else {
    selectionStartNode = range.startContainer.childNodes[range.startOffset];
  }

  var surroundingStartOffset = 0;

  if (surroundingRange.startContainer.nodeType == textNodeType) {
    surroundingStartOffset = surroundingRange.startOffset;
  }
  var surroundingRemoveAtEnd = 0;
  if (surroundingRange.endContainer.nodeType == textNodeType) {
    surroundingRemoveAtEnd = surroundingRange.endContainer.textContent.length -
        surroundingRange.endOffset;
  }

  // It is possible that invalid nodes are present inside the surrounding range.
  // Extract text to make sure this is not the case.
  var textNodes = Context.textNodesFromRange(surroundingRange);

  var offset = 0;
  var index = 0;
  var surroundingString = '';
  var foundStart = false;
  for (index = 0; index < textNodes.length; index++) {
    if (textNodes[index] === ' ') {
      surroundingString += ' ';
      if (!foundStart) {
        offset += 1;
      }
      continue;
    }
    if (textNodes[index] == selectionStartNode) {
      foundStart = true;
    }
    if (!foundStart) {
      offset += textNodes[index].textContent.length;
    }
    surroundingString += textNodes[index].textContent;
  }
  offset += selectionStartOffset - surroundingStartOffset;

  surroundingString = surroundingString.substring(surroundingStartOffset,
      surroundingString.length - surroundingRemoveAtEnd);

  contextData.surroundingText = surroundingString;
  contextData.offsetStart = offset;
  contextData.offsetEnd = offset + range.toString().length;

  Context.surroundingRange = {
            startContainer: surroundingRange.startContainer,
               startOffset: surroundingRange.startOffset,
              endContainer: surroundingRange.endContainer,
                 endOffset: surroundingRange.endOffset,
      highlightStartOffset: contextData.offsetStart,
        highlightEndOffset: contextData.offsetEnd
  };

  surroundingRange.detach();
};

/**
 * Returns whether character is a whitespace.
 * @param {string} character The character to test.
 * @return {boolean} Whether |character| is a whitespace.
 */
Context.isCharSpace = function(character) {
  return character.trim().length == 0;
};

/**
 * Find next node in a DFS order. A parent is returned before its children.
 * @param {Node} node The current node.
 * @param {Node} endNode The right limit of the DFS.
 * @param {boolean} onlyValid Whether invalid node should be skipped in DFS.
 * @return {Node} The node coming after |node|. Null is endNode is reached.
 */
Context.getNextNode = function(node, endNode, onlyValid) {
  if (!node)
    return null;

  if (node.childNodes.length > 0) {
    node = node.childNodes[0];
    if (node.nodeType == node.TEXT_NODE ||
       (node.nodeType == node.ELEMENT_NODE &&
       (!onlyValid || Context.isValidElement(node, true)))) {
      return node;
    }
    if (node.nodeType == node.ELEMENT_NODE && node.contains(endNode)) {
      return null;
    }
  }

  while (node != null) {
    if (!node.nextSibling) {
      node = node.parentNode;
      continue;
    }
    if (node.contains(endNode))
      return null;
    node = node.nextSibling;
    if (node.nodeType == node.TEXT_NODE ||
       (node.nodeType == node.ELEMENT_NODE &&
       (!onlyValid || Context.isValidElement(node, true)))) {
      return node;
    }
  }
  return null;
};

/**
 * Creates the list of text nodes that are part of range. The list will contain
 * whitespace strings to replace block elements.
 * @param {Range} range The range containing the text information.
 * @return {Array.<Node>} The array of text nodes in the range.
 */
Context.textNodesFromRange = function(range) {
  var blockStack = [];

  var startNode = range.startContainer.childNodes[range.startOffset] ||
      range.startContainer;
  var endNode = range.endContainer.childNodes[range.endOffset] ||
      range.endContainer;

  if (startNode == endNode && startNode.childNodes.length === 0) {
    return [startNode];
  }

  var textNodes = [];
  var node = startNode;
  // Do not add a space as first node.
  var addSpace = false;
  var lastNodeAddedHasSpace = true;

  do {
    if (node.nodeType == node.TEXT_NODE && node.textContent.length > 0) {
      while (blockStack.length && !blockStack[0].contains(node)) {
        addSpace = true;
        blockStack.shift();
      }
      if (addSpace && !lastNodeAddedHasSpace &&
          !Context.isCharSpace(node.textContent[0])) {
        textNodes.push(' ');
        addSpace = false;
      }
      textNodes.push(node);
      lastNodeAddedHasSpace = Context.isCharSpace(
          node.textContent[node.textContent.length - 1]);
    } else if (node.nodeType == node.ELEMENT_NODE) {
      var style = window.getComputedStyle(node);
      if (style && style.display != 'inline') {
        addSpace = true;
        blockStack.unshift(node);
      }
    }
    node = Context.getNextNode(node, endNode, true);
  } while (node && node != endNode);

  if (node == endNode && node.nodeType == node.TEXT_NODE) {
    if (addSpace && !lastNodeAddedHasSpace &&
        !Context.isCharSpace(node.textContent[0])) {
      textNodes.push(' ');
    }
    textNodes.push(node);
  }
  return textNodes;
};

/**
 * Checks if a particular range contains a given point.
 * @param {Range} range A text range.
 * @param {number} x The point's x coordinate.
 * @param {number} y The point's y coordinate.
 * @return {boolean} whether the range contains the given point.
 */
Context.rangeContainsPoint = function(range, x, y) {
  var rects = range.getClientRects();
  var rectsLength = rects.length;
  for (var i = 0, length = rectsLength; i < length; i++) {
    var rect = rects[i];
    var contains = Context.rectContainsPoint(rect, x, y);
    if (contains) {
      return true;
    }
  }
  return false;
};

/**
 * Checks if a particular rectangle contains a given point.
 * @param {Rect} rect A rectangle.
 * @param {number} x The point's x coordinate.
 * @param {number} y The point's y coordinate.
 * @return {boolean} whether the rectangle contains the given point.
 * @private
 */
Context.rectContainsPoint = function(rect, x, y) {
  if (x >= rect.left && x <= rect.right &&
      y >= rect.top && y <= rect.bottom) {
    return true;
  }
  return false;
};

/**
 * Create a new range to the [startOffset, endOffset] in the surrounding
 * range returned by Context.extractSurroundingDataFromRange.
 * @param {number} startOffset first character to include in the range.
 * @param {number} endOffset last character to include in the range.
 * @return {Range} the range including [startOffset, endOffset].
 * @private
 */
Context.createSurroundingRange = function(startOffset, endOffset) {
  if ((startOffset == Context.surroundingRange.highlightStartOffset &&
       endOffset == Context.surroundingRange.highlightEndOffset) ||
      startOffset >= endOffset) {
    return;
  }
  var range = document.createRange();
  range.setStart(Context.surroundingRange.startContainer,
                 Context.surroundingRange.startOffset);
  range.setEnd(Context.surroundingRange.endContainer,
               Context.surroundingRange.endOffset);

  var textNodes = Context.textNodesFromRange(range);
  var highlightRange = document.createRange();
  var length = textNodes.length;
  var offset = 0;
  if (length > 0 && Context.surroundingRange.startContainer == textNodes[0]) {
    // Ignore the text inside |startContainer| before the range.
    offset -= Context.surroundingRange.startOffset;
  }
  var startSet = false;
  for (var index = 0; index < length; index++) {
    var node = textNodes[index];
    if (node === ' ') {
      offset += 1;
      continue;
    }
    if (!startSet && offset + node.textContent.length > startOffset) {
      startSet = true;
      highlightRange.setStart(node, startOffset - offset);
    }
    if (offset + node.textContent.length >= endOffset) {
      highlightRange.setEnd(node, endOffset - offset);
      break;
    }
    offset += node.textContent.length;
  }
  return highlightRange;
};

/**
 * Returns the string contained in the parameter |range|. Text rules are the
 * same as textNodesFromRange.
 * @param {Range} range the range to convert to string.
 * @return {string} the string contained in range.
 * @private
 */
Context.rangeToString = function(range) {
  var textNodes = Context.textNodesFromRange(range);
  var string = '';
  var length = textNodes.length;
  for (var index = 0; index < length; index++) {
    var node = textNodes[index];
    if (node === ' ') {
      string += ' ';
      continue;
    }
    var nodeString = node.textContent;
    if (node == range.endNode) {
      nodeString = nodeString.substring(0, range.endOffset);
    }
    if (node == range.startNode) {
      nodeString.substring(range.startOffset, nodeString.length);
    }
    string += nodeString;
  }
  return string;
};

/**
 * Create the text client rects contained in the current |highlightRange|.
 * @return {string} A string containing a comma separated list of rects in
 * the format 'top bottom left right'. Coordinates are page based (not screen
 * based).
 * @private
 */
Context.getHighlightRects = function() {
  if (Context.highlightRange == null) {
    return '';
  }
  var rectsArray = new Array();
  var rects = Context.highlightRange.getClientRects();
  var rectsLength = rects.length;
  for (var i = 0, length = rectsLength; i < length; i++) {
    var top = rects[i].top + document.body.scrollTop;
    var bottom = rects[i].bottom + document.body.scrollTop;
    var left = rects[i].left + document.body.scrollLeft;
    var right = rects[i].right + document.body.scrollLeft;
    rectsArray[i] = '' + top + ' ' + bottom + ' ' + left + ' ' + right;
  }
  return rectsArray.join(',');
};

/* Anyonymizing block end */
}
