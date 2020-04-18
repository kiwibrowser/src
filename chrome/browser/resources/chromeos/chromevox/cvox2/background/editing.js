// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Processes events related to editing text and emits the
 * appropriate spoken and braille feedback.
 */

goog.provide('editing.TextEditHandler');

goog.require('AutomationTreeWalker');
goog.require('AutomationUtil');
goog.require('Output');
goog.require('Output.EventType');
goog.require('TreePathRecoveryStrategy');
goog.require('cursors.Cursor');
goog.require('cursors.Range');
goog.require('cvox.BrailleBackground');
goog.require('cvox.ChromeVoxEditableTextBase');
goog.require('cvox.LibLouis.FormType');

goog.scope(function() {
var AutomationEvent = chrome.automation.AutomationEvent;
var AutomationNode = chrome.automation.AutomationNode;
var Cursor = cursors.Cursor;
var Dir = constants.Dir;
var EventType = chrome.automation.EventType;
var FormType = cvox.LibLouis.FormType;
var Range = cursors.Range;
var RoleType = chrome.automation.RoleType;
var StateType = chrome.automation.StateType;
var Movement = cursors.Movement;
var Unit = cursors.Unit;

/**
 * A handler for automation events in a focused text field or editable root
 * such as a |contenteditable| subtree.
 * @constructor
 * @param {!AutomationNode} node
 */
editing.TextEditHandler = function(node) {
  /** @const {!AutomationNode} @private */
  this.node_ = node;

  if (!node.state[StateType.EDITABLE])
    throw '|node| must be editable.';

  chrome.automation.getDesktop(function(desktop) {
    // A rich text field is one where selection gets placed on a DOM descendant
    // to a root text field. This is one of:
    // - content editables (detected via richly editable state)
    // - the node is a textarea
    //
    // The only other editables we expect are all single line (including those
    // from ARC++).
    var useRichText =
        node.state[StateType.RICHLY_EDITABLE] || node.htmlTag == 'textarea';

    /** @private {!AutomationEditableText} */
    this.editableText_ = useRichText ? new AutomationRichEditableText(node) :
                                       new AutomationEditableText(node);
  }.bind(this));
};

editing.TextEditHandler.prototype = {
  /** @return {!AutomationNode} */
  get node() {
    return this.node_;
  },

  /**
   * Receives the following kinds of events when the node provided to the
   * constructor is focuse: |focus|, |textChanged|, |textSelectionChanged| and
   * |valueChanged|.
   * An implementation of this method should emit the appropritate braille and
   * spoken feedback for the event.
   * @param {!(AutomationEvent|CustomAutomationEvent)} evt
   */
  onEvent: function(evt) {
    if (evt.type !== EventType.TEXT_CHANGED &&
        evt.type !== EventType.TEXT_SELECTION_CHANGED &&
        evt.type !== EventType.DOCUMENT_SELECTION_CHANGED &&
        evt.type !== EventType.VALUE_CHANGED && evt.type !== EventType.FOCUS)
      return;
    if (!evt.target.state.focused || !evt.target.state.editable ||
        evt.target != this.node_)
      return;

    this.editableText_.onUpdate(evt.eventFrom);
  },

  /**
   * Returns true if selection starts at the first line.
   * @return {boolean}
   */
  isSelectionOnFirstLine: function() {
    return this.editableText_.isSelectionOnFirstLine();
  },

  /**
   * Returns true if selection ends at the last line.
   * @return {boolean}
   */
  isSelectionOnLastLine: function() {
    return this.editableText_.isSelectionOnLastLine();
  },

  /**
   * Moves range to after this text field.
   */
  moveToAfterEditText: function() {
    var after = AutomationUtil.findNextNode(
                    this.node_, Dir.FORWARD, AutomationPredicate.object,
                    {skipInitialSubtree: true}) ||
        this.node_;
    ChromeVoxState.instance.navigateToRange(cursors.Range.fromNode(after));
  }
};

/**
 * A |ChromeVoxEditableTextBase| that implements text editing feedback
 * for automation tree text fields.
 * @constructor
 * @param {!AutomationNode} node
 * @extends {cvox.ChromeVoxEditableTextBase}
 */
function AutomationEditableText(node) {
  if (!node.state.editable)
    throw Error('Node must have editable state set to true.');
  var start = node.textSelStart;
  var end = node.textSelEnd;
  cvox.ChromeVoxEditableTextBase.call(
      this, node.value || '', Math.min(start, end), Math.max(start, end),
      node.state[StateType.PROTECTED] /**password*/, cvox.ChromeVox.tts);
  /** @override */
  this.multiline = node.state[StateType.MULTILINE] || false;
  /** @type {!AutomationNode} @private */
  this.node_ = node;
}

AutomationEditableText.prototype = {
  __proto__: cvox.ChromeVoxEditableTextBase.prototype,

  /**
   * Called when the text field has been updated.
   * @param {string|undefined} eventFrom
   */
  onUpdate: function(eventFrom) {
    var newValue = this.node_.value || '';

    var textChangeEvent = new cvox.TextChangeEvent(
        newValue, this.node_.textSelStart || 0, this.node_.textSelEnd || 0,
        true /* triggered by user */);
    this.changed(textChangeEvent);
    this.outputBraille_();
  },

  /**
   * Returns true if selection starts on the first line.
   */
  isSelectionOnFirstLine: function() {
    return true;
  },

  /**
   * Returns true if selection ends on the last line.
   */
  isSelectionOnLastLine: function() {
    return true;
  },

  /** @override */
  getLineIndex: function(charIndex) {
    return 0;
  },

  /** @override */
  getLineStart: function(lineIndex) {
    return 0;
  },

  /** @override */
  getLineEnd: function(lineIndex) {
    return this.node_.value.length;
  },

  /** @private */
  outputBraille_: function() {
    var output = new Output();
    var range;
    range = Range.fromNode(this.node_);
    output.withBraille(range, null, Output.EventType.NAVIGATE);
    output.go();
  }
};


/**
 * A |ChromeVoxEditableTextBase| that implements text editing feedback
 * for automation tree text fields using anchor and focus selection.
 * @constructor
 * @param {!AutomationNode} node
 * @extends {AutomationEditableText}
 */
function AutomationRichEditableText(node) {
  AutomationEditableText.call(this, node);

  var root = this.node_.root;
  if (!root || !root.anchorObject || !root.focusObject ||
      root.anchorOffset === undefined || root.focusOffset === undefined)
    return;

  this.anchorLine_ = new editing.EditableLine(
      root.anchorObject, root.anchorOffset, root.anchorObject,
      root.anchorOffset);
  this.focusLine_ = new editing.EditableLine(
      root.focusObject, root.focusOffset, root.focusObject, root.focusOffset);

  this.line_ = new editing.EditableLine(
      root.anchorObject, root.anchorOffset, root.focusObject, root.focusOffset);

  this.updateIntraLineState_(this.line_);
}

AutomationRichEditableText.prototype = {
  __proto__: AutomationEditableText.prototype,

  /** @override */
  isSelectionOnFirstLine: function() {
    var deep = this.line_.end_.node;
    while (deep.previousOnLine)
      deep = deep.previousOnLine;
    var next = AutomationUtil.findNextNode(
        deep, Dir.BACKWARD, AutomationPredicate.inlineTextBox);
    if (!next)
      return true;
    var exited = AutomationUtil.getUniqueAncestors(next, deep);
    return !!exited.find(function(item) {
      return item == this.node_;
    }.bind(this));
  },

  /** @override */
  isSelectionOnLastLine: function() {
    var deep = this.line_.end_.node;
    while (deep.nextOnLine)
      deep = deep.nextOnLine;
    var next = AutomationUtil.findNextNode(
        deep, Dir.FORWARD, AutomationPredicate.inlineTextBox);
    if (!next)
      return true;
    var exited = AutomationUtil.getUniqueAncestors(next, deep);
    return !!exited.find(function(item) {
      return item == this.node_;
    }.bind(this));
  },

  /** @override */
  onUpdate: function(eventFrom) {
    var root = this.node_.root;
    if (!root.anchorObject || !root.focusObject ||
        root.anchorOffset === undefined || root.focusOffset === undefined)
      return;

    var anchorLine = new editing.EditableLine(
        root.anchorObject, root.anchorOffset, root.anchorObject,
        root.anchorOffset);
    var focusLine = new editing.EditableLine(
        root.focusObject, root.focusOffset, root.focusObject, root.focusOffset);

    var prevAnchorLine = this.anchorLine_;
    var prevFocusLine = this.focusLine_;
    this.anchorLine_ = anchorLine;
    this.focusLine_ = focusLine;

    // Compute the current line based upon whether the current selection was
    // extended from anchor or focus. The default behavior is to compute lines
    // via focus.
    var baseLineOnStart = prevFocusLine.isSameLineAndSelection(focusLine);
    var isSameSelection =
        baseLineOnStart && prevAnchorLine.isSameLineAndSelection(anchorLine);

    var cur;
    if (isSameSelection && this.line_) {
      // Nothing changed, return.
      return;
    } else {
      cur = new editing.EditableLine(
          root.anchorObject, root.anchorOffset, root.focusObject,
          root.focusOffset, baseLineOnStart);
    }
    var prev = this.line_;
    this.line_ = cur;

    // During continuous read, skip speech (which gets handled in
    // CommandHandler). We use the speech end callback to trigger additional
    // speech.
    if (ChromeVoxState.isReadingContinuously) {
      this.brailleCurrentRichLine_();
      this.updateIntraLineState_(cur);
      return;
    }

    // Selection stayed within the same line(s) and didn't cross into new lines.

    // We must validate the previous lines as state changes in the accessibility
    // tree may have invalidated the lines.
    if (prevAnchorLine.isValidLine() && prevFocusLine.isValidLine() &&
        anchorLine.isSameLine(prevAnchorLine) &&
        focusLine.isSameLine(prevFocusLine)) {
      // Intra-line changes.
      this.changed(new cvox.TextChangeEvent(
          cur.text || '', cur.startOffset, cur.endOffset, true));
      this.brailleCurrentRichLine_();

      // Finally, queue up any text markers/styles at bounds.
      var container = cur.startContainer_;
      if (!container)
        return;

      if (container.markerTypes) {
        // Only consider markers that start or end at the selection bounds.
        var markerStartIndex = -1, markerEndIndex = -1;
        var localStartOffset = cur.localStartOffset;
        for (var i = 0; i < container.markerStarts.length; i++) {
          if (container.markerStarts[i] == localStartOffset) {
            markerStartIndex = i;
            break;
          }
        }

        var localEndOffset = cur.localEndOffset;
        for (var i = 0; i < container.markerEnds.length; i++) {
          if (container.markerEnds[i] == localEndOffset) {
            markerEndIndex = i;
            break;
          }
        }

        if (markerStartIndex > -1)
          this.speakTextMarker_(container.markerTypes[markerStartIndex]);

        if (markerEndIndex > -1)
          this.speakTextMarker_(container.markerTypes[markerEndIndex], true);
      }

      // Start of the container.
      if (cur.containerStartOffset == cur.startOffset)
        this.speakTextStyle_(container);
      else if (cur.containerEndOffset == cur.endOffset)
        this.speakTextStyle_(container, true);

      return;
    }

    // TODO(dtseng): base/extent and anchor/focus are ordered
    // (i.e. anchor/base always comes before focus/extent) in Blink
    // accessibility. However, in other parts of Blink, they are
    // unordered (i.e. anchor is where the selection starts and focus
    // where it ends). The latter is correct. Change this once Blink
    // ax gets fixed.
    var curBase = baseLineOnStart ? focusLine : anchorLine;

    if (cur.text == '') {
      // This line has no text content. Describe the DOM selection.
      new Output()
          .withRichSpeechAndBraille(
              new Range(cur.start_, cur.end_),
              new Range(prev.start_, prev.end_), Output.EventType.NAVIGATE)
          .go();
    } else if (
        !cur.hasCollapsedSelection() &&
        (curBase.isSameLine(prevAnchorLine) ||
         curBase.isSameLine(prevFocusLine))) {
      // This is a selection that gets extended from the same anchor.

      // Speech requires many more states than braille.
      var curExtent = baseLineOnStart ? anchorLine : focusLine;
      var text = '';
      var suffixMsg = '';
      if (curBase.isBeforeLine(curExtent)) {
        // Forward selection.
        if (prev.isBeforeLine(curBase)) {
          // Wrapped across the baseline. Read out the new selection.
          suffixMsg = 'selected';
          text = this.getTextSelection_(
              curBase.startContainer_, curBase.localStartOffset,
              curExtent.endContainer_, curExtent.localEndOffset);
        } else {
          if (prev.isBeforeLine(curExtent)) {
            // Grew.
            suffixMsg = 'selected';
            text = this.getTextSelection_(
                prev.endContainer_, prev.localEndOffset,
                curExtent.endContainer_, curExtent.localEndOffset);
          } else {
            // Shrank.
            suffixMsg = 'unselected';
            text = this.getTextSelection_(
                curExtent.endContainer_, curExtent.localEndOffset,
                prev.endContainer_, prev.localEndOffset);
          }
        }
      } else {
        // Backward selection.
        if (curBase.isBeforeLine(prev)) {
          // Wrapped across the baseline. Read out the new selection.
          suffixMsg = 'selected';
          text = this.getTextSelection_(
              curExtent.startContainer_, curExtent.localStartOffset,
              curBase.endContainer_, curBase.localEndOffset);
        } else {
          if (curExtent.isBeforeLine(prev)) {
            // Grew.
            suffixMsg = 'selected';
            text = this.getTextSelection_(
                curExtent.startContainer_, curExtent.localStartOffset,
                prev.startContainer_, prev.localStartOffset);
          } else {
            // Shrank.
            suffixMsg = 'unselected';
            text = this.getTextSelection_(
                prev.startContainer_, prev.localStartOffset,
                curExtent.startContainer_, curExtent.localStartOffset);
          }
        }
      }

      cvox.ChromeVox.tts.speak(text, cvox.QueueMode.CATEGORY_FLUSH);
      cvox.ChromeVox.tts.speak(Msgs.getMsg(suffixMsg), cvox.QueueMode.QUEUE);
      this.brailleCurrentRichLine_();
    } else {
      // A catch-all for any other transitions.

      // Describe the current line. This accounts for previous/current
      // selections and picking the line edge boundary that changed (as computed
      // above). This is also the code path for describing paste. It also covers
      // jump commands which are non-overlapping selections from prev to cur.
      this.speakCurrentRichLine_(prev);
      this.brailleCurrentRichLine_();
    }
    this.updateIntraLineState_(cur);
  },

  /**
   * @param {AutomationNode|undefined} startNode
   * @param {number} startOffset
   * @param {AutomationNode|undefined} endNode
   * @param {number} endOffset
   * @return {string}
   */
  getTextSelection_: function(startNode, startOffset, endNode, endOffset) {
    if (!startNode || !endNode)
      return '';

    if (startNode == endNode) {
      return startNode.name ? startNode.name.substring(startOffset, endOffset) :
                              '';
    }

    var text = '';
    if (startNode.name)
      text = startNode.name.substring(startOffset);

    for (var node = startNode;
         (node = AutomationUtil.findNextNode(
              node, Dir.FORWARD, AutomationPredicate.leafOrStaticText)) &&
         node != endNode;) {
      // Padding needs to get added to break up speech utterances.
      if (node.name)
        text += ' ' + node.name;
    }

    if (endNode.name)
      text += ' ' + endNode.name.substring(0, endOffset);
    return text;
  },

  /**
   * @param {number} markerType
   * @param {boolean=} opt_end
   * @private
   */
  speakTextMarker_: function(markerType, opt_end) {
    // TODO(dtseng): Plumb through constants to automation.
    var msgs = [];
    if (markerType & 1)
      msgs.push(opt_end ? 'misspelling_end' : 'misspelling_start');
    if (markerType & 2)
      msgs.push(opt_end ? 'grammar_end' : 'grammar_start');
    if (markerType & 4)
      msgs.push(opt_end ? 'text_match_end' : 'text_match_start');

    if (msgs.length) {
      msgs.forEach(function(msg) {
        cvox.ChromeVox.tts.speak(
            Msgs.getMsg(msg), cvox.QueueMode.QUEUE,
            cvox.AbstractTts.PERSONALITY_ANNOTATION);
      });
    }
  },

  /**
   * @param {!AutomationNode} style
   * @param {boolean=} opt_end
   * @private
   */
  speakTextStyle_: function(style, opt_end) {
    var msgs = [];
    if (style.state.linked)
      msgs.push(opt_end ? 'link_end' : 'link_start');
    if (style.subscript)
      msgs.push(opt_end ? 'subscript_end' : 'subscript_start');
    if (style.superscript)
      msgs.push(opt_end ? 'superscript_end' : 'superscript_start');
    if (style.bold)
      msgs.push(opt_end ? 'bold_end' : 'bold_start');
    if (style.italic)
      msgs.push(opt_end ? 'italic_end' : 'italic_start');
    if (style.underline)
      msgs.push(opt_end ? 'underline_end' : 'underline_start');
    if (style.lineThrough)
      msgs.push(opt_end ? 'line_through_end' : 'line_through_start');

    if (msgs.length) {
      msgs.forEach(function(msg) {
        cvox.ChromeVox.tts.speak(
            Msgs.getMsg(msg), cvox.QueueMode.QUEUE,
            cvox.AbstractTts.PERSONALITY_ANNOTATION);
      });
    }
  },

  /**
   * @param {editing.EditableLine} prevLine
   * @private
   */
  speakCurrentRichLine_: function(prevLine) {
    var prev = prevLine ? prevLine.startContainer_ : this.node_;
    var lineNodes =
        /** @type {Array<!AutomationNode>} */ (
            this.line_.value_.getSpansInstanceOf(
                /** @type {function()} */ (this.node_.constructor)));
    var queueMode = cvox.QueueMode.CATEGORY_FLUSH;
    for (var i = 0, cur; cur = lineNodes[i]; i++) {
      if (cur.children.length)
        continue;

      var o = new Output()
                  .withRichSpeech(
                      Range.fromNode(cur), prev ? Range.fromNode(prev) : null,
                      Output.EventType.NAVIGATE)
                  .withQueueMode(queueMode);

      // Ignore whitespace only output except if it is leading content on the
      // line.
      if (!o.isOnlyWhitespace || i == 0)
        o.go();
      prev = cur;
      queueMode = cvox.QueueMode.QUEUE;
    }
  },

  /** @private */
  brailleCurrentRichLine_: function() {
    var isFirstLine = this.isSelectionOnFirstLine();
    var cur = this.line_;
    if (cur.value_ === null)
      return;

    var value = new MultiSpannable(cur.value_);
    if (!this.node_.constructor)
      return;
    value.getSpansInstanceOf(this.node_.constructor).forEach(function(span) {
      var style = span.role == RoleType.INLINE_TEXT_BOX ? span.parent : span;
      if (!style)
        return;
      var formType = FormType.PLAIN_TEXT;
      // Currently no support for sub/superscript in 3rd party liblouis library.
      if (style.bold)
        formType |= FormType.BOLD;
      if (style.italic)
        formType |= FormType.ITALIC;
      if (style.underline)
        formType |= FormType.UNDERLINE;
      if (formType == FormType.PLAIN_TEXT)
        return;
      var start = value.getSpanStart(span);
      var end = value.getSpanEnd(span);
      value.setSpan(new cvox.BrailleTextStyleSpan(formType), start, end);
    });

    // Provide context for the current selection.
    var context = cur.startContainer_;

    if (context) {
      var output = new Output().suppress('name').withBraille(
          Range.fromNode(context), Range.fromNode(this.node_),
          Output.EventType.NAVIGATE);
      if (output.braille.length) {
        var end = cur.containerEndOffset + 1;
        var prefix = value.substring(0, end);
        var suffix = value.substring(end, value.length);
        value = prefix;
        value.append(Output.SPACE);
        value.append(output.braille);
        if (suffix.length) {
          if (suffix.toString()[0] != Output.SPACE)
            value.append(Output.SPACE);
          value.append(suffix);
        }
      }
    }

    if (isFirstLine) {
      if (!/\s/.test(value.toString()[value.length - 1]))
        value.append(Output.SPACE);
      value.append(Msgs.getMsg('tag_textarea_brl'));
    }
    value.setSpan(new cvox.ValueSpan(0), 0, cur.value_.length);
    value.setSpan(
        new cvox.ValueSelectionSpan(), cur.startOffset, cur.endOffset);
    cvox.ChromeVox.braille.write(new cvox.NavBraille(
        {text: value, startIndex: cur.startOffset, endIndex: cur.endOffset}));
  },

  /** @override */
  describeSelectionChanged: function(evt) {
    // Note that since Chrome allows for selection to be placed immediately at
    // the end of a line (i.e. end == value.length) and since we try to describe
    // the character to the right, just describe it as a new line.
    if ((this.start + 1) == evt.start && evt.start == this.value.length) {
      this.speak('\n', evt.triggeredByUser);
      return;
    }

    cvox.ChromeVoxEditableTextBase.prototype.describeSelectionChanged.call(
        this, evt);
  },

  /** @override */
  getLineIndex: function(charIndex) {
    return 0;
  },

  /** @override */
  getLineStart: function(lineIndex) {
    return 0;
  },

  /** @override */
  getLineEnd: function(lineIndex) {
    return this.value.length;
  },

  /**
   * @private
   * @param {editing.EditableLine} cur Current line.
   */
  updateIntraLineState_: function(cur) {
    this.value = cur.text;
    this.start = cur.startOffset;
    this.end = cur.endOffset;
  }
};

/**
 * @param {!AutomationNode} node The root editable node, i.e. the root of a
 *     contenteditable subtree or a text field.
 * @return {editing.TextEditHandler}
 */
editing.TextEditHandler.createForNode = function(node) {
  if (!node.state.editable)
    throw new Error('Expected editable node.');

  return new editing.TextEditHandler(node);
};

/**
 * An observer that reacts to ChromeVox range changes that modifies braille
 * table output when over email or url text fields.
 * @constructor
 * @implements {ChromeVoxStateObserver}
 */
editing.EditingChromeVoxStateObserver = function() {
  ChromeVoxState.addObserver(this);
};

editing.EditingChromeVoxStateObserver.prototype = {
  __proto__: ChromeVoxStateObserver,

  /** @override */
  onCurrentRangeChanged: function(range) {
    var inputType = range && range.start.node.inputType;
    if (inputType == 'email' || inputType == 'url') {
      cvox.BrailleBackground.getInstance().getTranslatorManager().refresh(
          localStorage['brailleTable8']);
      return;
    }
    cvox.BrailleBackground.getInstance().getTranslatorManager().refresh(
        localStorage['brailleTable']);
  }
};

/**
 * @private {ChromeVoxStateObserver}
 */
editing.observer_ = new editing.EditingChromeVoxStateObserver();

/**
 * An EditableLine encapsulates all data concerning a line in the automation
 * tree necessary to provide output.
 * Editable: an editable selection (e.g. start/end offsets) get saved.
 * Line: nodes/offsets at the beginning/end of a line get saved.
 * @param {!AutomationNode} startNode
 * @param {number} startIndex
 * @param {!AutomationNode} endNode
 * @param {number} endIndex
 * @param {boolean=} opt_baseLineOnStart Controls whether to use anchor or
 * focus for Line computations as described above. Selections are automatically
 * truncated up to either the line start or end.
 * @constructor
 */
editing.EditableLine = function(
    startNode, startIndex, endNode, endIndex, opt_baseLineOnStart) {
  /** @private {!Cursor} */
  this.start_ = new Cursor(startNode, startIndex);
  this.start_ = this.start_.deepEquivalent || this.start_;

  /** @private {!Cursor} */
  this.end_ = new Cursor(endNode, endIndex);
  this.end_ = this.end_.deepEquivalent || this.end_;
  /** @private {number} */
  this.localContainerStartOffset_ = startIndex;
  /** @private {number} */
  this.localContainerEndOffset_ = endIndex;

  // Computed members.
  /** @private {Spannable} */
  this.value_;
  /** @private {AutomationNode|undefined} */
  this.lineStart_;
  /** @private {AutomationNode|undefined} */
  this.lineEnd_;
  /** @private {AutomationNode|undefined} */
  this.startContainer_;
  /** @private {AutomationNode|undefined} */
  this.lineStartContainer_;
  /** @private {number} */
  this.localLineStartContainerOffset_ = 0;
  /** @private {AutomationNode|undefined} */
  this.lineEndContainer_;
  /** @private {number} */
  this.localLineEndContainerOffset_ = 0;
  /** @type {RecoveryStrategy} */
  this.lineStartContainerRecovery_;

  this.computeLineData_(opt_baseLineOnStart);
};

editing.EditableLine.prototype = {
  /** @private */
  computeLineData_: function(opt_baseLineOnStart) {
    // Note that we calculate the line based only upon anchor or focus even if
    // they do not fall on the same line. It is up to the caller to specify
    // which end to base this line upon since it requires reasoning about two
    // lines.
    var nameLen = 0;
    var lineBase = opt_baseLineOnStart ? this.start_ : this.end_;
    var lineExtend = opt_baseLineOnStart ? this.end_ : this.start_;

    if (lineBase.node.name)
      nameLen = lineBase.node.name.length;

    this.value_ = new Spannable(lineBase.node.name || '', lineBase);
    if (lineBase.node == lineExtend.node)
      this.value_.setSpan(lineExtend, 0, nameLen);

    this.startContainer_ = this.start_.node;
    if (this.startContainer_.role == RoleType.INLINE_TEXT_BOX)
      this.startContainer_ = this.startContainer_.parent;
    this.endContainer_ = this.end_.node;
    if (this.endContainer_.role == RoleType.INLINE_TEXT_BOX)
      this.endContainer_ = this.endContainer_.parent;

    // Initialize defaults.
    this.lineStart_ = lineBase.node;
    this.lineEnd_ = this.lineStart_;
    this.lineStartContainer_ = this.lineStart_.parent;
    this.lineEndContainer_ = this.lineStart_.parent;

    // Annotate each chunk with its associated inline text box node.
    this.value_.setSpan(this.lineStart_, 0, nameLen);

    // Also, track the nodes necessary for selection (either their parents, in
    // the case of inline text boxes, or the node itself).
    var parents = [this.startContainer_];

    // Compute the start of line.
    var lineStart = this.lineStart_;

    // Hack: note underlying bugs require these hacks.
    while ((lineStart.previousOnLine && lineStart.previousOnLine.role) ||
           (lineStart.previousSibling && lineStart.previousSibling.lastChild &&
            lineStart.previousSibling.lastChild.nextOnLine == lineStart)) {
      if (lineStart.previousOnLine)
        lineStart = lineStart.previousOnLine;
      else
        lineStart = lineStart.previousSibling.lastChild;

      this.lineStart_ = lineStart;

      if (lineStart.role != RoleType.INLINE_TEXT_BOX)
        parents.unshift(lineStart);
      else if (parents[0] != lineStart.parent)
        parents.unshift(lineStart.parent);

      var prepend = new Spannable(lineStart.name, lineStart);
      prepend.append(this.value_);
      this.value_ = prepend;
    }
    this.lineStartContainer_ = this.lineStart_.parent;

    var lineEnd = this.lineEnd_;

    // Hack: note underlying bugs require these hacks.
    while ((lineEnd.nextOnLine && lineEnd.nextOnLine.role) ||
           (lineEnd.nextSibling &&
            lineEnd.nextSibling.previousOnLine == lineEnd)) {
      if (lineEnd.nextOnLine)
        lineEnd = lineEnd.nextOnLine;
      else
        lineEnd = lineEnd.nextSibling.firstChild;

      this.lineEnd_ = lineEnd;

      if (lineEnd.role != RoleType.INLINE_TEXT_BOX)
        parents.push(this.lineEnd_);
      else if (parents[parents.length - 1] != lineEnd.parent)
        parents.push(this.lineEnd_.parent);

      var annotation = lineEnd;
      if (lineEnd == this.end_.node)
        annotation = this.end_;

      this.value_.append(new Spannable(lineEnd.name, annotation));
    }
    this.lineEndContainer_ = this.lineEnd_.parent;

    // Finally, annotate with all parent static texts as NodeSpan's so that
    // braille routing can key properly into the node with an offset.
    // Note that both line start and end needs to account for
    // potential offsets into the static texts as follows.
    var textCountBeforeLineStart = 0, textCountAfterLineEnd = 0;
    var finder = this.lineStart_;
    while (finder.previousSibling) {
      finder = finder.previousSibling;
      textCountBeforeLineStart += finder.name ? finder.name.length : 0;
    }
    this.localLineStartContainerOffset_ = textCountBeforeLineStart;

    if (this.lineStartContainer_) {
      this.lineStartContainerRecovery_ =
          new TreePathRecoveryStrategy(this.lineStartContainer_);
    }

    finder = this.lineEnd_;
    while (finder.nextSibling) {
      finder = finder.nextSibling;
      textCountAfterLineEnd += finder.name ? finder.name.length : 0;
    }

    if (this.lineEndContainer_.name) {
      this.localLineEndContainerOffset_ =
          this.lineEndContainer_.name.length - textCountAfterLineEnd;
    }

    var len = 0;
    for (var i = 0; i < parents.length; i++) {
      var parent = parents[i];

      if (!parent.name)
        continue;

      var prevLen = len;

      var currentLen = parent.name.length;
      var offset = 0;

      // Subtract off the text count before when at the start of line.
      if (i == 0) {
        currentLen -= textCountBeforeLineStart;
        offset = textCountBeforeLineStart;
      }

      // Subtract text count after when at the end of the line.
      if (i == parents.length - 1)
        currentLen -= textCountAfterLineEnd;

      len += currentLen;

      try {
        this.value_.setSpan(new Output.NodeSpan(parent, offset), prevLen, len);

        // Also, annotate this span if it is associated with line containre.
        if (parent == this.startContainer_)
          this.value_.setSpan(parent, prevLen, len);
      } catch (e) {
      }
    }
  },

  /**
   * Gets the selection offset based on the text content of this line.
   * @return {number}
   */
  get startOffset() {
    // It is possible that the start cursor points to content before this line
    // (e.g. in a multi-line selection).
    try {
      return this.value_.getSpanStart(this.start_) + this.start_.index;
    } catch (e) {
      // When that happens, fall back to the start of this line.
      return 0;
    }
  },

  /**
   * Gets the selection offset based on the text content of this line.
   * @return {number}
   */
  get endOffset() {
    try {
      return this.value_.getSpanStart(this.end_) + this.end_.index;
    } catch (e) {
      return this.value_.length;
    }
  },

  /**
   * Gets the selection offset based on the parent's text.
   * The parent is expected to be static text.
   * @return {number}
   */
  get localStartOffset() {
    return this.localContainerStartOffset_;
  },

  /**
   * Gets the selection offset based on the parent's text.
   * The parent is expected to be static text.
   * @return {number}
   */
  get localEndOffset() {
    return this.localContainerEndOffset_;
  },

  /**
   * Gets the start offset of the container, relative to the line text content.
   * The container refers to the static text parenting the inline text box.
   * @return {number}
   */
  get containerStartOffset() {
    return this.value_.getSpanStart(this.startContainer_);
  },

  /**
   * Gets the end offset of the container, relative to the line text content.
   * The container refers to the static text parenting the inline text box.
   * @return {number}
   */
  get containerEndOffset() {
    return this.value_.getSpanEnd(this.startContainer_) - 1;
  },

  /**
   * The text content of this line.
   * @return {string} The text of this line.
   */
  get text() {
    return this.value_.toString();
  },

  /** @return {string} */
  get selectedText() {
    return this.value_.toString().substring(this.startOffset, this.endOffset);
  },

  /** @return {boolean} */
  hasCollapsedSelection: function() {
    return this.start_.equals(this.end_);
  },

  /**
   * Returns true if |otherLine| surrounds the same line as |this|. Note that
   * the contents of the line might be different.
   * @param {editing.EditableLine} otherLine
   * @return {boolean}
   */
  isSameLine: function(otherLine) {
    // Equality is intentionally loose here as any of the state nodes can be
    // invalidated at any time. We rely upon the start/anchor of the line
    // staying the same.
    return (otherLine.lineStartContainer_ == this.lineStartContainer_ &&
            otherLine.localLineStartContainerOffset_ ==
                this.localLineStartContainerOffset_) ||
        (otherLine.lineEndContainer_ == this.lineEndContainer_ &&
         otherLine.localLineEndContainerOffset_ ==
             this.localLineEndContainerOffset_) ||
        (otherLine.lineStartContainerRecovery_.node ==
             this.lineStartContainerRecovery_.node &&
         otherLine.localLineStartContainerOffset_ ==
             this.localLineStartContainerOffset_);
  },

  /**
   * Returns true if |otherLine| surrounds the same line as |this| and has the
   * same selection.
   * @param {editing.EditableLine} otherLine
   * @return {boolean}
   */
  isSameLineAndSelection: function(otherLine) {
    return this.isSameLine(otherLine) &&
        this.startOffset == otherLine.startOffset &&
        this.endOffset == otherLine.endOffset;
  },

  /**
   * Returns whether this line comes before |otherLine| in document order.
   * @return {boolean}
   */
  isBeforeLine: function(otherLine) {
    if (this.isSameLine(otherLine) || !this.lineStartContainer_ ||
        !otherLine.lineStartContainer_)
      return false;
    return AutomationUtil.getDirection(
               this.lineStartContainer_, otherLine.lineStartContainer_) ==
        Dir.FORWARD;
  },

  /**
   * Performs a validation that this line still refers to a line given its
   * internally tracked state.
   */
  isValidLine: function() {
    if (!this.lineStartContainer_ || !this.lineEndContainer_)
      return false;

    var start = new cursors.Cursor(
        this.lineStartContainer_, this.localLineStartContainerOffset_);
    var end = new cursors.Cursor(
        this.lineEndContainer_, this.localLineEndContainerOffset_ - 1);
    var localStart = start.deepEquivalent || start;
    var localEnd = end.deepEquivalent || end;
    var localStartNode = localStart.node;
    var localEndNode = localEnd.node;

    // Unfortunately, there are asymmetric errors in lines, so we need to check
    // in both directions.
    var testStartNode = localStartNode;
    do {
      if (testStartNode == localEndNode)
        return true;

      // Hack/workaround for broken *OnLine links.
      if (testStartNode.nextOnLine && testStartNode.nextOnLine.role)
        testStartNode = testStartNode.nextOnLine;
      else if (
          testStartNode.nextSibling &&
          testStartNode.nextSibling.previousOnLine == testStartNode)
        testStartNode = testStartNode.nextSibling;
      else
        break;
    } while (testStartNode);

    var testEndNode = localEndNode;
    do {
      if (testEndNode == localStartNode)
        return true;

      // Hack/workaround for broken *OnLine links.
      if (testEndNode.previousOnLine && testEndNode.previousOnLine.role)
        testEndNode = testEndNode.previousOnLine;
      else if (
          testEndNode.previousSibling &&
          testEndNode.previousSibling.nextOnLine == testEndNode)
        testEndNode = testEndNode.previousSibling;
      else
        break;
    } while (testEndNode);

    return false;
  }
};

});
