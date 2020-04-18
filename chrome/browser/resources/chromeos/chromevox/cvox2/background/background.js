// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The entry point for all ChromeVox2 related code for the
 * background page.
 */

goog.provide('Background');

goog.require('AutomationPredicate');
goog.require('AutomationUtil');
goog.require('BackgroundKeyboardHandler');
goog.require('BrailleCommandData');
goog.require('BrailleCommandHandler');
goog.require('ChromeVoxState');
goog.require('CommandHandler');
goog.require('DesktopAutomationHandler');
goog.require('FindHandler');
goog.require('GestureCommandHandler');
goog.require('LiveRegions');
goog.require('MediaAutomationHandler');
goog.require('NextEarcons');
goog.require('Notifications');
goog.require('Output');
goog.require('Output.EventType');
goog.require('PanelCommand');
goog.require('constants');
goog.require('cursors.Cursor');
goog.require('cvox.BrailleKeyCommand');
goog.require('cvox.ChromeVoxBackground');
goog.require('cvox.ChromeVoxEditableTextBase');
goog.require('cvox.ExtensionBridge');
goog.require('cvox.NavBraille');

goog.scope(function() {
var AutomationNode = chrome.automation.AutomationNode;
var Dir = constants.Dir;
var EventType = chrome.automation.EventType;
var RoleType = chrome.automation.RoleType;
var StateType = chrome.automation.StateType;

/**
 * ChromeVox2 background page.
 * @constructor
 * @extends {ChromeVoxState}
 */
Background = function() {
  ChromeVoxState.call(this);

  /**
   * A list of site substring patterns to use with ChromeVox next. Keep these
   * strings relatively specific.
   * @type {!Array<string>}
   * @private
   */
  this.whitelist_ = ['chromevox_next_test'];

  /**
   * @type {cursors.Range}
   * @private
   */
  this.currentRange_ = null;

  // Manually bind all functions to |this|.
  for (var func in this) {
    if (typeof(this[func]) == 'function')
      this[func] = this[func].bind(this);
  }

  /** @type {!cvox.AbstractEarcons} @private */
  this.nextEarcons_ = new NextEarcons();

  // Read-only earcons.
  Object.defineProperty(cvox.ChromeVox, 'earcons', {
    get: (function() {
           return this.nextEarcons_;
         }).bind(this)
  });

  if (cvox.ChromeVox.isChromeOS) {
    Object.defineProperty(cvox.ChromeVox, 'modKeyStr', {
      get: function() {
        return 'Search';
      }.bind(this)
    });

    Object.defineProperty(cvox.ChromeVox, 'typingEcho', {
      get: function() {
        return parseInt(localStorage['typingEcho'], 10);
      }.bind(this),
      set: function(v) {
        localStorage['typingEcho'] = v;
      }.bind(this)
    });
  }

  Object.defineProperty(cvox.ChromeVox, 'isActive', {
    get: function() {
      return localStorage['active'] !== 'false';
    },
    set: function(value) {
      localStorage['active'] = value;
    }
  });

  Object.defineProperty(cvox.ChromeVox, 'typingEcho', {
    get: function() {
      var typingEcho = parseInt(localStorage['typingEcho'], 10) || 0;
      return typingEcho;
    },
    set: function(value) {
      localStorage['typingEcho'] = value;
    }
  });

  cvox.ExtensionBridge.addMessageListener(this.onMessage_);

  /** @type {!BackgroundKeyboardHandler} @private */
  this.keyboardHandler_ = new BackgroundKeyboardHandler();

  /** @type {!LiveRegions} @private */
  this.liveRegions_ = new LiveRegions(this);

  document.addEventListener('copy', this.onClipboardEvent_);
  document.addEventListener('cut', this.onClipboardEvent_);
  document.addEventListener('paste', this.onClipboardEvent_);

  /** @private {boolean} */
  this.preventPasteOutput_ = false;

  /**
   * Maps a non-desktop root automation node to a range position suitable for
   *     restoration.
   * @type {WeakMap<AutomationNode, cursors.Range>}
   * @private
   */
  this.focusRecoveryMap_ = new WeakMap();

  chrome.automation.getDesktop(function(desktop) {
    /** @type {string} */
    this.chromeChannel_ = desktop.chromeChannel;
  }.bind(this));

  CommandHandler.init();
  FindHandler.init();

  Notifications.onStartup();
};

Background.prototype = {
  __proto__: ChromeVoxState.prototype,

  /**
   * Maps the last node with range in a given root.
   * @type {WeakMap<AutomationNode>}
   */
  get focusRecoveryMap() {
    return this.focusRecoveryMap_;
  },

  /**
   * @override
   */
  getCurrentRange: function() {
    if (this.currentRange_ && this.currentRange_.isValid())
      return this.currentRange_;
    return null;
  },

  /**
   * @override
   */
  setCurrentRange: function(newRange) {
    // Clear anything that was frozen on the braille display whenever
    // the user navigates.
    cvox.ChromeVox.braille.thaw();

    if (newRange && !newRange.isValid())
      return;

    this.currentRange_ = newRange;
    ChromeVoxState.observers.forEach(function(observer) {
      observer.onCurrentRangeChanged(newRange);
    });

    if (this.currentRange_) {
      var start = this.currentRange_.start.node;
      start.makeVisible();

      var root = AutomationUtil.getTopLevelRoot(start);
      if (!root || root.role == RoleType.DESKTOP)
        return;

      var position = {};
      var loc = start.unclippedLocation;
      position.x = loc.left + loc.width / 2;
      position.y = loc.top + loc.height / 2;
      var url = root.docUrl;
      url = url.substring(0, url.indexOf('#')) || url;
      cvox.ChromeVox.position[url] = position;
    }
  },

  /**
   * @override
   */
  navigateToRange: function(range, opt_focus, opt_speechProps) {
    opt_focus = opt_focus === undefined ? true : opt_focus;
    opt_speechProps = opt_speechProps || {};
    var prevRange = this.currentRange_;
    if (opt_focus)
      this.setFocusToRange_(range, prevRange);

    this.setCurrentRange(range);

    var o = new Output();
    var selectedRange;
    var msg;

    if (this.pageSel_ && this.pageSel_.isValid() && range.isValid()) {
      // Compute the direction of the endpoints of each range.

      // Casts are ok because isValid checks node start and end nodes are
      // non-null; Closure just doesn't eval enough to see it.
      var startDir = AutomationUtil.getDirection(
          this.pageSel_.start.node,
          /** @type {!AutomationNode} */ (range.start.node));
      var endDir = AutomationUtil.getDirection(
          this.pageSel_.end.node,
          /** @type {!AutomationNode} */ (range.end.node));

      // Selection across roots isn't supported.
      var pageRootStart = this.pageSel_.start.node.root;
      var pageRootEnd = this.pageSel_.end.node.root;
      var curRootStart = range.start.node.root;
      var curRootEnd = range.end.node.root;

      // Disallow crossing over the start of the page selection and roots.
      if (startDir == Dir.BACKWARD || pageRootStart != pageRootEnd ||
          pageRootStart != curRootStart || pageRootEnd != curRootEnd) {
        o.format('@end_selection');
        this.pageSel_ = null;
      } else {
        // Expand or shrink requires different feedback.
        if (endDir == Dir.FORWARD &&
            (this.pageSel_.end.node != range.end.node ||
             this.pageSel_.end.index <= range.end.index)) {
          msg = '@selected';
        } else {
          msg = '@unselected';
          selectedRange = prevRange;
        }
        this.pageSel_ = new cursors.Range(this.pageSel_.start, range.end);
        if (this.pageSel_)
          this.pageSel_.select();
      }
    } else {
      // Ensure we don't select the editable when we first encounter it.
      var lca = null;
      if (range.start.node && prevRange.start.node) {
        lca = AutomationUtil.getLeastCommonAncestor(
            prevRange.start.node, range.start.node);
      }
      if (!lca || lca.state[StateType.EDITABLE] ||
          !range.start.node.state[StateType.EDITABLE])
        range.select();
    }

    o.withRichSpeechAndBraille(
         selectedRange || range, prevRange, Output.EventType.NAVIGATE)
        .withQueueMode(cvox.QueueMode.FLUSH);

    if (msg)
      o.format(msg);

    for (var prop in opt_speechProps)
      o.format('!' + prop);

    o.go();
  },

  /**
   * Open the options page in a new tab.
   */
  showOptionsPage: function() {
    var optionsPage = {url: 'chromevox/background/options.html'};
    chrome.tabs.create(optionsPage);
  },

  /**
   * @override
   */
  onBrailleKeyEvent: function(evt, content) {
    return BrailleCommandHandler.onBrailleKeyEvent(evt, content);
  },

  /**
   * @param {Object} msg A message sent from a content script.
   * @param {Port} port
   * @private
   */
  onMessage_: function(msg, port) {
    var target = msg['target'];
    var action = msg['action'];

    switch (target) {
      case 'next':
        if (action == 'getIsClassicEnabled') {
          var url = msg['url'];
          var isClassicEnabled = false;
          port.postMessage(
              {target: 'next', isClassicEnabled: isClassicEnabled});
        } else if (action == 'onCommand') {
          CommandHandler.onCommand(msg['command']);
        } else if (action == 'flushNextUtterance') {
          Output.forceModeForNextSpeechUtterance(cvox.QueueMode.FLUSH);
        }
        break;
    }
  },

  /**
   * @override
   */
  markCurrentRange: function() {
    if (!this.currentRange)
      return;

    var root = AutomationUtil.getTopLevelRoot(this.currentRange.start.node);
    if (root)
      this.focusRecoveryMap_.set(root, this.currentRange);
  },

  /**
   * Detects various clipboard events and provides spoken output.
   *
   * Note that paste is explicitly skipped sometimes because during a copy or
   * cut, the copied or cut text is retrieved by pasting into a fake text
   * area. To prevent this from triggering paste output, this staste is tracked
   * via a field.
   * @param {!Event} evt
   * @private
   */
  onClipboardEvent_: function(evt) {
    var text = '';
    if (evt.type == 'paste') {
      if (this.preventPasteOutput_) {
        this.preventPasteOutput_ = false;
        return;
      }
      text = evt.clipboardData.getData('text');
      cvox.ChromeVox.tts.speak(
          Msgs.getMsg(evt.type, [text]), cvox.QueueMode.QUEUE);
    } else if (evt.type == 'copy' || evt.type == 'cut') {
      window.setTimeout(function() {
        this.preventPasteOutput_ = true;
        var textarea = document.createElement('textarea');
        document.body.appendChild(textarea);
        textarea.focus();
        document.execCommand('paste');
        var clipboardContent = textarea.value;
        textarea.remove();
        cvox.ChromeVox.tts.speak(
            Msgs.getMsg(evt.type, [clipboardContent]), cvox.QueueMode.FLUSH);
        ChromeVoxState.instance.pageSel_ = null;
      }.bind(this), 20);
    }
  },

  /** @private */
  setCurrentRangeToFocus_: function() {
    chrome.automation.getFocus(function(focus) {
      if (focus)
        this.setCurrentRange(cursors.Range.fromNode(focus));
      else
        this.setCurrentRange(null);
    }.bind(this));
  },

  /**
   * @param {!cursors.Range} range
   * @param {cursors.Range} prevRange
   * @private
   */
  setFocusToRange_: function(range, prevRange) {
    var start = range.start.node;
    var end = range.end.node;

    // First, see if we've crossed a root. Remove once webview handles focus
    // correctly.
    if (prevRange && prevRange.start.node && start) {
      var entered =
          AutomationUtil.getUniqueAncestors(prevRange.start.node, start);
      var embeddedObject = entered.find(function(f) {
        return f.role == RoleType.EMBEDDED_OBJECT;
      });
      if (embeddedObject && !embeddedObject.state[StateType.FOCUSED])
        embeddedObject.focus();
    }

    if (start.state[StateType.FOCUSED] || end.state[StateType.FOCUSED])
      return;

    var isFocusableLinkOrControl = function(node) {
      return node.state[StateType.FOCUSABLE] &&
          AutomationPredicate.linkOrControl(node);
    };

    // Always try to give nodes selection.
    if (start.defaultActionVerb == chrome.automation.DefaultActionVerb.SELECT) {
      start.doDefault();
    }

    // Next, try to focus the start or end node.
    if (!AutomationPredicate.structuralContainer(start) &&
        start.state[StateType.FOCUSABLE]) {
      if (!start.state[StateType.FOCUSED])
        start.focus();
      return;
    } else if (
        !AutomationPredicate.structuralContainer(end) &&
        end.state[StateType.FOCUSABLE]) {
      if (!end.state[StateType.FOCUSED])
        end.focus();
      return;
    }

    // If a common ancestor of |start| and |end| is a link, focus that.
    var ancestor = AutomationUtil.getLeastCommonAncestor(start, end);
    while (ancestor && ancestor.root == start.root) {
      if (isFocusableLinkOrControl(ancestor)) {
        if (!ancestor.state[StateType.FOCUSED])
          ancestor.focus();
        return;
      }
      ancestor = ancestor.parent;
    }

    // If nothing is focusable, set the sequential focus navigation starting
    // point, which ensures that the next time you press Tab, you'll reach
    // the next or previous focusable node from |start|.
    if (!start.state[StateType.OFFSCREEN])
      start.setSequentialFocusNavigationStartingPoint();
  }
};

/**
 * Converts a list of globs, as used in the extension manifest, to a regular
 * expression that matches if and only if any of the globs in the list matches.
 * @param {!Array<string>} globs
 * @return {!RegExp}
 * @private
 */
Background.globsToRegExp_ = function(globs) {
  return new RegExp(
      '^(' +
      globs
          .map(function(glob) {
            return glob.replace(/[.+^$(){}|[\]\\]/g, '\\$&')
                .replace(/\*/g, '.*')
                .replace(/\?/g, '.');
          })
          .join('|') +
      ')$');
};

new Background();

});  // goog.scope
