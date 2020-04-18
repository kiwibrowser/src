// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var AutomationEvent = chrome.automation.AutomationEvent;
var EventType = chrome.automation.EventType;
var RoleType = chrome.automation.RoleType;
var SelectToSpeakState = chrome.accessibilityPrivate.SelectToSpeakState;

/**
 * CrosSelectToSpeakStartSpeechMethod enums.
 * These values are persisted to logs and should not be renumbered or re-used.
 * See tools/metrics/histograms/enums.xml.
 * @enum {number}
 */
const StartSpeechMethod = {
  MOUSE: 0,
  KEYSTROKE: 1,
};

/**
 * The number of enum values in CrosSelectToSpeakStartSpeechMethod. This should
 * be kept in sync with the enum count in tools/metrics/histograms/enums.xml.
 * @type {number}
 */
const START_SPEECH_METHOD_COUNT = Object.keys(StartSpeechMethod).length;

/**
 * CrosSelectToSpeakStateChangeEvent enums.
 * These values are persisted to logs and should not be renumbered or re-used.
 * See tools/metrics/histograms/enums.xml.
 * @enum {number}
 */
const StateChangeEvent = {
  START_SELECTION: 0,
  CANCEL_SPEECH: 1,
  CANCEL_SELECTION: 2,
};

/**
 * The number of enum values in CrosSelectToSpeakStateChangeEvent. This should
 * be kept in sync with the enum count in tools/metrics/histograms/enums.xml.
 * @type {number}
 */
const STATE_CHANGE_EVENT_COUNT = Object.keys(StateChangeEvent).length;

/**
 * The name of the state change request metric.
 * @type {string}
 */
const STATE_CHANGE_EVENT_METRIC_NAME =
    'Accessibility.CrosSelectToSpeak.StateChangeEvent';

// This must be the same as in ash/system/accessibility/select_to_speak_tray.cc:
// ash::kSelectToSpeakTrayClassName.
const SELECT_TO_SPEAK_TRAY_CLASS_NAME =
    'tray/TrayBackgroundView/SelectToSpeakTray';

// Number of milliseconds to wait after requesting a clipboard read
// before clipboard change and paste events are ignored.
const CLIPBOARD_READ_MAX_DELAY_MS = 1000;

// Number of milliseconds to wait after requesting a clipboard copy
// before clipboard copy events are ignored, used to clear the clipboard
// after reading data in a paste event.
const CLIPBOARD_CLEAR_MAX_DELAY_MS = 500;

// Matches one of the known Drive apps which need the clipboard to find and read
// selected text. Includes sandbox and non-sandbox versions.
const DRIVE_APP_REGEXP =
    /^https:\/\/docs\.(?:sandbox\.)?google\.com\/(?:(?:presentation)|(?:document)|(?:spreadsheets)|(?:drawings)){1}\//;

/**
 * Determines if a node is in one of the known Google Drive apps that needs
 * special case treatment for speaking selected text. Not all Google Drive pages
 * are included, because some are not known to have a problem with selection:
 * Forms is not included since it's relatively similar to any HTML page, for
 * example.
 * @param {AutomationNode=}  node The node to check
 * @return {?AutomationNode} The Drive App root node, or null if none is
 *     found.
 */
function getDriveAppRoot(node) {
  while (node !== undefined && node.root !== undefined) {
    if (node.root.url !== undefined && DRIVE_APP_REGEXP.exec(node.root.url))
      return node.root;
    node = node.root.parent;
  }
  return null;
}

/**
 * @constructor
 */
var SelectToSpeak = function() {
  /**
   * The current state of the SelectToSpeak extension, from
   * SelectToSpeakState.
   * @private {!SelectToSpeakState}
   */
  this.state_ = SelectToSpeakState.INACTIVE;

  /** @private {boolean} */
  this.trackingMouse_ = false;

  /** @private {boolean} */
  this.didTrackMouse_ = false;

  /** @private {boolean} */
  this.isSearchKeyDown_ = false;

  /** @private {boolean} */
  this.isSelectionKeyDown_ = false;

  /** @private {!Set<number>} */
  this.keysCurrentlyDown_ = new Set();

  /** @private {!Set<number>} */
  this.keysPressedTogether_ = new Set();

  /** @private {{x: number, y: number}} */
  this.mouseStart_ = {x: 0, y: 0};

  /** @private {{x: number, y: number}} */
  this.mouseEnd_ = {x: 0, y: 0};

  chrome.automation.getDesktop(function(desktop) {
    this.desktop_ = desktop;

    // After the user selects a region of the screen, we do a hit test at
    // the center of that box using the automation API. The result of the
    // hit test is a MOUSE_RELEASED accessibility event.
    desktop.addEventListener(
        EventType.MOUSE_RELEASED, this.onAutomationHitTest_.bind(this), true);

    // When Select-To-Speak is active, we do a hit test on the active node
    // and the result is a HOVER accessibility event. This event is used to
    // check that the current node is in the foreground window.
    desktop.addEventListener(
        EventType.HOVER, this.onHitTestCheckCurrentNodeMatches_.bind(this),
        true);
  }.bind(this));

  /** @private {?string} */
  this.voiceNameFromPrefs_ = null;

  /** @private {?string} */
  this.voiceNameFromLocale_ = null;

  /** @private {Set<string>} */
  this.validVoiceNames_ = new Set();

  /** @private {number} */
  this.speechRate_ = 1.0;

  /** @private {number} */
  this.speechPitch_ = 1.0;

  /** @private {boolean} */
  this.wordHighlight_ = true;

  /** @const {string} */
  this.color_ = '#f73a98';

  /** @private {string} */
  this.highlightColor_ = '#5e9bff';

  /** @private {boolean} */
  this.readAfterClose_ = true;

  /** @private {?NodeGroupItem} */
  this.currentNode_ = null;

  /** @private {number} */
  this.currentNodeGroupIndex_ = -1;

  /** @private {?Object} */
  this.currentNodeWord_ = null;

  /** @private {?AutomationNode} */
  this.currentBlockParent_ = null;

  /** @private {boolean} */
  this.visible_ = true;

  /** @private {boolean} */
  this.scrollToSpokenNode_ = false;

  /**
   * The timestamp at which clipboard data read was requested by the user
   * doing a "read selection" keystroke on a Google Docs app. If a
   * clipboard change event comes in within CLIPBOARD_READ_MAX_DELAY_MS,
   * Select-to-Speak will read that text out loud.
   * @private {number}
   */
  this.readClipboardDataTimeMs_ = -1;

  /**
   * The interval ID from a call to setInterval, which is set whenever
   * speech is in progress.
   * @private {number|undefined}
   */
  this.intervalId_;

  /** @private {Audio} */
  this.null_selection_tone_ = new Audio('earcons/null_selection.ogg');

  this.initPreferences_();

  this.setUpEventListeners_();
};

/** @const {number} */
SelectToSpeak.SEARCH_KEY_CODE = 91;

/** @const {number} */
SelectToSpeak.CONTROL_KEY_CODE = 17;

/** @const {number} */
SelectToSpeak.READ_SELECTION_KEY_CODE = 83;

/** @const {number} */
SelectToSpeak.NODE_STATE_TEST_INTERVAL_MS = 1000;

SelectToSpeak.prototype = {
  /**
   * Called when the mouse is pressed and the user is in a mode where
   * select-to-speak is capturing mouse events (for example holding down
   * Search).
   *
   * @param {!Event} evt The DOM event
   * @return {boolean} True if the default action should be performed;
   *    we always return false because we don't want any other event
   *    handlers to run.
   */
  onMouseDown_: function(evt) {
    // If the user hasn't clicked 'search', or if they are currently
    // trying to highlight a selection, don't track the mouse.
    if (this.state_ != SelectToSpeakState.SELECTING &&
        (!this.isSearchKeyDown_ || this.isSelectionKeyDown_))
      return false;

    this.onStateChanged_(SelectToSpeakState.SELECTING);

    this.trackingMouse_ = true;
    this.didTrackMouse_ = true;
    this.mouseStart_ = {x: evt.screenX, y: evt.screenY};
    this.cancelIfSpeaking_(false /* don't clear the focus ring */);

    // Fire a hit test event on click to warm up the cache.
    this.desktop_.hitTest(evt.screenX, evt.screenY, EventType.MOUSE_PRESSED);

    this.onMouseMove_(evt);
    return false;
  },

  /**
   * Called when the mouse is moved or dragged and the user is in a
   * mode where select-to-speak is capturing mouse events (for example
   * holding down Search).
   *
   * @param {!Event} evt The DOM event
   * @return {boolean} True if the default action should be performed.
   */
  onMouseMove_: function(evt) {
    if (!this.trackingMouse_)
      return false;

    var rect = rectFromPoints(
        this.mouseStart_.x, this.mouseStart_.y, evt.screenX, evt.screenY);
    chrome.accessibilityPrivate.setFocusRing([rect], this.color_);
    return false;
  },

  /**
   * Called when the mouse is released and the user is in a
   * mode where select-to-speak is capturing mouse events (for example
   * holding down Search).
   *
   * @param {!Event} evt
   * @return {boolean} True if the default action should be performed.
   */
  onMouseUp_: function(evt) {
    if (!this.trackingMouse_)
      return false;
    this.onMouseMove_(evt);
    this.trackingMouse_ = false;
    if (!this.keysCurrentlyDown_.has(SelectToSpeak.SEARCH_KEY_CODE)) {
      // This is only needed to cancel something started with the search key.
      this.didTrackMouse_ = false;
    }

    this.onStateChanged_(SelectToSpeakState.INACTIVE);

    this.mouseEnd_ = {x: evt.screenX, y: evt.screenY};
    var ctrX = Math.floor((this.mouseStart_.x + this.mouseEnd_.x) / 2);
    var ctrY = Math.floor((this.mouseStart_.y + this.mouseEnd_.y) / 2);

    // Do a hit test at the center of the area the user dragged over.
    // This will give us some context when searching the accessibility tree.
    // The hit test will result in a EventType.MOUSE_RELEASED event being
    // fired on the result of that hit test, which will trigger
    // onAutomationHitTest_.
    this.desktop_.hitTest(ctrX, ctrY, EventType.MOUSE_RELEASED);
    return false;
  },

  onClipboardDataChanged_: function() {
    if (Date.now() - this.readClipboardDataTimeMs_ <
        CLIPBOARD_READ_MAX_DELAY_MS) {
      // The data has changed, and we are ready to read it.
      // Get it using a paste.
      document.execCommand('paste');
    }
  },

  onClipboardPaste_: function(evt) {
    if (Date.now() - this.readClipboardDataTimeMs_ <
        CLIPBOARD_READ_MAX_DELAY_MS) {
      // Read the current clipboard data.
      evt.preventDefault();
      this.startSpeech_(evt.clipboardData.getData('text/plain'));
      this.readClipboardDataTimeMs_ = -1;
      // Clear the clipboard data by copying nothing (the current document).
      // Do this in a timeout to avoid a recursive warning per
      // https://crbug.com/363288.
      setTimeout(() => {
        this.clearClipboardDataTimeMs_ = Date.now();
        document.execCommand('copy');
      }, 0);
    }
  },

  onClipboardCopy_: function(evt) {
    if (Date.now() - this.clearClipboardDataTimeMs_ <
        CLIPBOARD_CLEAR_MAX_DELAY_MS) {
      // onClipboardPaste has just completed reading the clipboard for speech.
      // This is used to clear the clipboard.
      evt.clipboardData.setData('text/plain', '');
      evt.preventDefault();
      this.clearClipboardDataTimeMs_ = -1;
    }
  },

  /**
   * Called in response to our hit test after the mouse is released,
   * when the user is in a mode where select-to-speak is capturing
   * mouse events (for example holding down Search).
   *
   * @param {!AutomationEvent} evt The automation event.
   */
  onAutomationHitTest_: function(evt) {
    // Walk up to the nearest window, web area, toolbar, or dialog that the
    // hit node is contained inside. Only speak objects within that
    // container. In the future we might include other container-like
    // roles here.
    var root = evt.target;
    // TODO: Use AutomationPredicate.root instead?
    while (root.parent && root.role != RoleType.WINDOW &&
           root.role != RoleType.ROOT_WEB_AREA &&
           root.role != RoleType.DESKTOP && root.role != RoleType.DIALOG &&
           root.role != RoleType.ALERT_DIALOG &&
           root.role != RoleType.TOOLBAR) {
      root = root.parent;
    }

    var rect = rectFromPoints(
        this.mouseStart_.x, this.mouseStart_.y, this.mouseEnd_.x,
        this.mouseEnd_.y);
    var nodes = [];
    chrome.automation.getFocus(function(focusedNode) {
      // In some cases, e.g. ARC++, the window received in the hit test request,
      // which is computed based on which window is the event handler for the
      // hit point, isn't the part of the tree that contains the actual
      // content. In such cases, use focus to get the root.
      // TODO(katie): Determine if this work-around needs to be ARC++ only. If
      // so, look for classname exoshell on the root or root parent to confirm
      // that a node is in ARC++.
      if (!findAllMatching(root, rect, nodes) && focusedNode &&
          focusedNode.root.role != RoleType.DESKTOP) {
        findAllMatching(focusedNode.root, rect, nodes);
      }
      if (nodes.length == 1 &&
          nodes[0].className == SELECT_TO_SPEAK_TRAY_CLASS_NAME) {
        // Don't read only the Select-to-Speak toggle button in the tray unless
        // more items are being read.
        return;
      }
      this.startSpeechQueue_(nodes);
      this.recordStartEvent_(StartSpeechMethod.MOUSE);
    }.bind(this));
  },

  /**
   * @param {!Event} evt
   */
  onKeyDown_: function(evt) {
    if (this.keysPressedTogether_.size == 0 &&
        evt.keyCode == SelectToSpeak.SEARCH_KEY_CODE) {
      this.isSearchKeyDown_ = true;
    } else if (
        this.keysCurrentlyDown_.size == 1 &&
        evt.keyCode == SelectToSpeak.READ_SELECTION_KEY_CODE &&
        !this.trackingMouse_) {
      // Only go into selection mode if we aren't already tracking the mouse.
      this.isSelectionKeyDown_ = true;
    } else if (!this.trackingMouse_) {
      // Some other key was pressed.
      this.isSearchKeyDown_ = false;
    }

    this.keysCurrentlyDown_.add(evt.keyCode);
    this.keysPressedTogether_.add(evt.keyCode);
  },

  /**
   * @param {!Event} evt
   */
  onKeyUp_: function(evt) {
    if (evt.keyCode == SelectToSpeak.READ_SELECTION_KEY_CODE) {
      if (this.isSelectionKeyDown_ && this.keysPressedTogether_.size == 2 &&
          this.keysPressedTogether_.has(evt.keyCode) &&
          this.keysPressedTogether_.has(SelectToSpeak.SEARCH_KEY_CODE)) {
        this.cancelIfSpeaking_(true /* clear the focus ring */);
        chrome.automation.getFocus(this.requestSpeakSelectedText_.bind(this));
      }
      this.isSelectionKeyDown_ = false;
    } else if (evt.keyCode == SelectToSpeak.SEARCH_KEY_CODE) {
      this.isSearchKeyDown_ = false;

      // If we were in the middle of tracking the mouse, cancel it.
      if (this.trackingMouse_) {
        this.trackingMouse_ = false;
        this.stopAll_();
      }
    }

    // Stop speech when the user taps and releases Control or Search
    // without using the mouse or pressing any other keys along the way.
    if (!this.didTrackMouse_ &&
        (evt.keyCode == SelectToSpeak.SEARCH_KEY_CODE ||
         evt.keyCode == SelectToSpeak.CONTROL_KEY_CODE) &&
        this.keysPressedTogether_.has(evt.keyCode) &&
        this.keysPressedTogether_.size == 1) {
      this.trackingMouse_ = false;
      this.cancelIfSpeaking_(true /* clear the focus ring */);
    }

    this.keysCurrentlyDown_.delete(evt.keyCode);
    if (this.keysCurrentlyDown_.size == 0) {
      this.keysPressedTogether_.clear();
      this.didTrackMouse_ = false;
    }
  },

  /**
   * Queues up selected text for reading by finding the Position objects
   * representing the selection.
   */
  requestSpeakSelectedText_: function(focusedNode) {
    // If nothing is selected, return early.
    if (!focusedNode || !focusedNode.root || !focusedNode.root.anchorObject ||
        !focusedNode.root.focusObject) {
      this.onNullSelection_();
      return;
    }
    let anchorObject = focusedNode.root.anchorObject;
    let anchorOffset = focusedNode.root.anchorOffset || 0;
    let focusObject = focusedNode.root.focusObject;
    let focusOffset = focusedNode.root.focusOffset || 0;
    if (anchorObject === focusObject && anchorOffset == focusOffset) {
      this.onNullSelection_();
      return;
    }
    // First calculate the equivalant position for this selection.
    // Sometimes the automation selection returns a offset into a root
    // node rather than a child node, which may be a bug. This allows us to
    // work around that bug until it is fixed or redefined.
    // Note that this calculation is imperfect: it uses node name length
    // to index into child nodes. However, not all node names are
    // user-visible text, so this does not always work. Instead, we must
    // fix the Blink bug where focus offset is not specific enough to
    // say which node is selected and at what charOffset. See
    // https://crbug.com/803160 for more.
    let anchorPosition =
        getDeepEquivalentForSelection(anchorObject, anchorOffset, true);
    let focusPosition =
        getDeepEquivalentForSelection(focusObject, focusOffset, false);
    let firstPosition;
    let lastPosition;
    if (anchorPosition.node === focusPosition.node) {
      if (anchorPosition.offset < focusPosition.offset) {
        firstPosition = anchorPosition;
        lastPosition = focusPosition;
      } else {
        lastPosition = anchorPosition;
        firstPosition = focusPosition;
      }
    } else {
      let dir =
          AutomationUtil.getDirection(anchorPosition.node, focusPosition.node);
      // Highlighting may be forwards or backwards. Make sure we start at the
      // first node.
      if (dir == constants.Dir.FORWARD) {
        firstPosition = anchorPosition;
        lastPosition = focusPosition;
      } else {
        lastPosition = anchorPosition;
        firstPosition = focusPosition;
      }
    }

    // Adjust such that non-text types don't have offsets into their names.
    if (firstPosition.node.role != RoleType.STATIC_TEXT &&
        firstPosition.node.role != RoleType.INLINE_TEXT_BOX) {
      firstPosition.offset = 0;
    }
    if (lastPosition.node.role != RoleType.STATIC_TEXT &&
        lastPosition.node.role != RoleType.INLINE_TEXT_BOX) {
      lastPosition.offset = lastPosition.node.name ?
          lastPosition.node.name.length :
          lastPosition.node.value ? lastPosition.node.value.length : 0;
    }
    this.readNodesInSelection_(firstPosition, lastPosition, focusedNode);
  },

  /**
   * Reads nodes between the first and last position selected by the user.
   * @param {Position} firstPosition The first position at which to start
   *     reading.
   * @param {Position} lastPosition The last position at which to stop reading.
   * @param {AutomationNode} focusedNode The node with user focus.
   */
  readNodesInSelection_: function(firstPosition, lastPosition, focusedNode) {
    let nodes = [];
    let selectedNode = firstPosition.node;
    if (selectedNode.name && firstPosition.offset < selectedNode.name.length &&
        !shouldIgnoreNode(selectedNode, /* include offscreen */ true)) {
      // Initialize to the first node in the list if it's valid and inside
      // of the offset bounds.
      nodes.push(selectedNode);
    } else {
      // The selectedNode actually has no content selected. Let the list
      // initialize itself to the next node in the loop below.
      // This can happen if you click-and-drag starting after the text in
      // a first line to highlight text in a second line.
      firstPosition.offset = 0;
    }
    while (selectedNode && selectedNode != lastPosition.node &&
           AutomationUtil.getDirection(selectedNode, lastPosition.node) ===
               constants.Dir.FORWARD) {
      // TODO: Is there a way to optimize the directionality checking of
      // AutomationUtil.getDirection(selectedNode, finalNode)?
      // For example, by making a helper and storing partial computation?
      selectedNode = AutomationUtil.findNextNode(
          selectedNode, constants.Dir.FORWARD,
          AutomationPredicate.leafWithText);
      if (!selectedNode) {
        break;
      } else if (isTextField(selectedNode)) {
        // Dive down into the next text node.
        // Why does leafWithText return text fields?
        selectedNode = AutomationUtil.findNextNode(
            selectedNode, constants.Dir.FORWARD,
            AutomationPredicate.leafWithText);
        if (!selectedNode)
          break;
      }
      if (!shouldIgnoreNode(selectedNode, /* include offscreen */ true))
        nodes.push(selectedNode);
    }
    if (nodes.length > 0) {
      if (lastPosition.node !== nodes[nodes.length - 1]) {
        // The node at the last position was not added to the list, perhaps it
        // was whitespace or invisible. Clear the ending offset because it
        // relates to a node that doesn't exist.
        this.startSpeechQueue_(nodes, firstPosition.offset);
      } else {
        this.startSpeechQueue_(
            nodes, firstPosition.offset, lastPosition.offset);
      }
    } else {
      let driveAppRootNode = getDriveAppRoot(focusedNode);
      if (!driveAppRootNode)
        return;
      chrome.tabs.query({active: true}, (tabs) => {
        if (tabs.length == 0) {
          return;
        }
        let tab = tabs[0];
        this.readClipboardDataTimeMs_ = Date.now();
        this.currentNode_ = new NodeGroupItem(driveAppRootNode, 0, false);
        chrome.tabs.executeScript(tab.id, {
          allFrames: true,
          matchAboutBlank: true,
          code: 'document.execCommand("copy");'
        });
      });
      return;
    }
    this.initializeScrollingToOffscreenNodes_(focusedNode.root);
    this.recordStartEvent_(StartSpeechMethod.KEYSTROKE);
  },

  /**
   * Gets ready to cancel future scrolling to offscreen nodes as soon as
   * a user-initiated scroll is done.
   * @param {AutomationNode=} root The root node to listen for events on.
   */
  initializeScrollingToOffscreenNodes_: function(root) {
    if (!root) {
      return;
    }
    this.scrollToSpokenNode_ = true;
    let listener = (event) => {
      if (event.eventFrom != 'action') {
        // User initiated event. Cancel all future scrolling to spoken nodes.
        // If the user wants a certain scroll position we will respect that.
        this.scrollToSpokenNode_ = false;

        // Now remove this event listener, we no longer need it.
        root.removeEventListener(
            EventType.SCROLL_POSITION_CHANGED, listener, false);
      }
    };
    root.addEventListener(EventType.SCROLL_POSITION_CHANGED, listener, false);
  },

  /**
   * Plays a tone to let the user know they did the correct
   * keystroke but nothing was selected.
   */
  onNullSelection_: function() {
    this.null_selection_tone_.play();
  },

  /**
   * Stop speech. If speech was in-progress, the interruption
   * event will be caught and clearFocusRingAndNode_ will be
   * called, stopping visual feedback as well.
   * If speech was not in progress, i.e. if the user was drawing
   * a focus ring on the screen, this still clears the visual
   * focus ring.
   */
  stopAll_: function() {
    chrome.tts.stop();
    this.clearFocusRing_();
    this.onStateChanged_(SelectToSpeakState.INACTIVE);
  },

  /**
   * Clears the current focus ring and node, but does
   * not stop the speech.
   */
  clearFocusRingAndNode_: function() {
    this.clearFocusRing_();
    // Clear the node and also stop the interval testing.
    this.currentNode_ = null;
    this.currentNodeGroupIndex_ = -1;
    this.currentNodeWord_ = null;
    clearInterval(this.intervalId_);
    this.intervalId_ = undefined;
    this.scrollToSpokenNode_ = false;
  },

  /**
   * Clears the focus ring, but does not clear the current
   * node.
   */
  clearFocusRing_: function() {
    chrome.accessibilityPrivate.setFocusRing([]);
    chrome.accessibilityPrivate.setHighlights([], this.highlightColor_);
  },

  /**
   * Set up event listeners for mouse and keyboard events. These are
   * forwarded to us from the SelectToSpeakEventHandler so they should
   * be interpreted as global events on the whole screen, not local to
   * any particular window.
   */
  setUpEventListeners_: function() {
    document.addEventListener('keydown', this.onKeyDown_.bind(this));
    document.addEventListener('keyup', this.onKeyUp_.bind(this));
    document.addEventListener('mousedown', this.onMouseDown_.bind(this));
    document.addEventListener('mousemove', this.onMouseMove_.bind(this));
    document.addEventListener('mouseup', this.onMouseUp_.bind(this));
    chrome.clipboard.onClipboardDataChanged.addListener(
        this.onClipboardDataChanged_.bind(this));
    document.addEventListener('paste', this.onClipboardPaste_.bind(this));
    chrome.accessibilityPrivate.onSelectToSpeakStateChangeRequested.addListener(
        this.onStateChangeRequested_.bind(this));
    document.addEventListener('copy', this.onClipboardCopy_.bind(this));
    // Initialize the state to SelectToSpeakState.INACTIVE.
    chrome.accessibilityPrivate.onSelectToSpeakStateChanged(this.state_);
  },

  /**
   * Called when Chrome OS is requesting Select-to-Speak to switch states.
   */
  onStateChangeRequested_: function() {
    // Switch Select-to-Speak states on request.
    // We will need to track the current state and toggle from one state to
    // the next when this function is called, and then call
    // accessibilityPrivate.onSelectToSpeakStateChanged with the new state.
    switch (this.state_) {
      case SelectToSpeakState.INACTIVE:
        // Start selection.
        this.trackingMouse_ = true;
        this.onStateChanged_(SelectToSpeakState.SELECTING);
        this.recordSelectToSpeakStateChangeEvent_(
            StateChangeEvent.START_SELECTION);
        break;
      case SelectToSpeakState.SPEAKING:
        // Stop speaking.
        this.cancelIfSpeaking_(true /* clear the focus ring */);
        this.recordSelectToSpeakStateChangeEvent_(
            StateChangeEvent.CANCEL_SPEECH);
        break;
      case SelectToSpeakState.SELECTING:
        // Cancelled selection.
        this.trackingMouse_ = false;
        this.onStateChanged_(SelectToSpeakState.INACTIVE);
        this.recordSelectToSpeakStateChangeEvent_(
            StateChangeEvent.CANCEL_SELECTION);
    }
    this.onStateChangeRequestedCallbackForTest_ &&
        this.onStateChangeRequestedCallbackForTest_();
  },

  /**
   * Enqueue speech for the single given string. The string is not associated
   * with any particular nodes, so this does not do any work around drawing
   * focus rings, unlike startSpeechQueue_ below.
   * @param {string} text The text to speak.
   */
  startSpeech_: function(text) {
    this.prepareForSpeech_();
    let options = this.speechOptions_();
    options.onEvent = (event) => {
      if (event.type == 'start') {
        this.onStateChanged_(SelectToSpeakState.SPEAKING);
        this.testCurrentNode_();
      } else if (
          event.type == 'end' || event.type == 'interrupted' ||
          event.type == 'cancelled') {
        this.onStateChanged_(SelectToSpeakState.INACTIVE);
      }
    };
    chrome.tts.speak(text, options);
  },

  /**
   * Enqueue speech commands for all of the given nodes.
   * @param {Array<AutomationNode>} nodes The nodes to speak.
   * @param {number=} opt_startIndex The index into the first node's text
   * at which to start speaking. If this is not passed, will start at 0.
   * @param {number=} opt_endIndex The index into the last node's text
   * at which to end speech. If this is not passed, will stop at the end.
   */
  startSpeechQueue_: function(nodes, opt_startIndex, opt_endIndex) {
    this.prepareForSpeech_();
    for (var i = 0; i < nodes.length; i++) {
      let node = nodes[i];
      let nodeGroup = buildNodeGroup(nodes, i);
      if (i == 0) {
        // We need to start in the middle of a node. Remove all text before
        // the start index so that it is not spoken.
        // Backfill with spaces so that index counting functions don't get
        // confused.
        // Must check opt_startIndex in its own if statement to make the
        // Closure compiler happy.
        if (opt_startIndex !== undefined) {
          if (nodeGroup.nodes.length > 0 && nodeGroup.nodes[0].hasInlineText) {
            // The first node is inlineText type. Find the start index in
            // its staticText parent.
            let startIndexInParent = getStartCharIndexInParent(nodes[0]);
            opt_startIndex += startIndexInParent;
            nodeGroup.text = ' '.repeat(opt_startIndex) +
                nodeGroup.text.substr(opt_startIndex);
          }
        }
      }
      let isFirst = i == 0;
      // Advance i to the end of this group, to skip all nodes it contains.
      i = nodeGroup.endIndex;
      let isLast = (i == nodes.length - 1);
      if (isLast && opt_endIndex !== undefined && nodeGroup.nodes.length > 0) {
        // We need to stop in the middle of a node. Remove all text after
        // the end index so it is not spoken. Backfill with spaces so that
        // index counting functions don't get confused.
        // This only applies to inlineText nodes.
        if (nodeGroup.nodes[nodeGroup.nodes.length - 1].hasInlineText) {
          let startIndexInParent = getStartCharIndexInParent(nodes[i]);
          opt_endIndex += startIndexInParent;
          nodeGroup.text = nodeGroup.text.substr(
              0,
              nodeGroup.nodes[nodeGroup.nodes.length - 1].startChar +
                  opt_endIndex);
        }
      }
      if (nodeGroup.nodes.length == 0 && !isLast) {
        continue;
      }

      let options = this.speechOptions_();
      options.onEvent = (event) => {
        if (event.type == 'start' && nodeGroup.nodes.length > 0) {
          this.onStateChanged_(SelectToSpeakState.SPEAKING);
          this.currentBlockParent_ = nodeGroup.blockParent;
          this.currentNodeGroupIndex_ = 0;
          this.currentNode_ = nodeGroup.nodes[this.currentNodeGroupIndex_];
          if (this.wordHighlight_) {
            // At 'start', find the first word and highlight that.
            // Clear the previous word in the node.
            this.currentNodeWord_ = null;
            // If this is the first nodeGroup, pass the opt_startIndex.
            // If this is the last nodeGroup, pass the opt_endIndex.
            this.updateNodeHighlight_(
                nodeGroup.text, event.charIndex,
                isFirst ? opt_startIndex : undefined,
                isLast ? opt_endIndex : undefined);
          } else {
            this.testCurrentNode_();
          }
        } else if (event.type == 'interrupted' || event.type == 'cancelled') {
          this.onStateChanged_(SelectToSpeakState.INACTIVE);
        } else if (event.type == 'end') {
          if (isLast)
            this.onStateChanged_(SelectToSpeakState.INACTIVE);
        } else if (event.type == 'word') {
          console.debug(nodeGroup.text + ' (index ' + event.charIndex + ')');
          console.debug('-'.repeat(event.charIndex) + '^');
          if (this.currentNodeGroupIndex_ + 1 < nodeGroup.nodes.length) {
            let next = nodeGroup.nodes[this.currentNodeGroupIndex_ + 1];
            // Check if we've reached this next node yet using the
            // character index of the event. Add 1 for the space character
            // between words, and another to make it to the start of the
            // next node name.
            if (event.charIndex + 2 >= next.startChar) {
              // Move to the next node.
              this.currentNodeGroupIndex_ += 1;
              this.currentNode_ = next;
              this.currentNodeWord_ = null;
              if (!this.wordHighlight_) {
                // If we are doing a per-word highlight, we will test the
                // node after figuring out what the currently highlighted
                // word is.
                this.testCurrentNode_();
              }
            }
          }
          if (this.wordHighlight_) {
            this.updateNodeHighlight_(
                nodeGroup.text, event.charIndex, undefined,
                isLast ? opt_endIndex : undefined);
          } else {
            this.currentNodeWord_ = null;
          }
        }
      };
      chrome.tts.speak(nodeGroup.text || '', options);
    }
  },

  /**
   * Prepares for speech. Call once before chrome.tts.speak is called.
   */
  prepareForSpeech_: function() {
    this.cancelIfSpeaking_(true /* clear the focus ring */);
    if (this.intervalRef_ !== undefined) {
      clearInterval(this.intervalRef_);
    }
    this.intervalRef_ = setInterval(
        this.testCurrentNode_.bind(this),
        SelectToSpeak.NODE_STATE_TEST_INTERVAL_MS);
  },

  /**
   * Updates the state.
   * @param {!SelectToSpeakState} state
   */
  onStateChanged_: function(state) {
    if (this.state_ != state) {
      if (this.state_ == SelectToSpeakState.SELECTING &&
          state == SelectToSpeakState.INACTIVE && this.trackingMouse_) {
        // If we are tracking the mouse actively, then we have requested tts
        // to stop speaking just before mouse tracking began, so we
        // shouldn't transition into the inactive state now: The call to stop
        // speaking created an async 'cancel' event from the TTS engine that
        // is now resulting in an attempt to set the state inactive.
        return;
      }
      if (state == SelectToSpeakState.INACTIVE)
        this.clearFocusRingAndNode_();
      // Send state change event to Chrome.
      chrome.accessibilityPrivate.onSelectToSpeakStateChanged(state);
      this.state_ = state;
    }
  },

  /**
   * Generates the basic speech options for Select-to-Speak based on user
   * preferences. Call for each chrome.tts.speak.
   * @return {Object} options The TTS options.
   */
  speechOptions_: function() {
    let options = {
      rate: this.speechRate_,
      pitch: this.speechPitch_,
      enqueue: true
    };

    // Pick the voice name from prefs first, or the one that matches
    // the locale next, but don't pick a voice that isn't currently
    // loaded. If no voices are found, leave the voiceName option
    // unset to let the browser try to route the speech request
    // anyway if possible.
    var valid = '';
    this.validVoiceNames_.forEach(function(voiceName) {
      if (valid)
        valid += ',';
      valid += voiceName;
    });
    if (this.voiceNameFromPrefs_ &&
        this.validVoiceNames_.has(this.voiceNameFromPrefs_)) {
      options['voiceName'] = this.voiceNameFromPrefs_;
    } else if (
        this.voiceNameFromLocale_ &&
        this.validVoiceNames_.has(this.voiceNameFromLocale_)) {
      options['voiceName'] = this.voiceNameFromLocale_;
    }
    return options;
  },

  /**
   * Cancels the current speech queue after doing a callback to
   * record a cancel event if speech was in progress. We must cancel
   * before the callback (rather than in it) to avoid race conditions
   * where cancel is called twice.
   * @param {boolean} clearFocusRing Whether to clear the focus ring
   *    as well.
   */
  cancelIfSpeaking_: function(clearFocusRing) {
    chrome.tts.isSpeaking(this.recordCancelIfSpeaking_.bind(this));
    if (clearFocusRing) {
      this.stopAll_();
    } else {
      // Just stop speech
      chrome.tts.stop();
    }
  },

  /**
   * Records a cancel event if speech was in progress.
   * @param {boolean} speaking Whether speech was in progress
   */
  recordCancelIfSpeaking_: function(speaking) {
    if (speaking) {
      this.recordCancelEvent_();
    }
  },

  /**
   * Converts the speech rate into an enum based on
   * tools/metrics/histograms/enums.xml.
   * These values are persisted to logs. Entries should not be
   * renumbered and numeric values should never be reused.
   * @return {number} the current speech rate as an int for metrics.
   */
  speechRateToSparceHistogramInt_: function() {
    return this.speechRate_ * 100;
  },

  /**
   * Converts the speech pitch into an enum based on
   * tools/metrics/histograms/enums.xml.
   * These values are persisted to logs. Entries should not be
   * renumbered and numeric values should never be reused.
   * @return {number} the current speech pitch as an int for metrics.
   */
  speechPitchToSparceHistogramInt_: function() {
    return this.speechPitch_ * 100;
  },

  /**
   * Records an event that Select-to-Speak has begun speaking.
   * @param {number} method The CrosSelectToSpeakStartSpeechMethod enum
   *    that reflects how this event was triggered by the user.
   */
  recordStartEvent_: function(method) {
    chrome.metricsPrivate.recordUserAction(
        'Accessibility.CrosSelectToSpeak.StartSpeech');
    chrome.metricsPrivate.recordSparseValue(
        'Accessibility.CrosSelectToSpeak.SpeechRate',
        this.speechRateToSparceHistogramInt_());
    chrome.metricsPrivate.recordSparseValue(
        'Accessibility.CrosSelectToSpeak.SpeechPitch',
        this.speechPitchToSparceHistogramInt_());
    chrome.metricsPrivate.recordBoolean(
        'Accessibility.CrosSelectToSpeak.WordHighlighting',
        this.wordHighlight_);
    chrome.metricsPrivate.recordEnumerationValue(
        'Accessibility.CrosSelectToSpeak.StartSpeechMethod', method,
        START_SPEECH_METHOD_COUNT);
  },

  /**
   * Records an event that Select-to-Speak speech has been canceled.
   */
  recordCancelEvent_: function() {
    chrome.metricsPrivate.recordUserAction(
        'Accessibility.CrosSelectToSpeak.CancelSpeech');
  },

  /**
   * Records a user-requested state change event from a given state.
   * @param {number} changeType
   */
  recordSelectToSpeakStateChangeEvent_: function(changeType) {
    chrome.metricsPrivate.recordEnumerationValue(
        STATE_CHANGE_EVENT_METRIC_NAME, changeType, STATE_CHANGE_EVENT_COUNT);
  },

  /**
   * Loads preferences from chrome.storage, sets default values if
   * necessary, and registers a listener to update prefs when they
   * change.
   */
  initPreferences_: function() {
    var updatePrefs =
        (function() {
          chrome.storage.sync.get(
              ['voice', 'rate', 'pitch', 'wordHighlight', 'highlightColor'],
              (function(prefs) {
                if (prefs['voice']) {
                  this.voiceNameFromPrefs_ = prefs['voice'];
                }
                if (prefs['rate']) {
                  this.speechRate_ = parseFloat(prefs['rate']);
                } else {
                  chrome.storage.sync.set({'rate': this.speechRate_});
                }
                if (prefs['pitch']) {
                  this.speechPitch_ = parseFloat(prefs['pitch']);
                } else {
                  chrome.storage.sync.set({'pitch': this.speechPitch_});
                }
                if (prefs['wordHighlight'] !== undefined) {
                  this.wordHighlight_ = prefs['wordHighlight'];
                } else {
                  chrome.storage.sync.set(
                      {'wordHighlight': this.wordHighlight_});
                }
                if (prefs['highlightColor']) {
                  this.highlightColor_ = prefs['highlightColor'];
                } else {
                  chrome.storage.sync.set(
                      {'highlightColor': this.highlightColor_});
                }
              }).bind(this));
        }).bind(this);

    updatePrefs();
    chrome.storage.onChanged.addListener(updatePrefs);

    this.updateDefaultVoice_();
    window.speechSynthesis.onvoiceschanged = (function() {
                                               this.updateDefaultVoice_();
                                             }).bind(this);
  },

  /**
   * Get the list of TTS voices, and set the default voice if not already set.
   */
  updateDefaultVoice_: function() {
    var uiLocale = chrome.i18n.getMessage('@@ui_locale');
    uiLocale = uiLocale.replace('_', '-').toLowerCase();

    chrome.tts.getVoices(
        (function(voices) {
          this.validVoiceNames_ = new Set();

          if (voices.length == 0)
            return;

          voices.forEach((voice) => {
            if (!voice.eventTypes.includes('start') ||
                !voice.eventTypes.includes('end') ||
                !voice.eventTypes.includes('word') ||
                !voice.eventTypes.includes('cancelled')) {
              return;
            }
            this.validVoiceNames_.add(voice.voiceName);
          });

          voices.sort(function(a, b) {
            function score(voice) {
              if (voice.lang === undefined)
                return -1;
              var lang = voice.lang.toLowerCase();
              var s = 0;
              if (lang == uiLocale)
                s += 2;
              if (lang.substr(0, 2) == uiLocale.substr(0, 2))
                s += 1;
              return s;
            }
            return score(b) - score(a);
          });

          this.voiceNameFromLocale_ = voices[0].voiceName;

          chrome.storage.sync.get(
              ['voice'],
              (function(prefs) {
                if (!prefs['voice']) {
                  chrome.storage.sync.set({'voice': voices[0].voiceName});
                }
              }).bind(this));
        }).bind(this));
  },

  /**
   * Hides the speech and focus ring states if necessary based on a node's
   * current state.
   *
   * @param {NodeGroupItem} nodeGroupItem The node to use for updates
   * @param {boolean} inForeground Whether the node is in the foreground window.
   */
  updateFromNodeState_: function(nodeGroupItem, inForeground) {
    switch (getNodeState(nodeGroupItem.node)) {
      case NodeState.NODE_STATE_INVALID:
        // If the node is invalid, continue speech unless readAfterClose_
        // is set to true. See https://crbug.com/818835 for more.
        if (this.readAfterClose_) {
          this.clearFocusRing_();
          this.visible_ = false;
        } else {
          this.stopAll_();
        }
        break;
      case NodeState.NODE_STATE_INVISIBLE:
        // If it is invisible but still valid, just clear the focus ring.
        // Don't clear the current node because we may still use it
        // if it becomes visibile later.
        this.clearFocusRing_();
        this.visible_ = false;
        break;
      case NodeState.NODE_STATE_NORMAL:
      default:
        if (inForeground && !this.visible_) {
          this.visible_ = true;
          // Just came to the foreground.
          this.updateHighlightAndFocus_(nodeGroupItem);
        } else if (!inForeground) {
          this.clearFocusRing_();
          this.visible_ = false;
        }
    }
  },

  /**
   * Updates the speech and focus ring states based on a node's current state.
   *
   * @param {NodeGroupItem} nodeGroupItem The node to use for updates
   */
  updateHighlightAndFocus_: function(nodeGroupItem) {
    if (!this.visible_) {
      return;
    }
    let node = nodeGroupItem.hasInlineText && this.currentNodeWord_ ?
        findInlineTextNodeByCharacterIndex(
            nodeGroupItem.node, this.currentNodeWord_.start) :
        nodeGroupItem.node;
    if (this.scrollToSpokenNode_ && node.state.offscreen) {
      node.makeVisible();
    }
    if (this.wordHighlight_ && this.currentNodeWord_ != null) {
      // Only show the highlight if this is an inline text box.
      // Otherwise we'd be highlighting entire nodes, like images.
      // Highlight should be only for text.
      // Note that boundsForRange doesn't work on staticText.
      if (node.role == RoleType.INLINE_TEXT_BOX) {
        let charIndexInParent = getStartCharIndexInParent(node);
        chrome.accessibilityPrivate.setHighlights(
            [node.boundsForRange(
                this.currentNodeWord_.start - charIndexInParent,
                this.currentNodeWord_.end - charIndexInParent)],
            this.highlightColor_);
      } else {
        chrome.accessibilityPrivate.setHighlights([], this.highlightColor_);
      }
    }
    // Show the parent element of the currently verbalized node with the
    // focus ring. This is a nicer user-facing behavior than jumping from
    // node to node, as nodes may not correspond well to paragraphs or
    // blocks.
    // TODO: Better test: has no siblings in the group, highlight just
    // the one node. if it has siblings, highlight the parent.
    if (this.currentBlockParent_ != null &&
        node.role == RoleType.INLINE_TEXT_BOX) {
      chrome.accessibilityPrivate.setFocusRing(
          [this.currentBlockParent_.location], this.color_);
    } else {
      chrome.accessibilityPrivate.setFocusRing([node.location], this.color_);
    }
  },

  /**
   * Tests the active node to make sure the bounds are drawn correctly.
   */
  testCurrentNode_: function() {
    if (this.currentNode_ == null) {
      return;
    }
    if (this.currentNode_.node.location === undefined) {
      // Don't do the hit test because there is no location to test against.
      // Just directly update Select To Speak from node state.
      this.updateFromNodeState_(this.currentNode_, false);
    } else {
      this.updateHighlightAndFocus_(this.currentNode_);
      // Do a hit test to make sure the node is not in a background window
      // or minimimized. On the result checkCurrentNodeMatchesHitTest_ will be
      // called, and we will use that result plus the currentNode's state to
      // deterimine how to set the focus and whether to stop speech.
      this.desktop_.hitTest(
          this.currentNode_.node.location.left,
          this.currentNode_.node.location.top, EventType.HOVER);
    }
  },

  /**
   * Checks that the current node is in the same window as the HitTest node.
   * Uses this information to update Select-To-Speak from node state.
   */
  onHitTestCheckCurrentNodeMatches_: function(evt) {
    if (this.currentNode_ == null) {
      return;
    }
    chrome.automation.getFocus(function(focusedNode) {
      var window = getNearestContainingWindow(evt.target);
      var currentWindow = getNearestContainingWindow(this.currentNode_.node);
      var inForeground =
          currentWindow != null && window != null && currentWindow == window;
      if (!inForeground && focusedNode && currentWindow) {
        // See if the focused node window matches the currentWindow.
        // This may happen in some cases, for example, ARC++, when the window
        // which received the hit test request is not part of the tree that
        // contains the actual content. In such cases, use focus to get the
        // appropriate root.
        var focusedWindow = getNearestContainingWindow(focusedNode.root);
        inForeground = focusedWindow != null && currentWindow == focusedWindow;
      }
      this.updateFromNodeState_(this.currentNode_, inForeground);
    }.bind(this));
  },

  /**
   * Updates the currently highlighted node word based on the current text
   * and the character index of an event.
   * @param {string} text The current text
   * @param {number} charIndex The index of a current event in the text.
   * @param {number=} opt_startIndex The index at which to start the highlight.
   * This takes precedence over the charIndex.
   * @param {number=} opt_endIndex The index at which to end the highlight. This
   * takes precedence over the next word end.
   */
  updateNodeHighlight_: function(
      text, charIndex, opt_startIndex, opt_endIndex) {
    if (charIndex >= text.length) {
      // No need to do work if we are at the end of the paragraph.
      return;
    }
    // Get the next word based on the event's charIndex.
    let nextWordStart = getNextWordStart(text, charIndex, this.currentNode_);
    let nextWordEnd = getNextWordEnd(
        text, opt_startIndex === undefined ? nextWordStart : opt_startIndex,
        this.currentNode_);
    // Map the next word into the node's index from the text.
    let nodeStart = opt_startIndex === undefined ?
        nextWordStart - this.currentNode_.startChar :
        opt_startIndex - this.currentNode_.startChar;
    let nodeEnd = Math.min(
        nextWordEnd - this.currentNode_.startChar,
        nameLength(this.currentNode_.node));
    if ((this.currentNodeWord_ == null ||
         nodeStart >= this.currentNodeWord_.end) &&
        nodeStart <= nodeEnd) {
      // Only update the bounds if they have increased from the
      // previous node. Because tts may send multiple callbacks
      // for the end of one word and the beginning of the next,
      // checking that the current word has changed allows us to
      // reduce extra work.
      this.currentNodeWord_ = {'start': nodeStart, 'end': nodeEnd};
      this.testCurrentNode_();
    }
  },

  // ---------- Functionality for testing ---------- //

  /**
   * Fires a mock key down event for testing.
   * @param {!Event} event The fake key down event to fire. The object
   * must contain at minimum a keyCode.
   */
  fireMockKeyDownEvent: function(event) {
    this.onKeyDown_(event);
  },

  /**
   * Fires a mock key up event for testing.
   * @param {!Event} event The fake key up event to fire. The object
   * must contain at minimum a keyCode.
   */
  fireMockKeyUpEvent: function(event) {
    this.onKeyUp_(event);
  },

  /**
   * Fires a mock mouse down event for testing.
   * @param {!Event} event The fake mouse down event to fire. The object
   * must contain at minimum a screenX and a screenY.
   */
  fireMockMouseDownEvent: function(event) {
    this.onMouseDown_(event);
  },

  /**
   * Fires a mock mouse up event for testing.
   * @param {!Event} event The fake mouse up event to fire. The object
   * must contain at minimum a screenX and a screenY.
   */
  fireMockMouseUpEvent: function(event) {
    this.onMouseUp_(event);
  },

  /**
   * Function to be called when a state change request is received from the
   * accessibilityPrivate API.
   * @type {?function()}
   */
  onStateChangeRequestedCallbackForTest_: null,
};
