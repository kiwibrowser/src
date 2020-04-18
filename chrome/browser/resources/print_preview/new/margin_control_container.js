// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

/** @const {number} */
const MINIMUM_DISTANCE = 72;  // 1 inch

/**
 * @const {!Map<!print_preview.ticket_items.CustomMarginsOrientation, string>}
 */
const MARGIN_KEY_MAP = new Map([
  [print_preview.ticket_items.CustomMarginsOrientation.TOP, 'marginTop'],
  [print_preview.ticket_items.CustomMarginsOrientation.RIGHT, 'marginRight'],
  [print_preview.ticket_items.CustomMarginsOrientation.BOTTOM, 'marginBottom'],
  [print_preview.ticket_items.CustomMarginsOrientation.LEFT, 'marginLeft']
]);

Polymer({
  is: 'print-preview-margin-control-container',

  behaviors: [SettingsBehavior],

  properties: {
    /** @type {!print_preview.Size} */
    pageSize: {
      type: Object,
      notify: true,
    },

    /** @type {!print_preview.Margins} */
    documentMargins: {
      type: Object,
      notify: true,
    },

    previewLoaded: Boolean,

    /** @type {?print_preview.MeasurementSystem} */
    measurementSystem: Object,

    /** @private {number} */
    scaleTransform_: {
      type: Number,
      notify: true,
      value: 0,
    },

    /** @private {!print_preview.Coordinate2d} */
    translateTransform_: {
      type: Object,
      notify: true,
      value: new print_preview.Coordinate2d(0, 0),
    },

    /** @private {?print_preview.Size} */
    clipSize_: {
      type: Object,
      notify: true,
      value: null,
    },

    /** @private {boolean} */
    available_: {
      type: Boolean,
      notify: true,
      computed: 'computeAvailable_(previewLoaded, settings.margins.value)',
      observer: 'onAvailableChange_',
    },

    /** @private {boolean} */
    invisible_: {
      type: Boolean,
      reflectToAttribute: true,
      value: true,
    },

    /**
     * @private {!Array<!print_preview.ticket_items.CustomMarginsOrientation>}
     */
    marginSides_: {
      type: Array,
      notify: true,
      value: [
        print_preview.ticket_items.CustomMarginsOrientation.TOP,
        print_preview.ticket_items.CustomMarginsOrientation.RIGHT,
        print_preview.ticket_items.CustomMarginsOrientation.BOTTOM,
        print_preview.ticket_items.CustomMarginsOrientation.LEFT,
      ],
    },

    /**
     * String attribute used to set cursor appearance. Possible values:
     * empty (''): No margin control is currently being dragged.
     * 'dragging-horizontal': The left or right control is being dragged.
     * 'dragging-vertical': The top or bottom control is being dragged.
     * @private {string}
     */
    dragging_: {
      type: String,
      reflectToAttribute: true,
      value: '',
    },
  },

  observers: [
    'onMarginSettingsChange_(settings.customMargins.value)',
  ],

  /** @private {!print_preview.Coordinate2d} */
  pointerStartPositionInPixels_: new print_preview.Coordinate2d(0, 0),

  /** @private {?print_preview.Coordinate2d} */
  marginStartPositionInPixels_: null,

  /**
   * @return {boolean}
   * @private
   */
  computeAvailable_: function() {
    return this.previewLoaded && !!this.clipSize_ &&
        this.getSettingValue('margins') ==
        print_preview.ticket_items.MarginsTypeValue.CUSTOM &&
        !!this.pageSize;
  },

  /** @private */
  onAvailableChange_: function() {
    if (this.available_ && !!this.documentMargins) {
      const newMargins = {};
      // Track whether the margins have actually changed to avoid triggering the
      // setting change if they are the same.
      const oldMargins = this.getSettingValue('customMargins');
      let change = false;
      for (let side of Object.values(
               print_preview.ticket_items.CustomMarginsOrientation)) {
        const key = MARGIN_KEY_MAP.get(side);
        newMargins[key] = this.documentMargins.get(side);
        change = change || (newMargins[key] != oldMargins[key]);
      }
      if (change)
        this.setSetting('customMargins', newMargins);
    }
    this.invisible_ = !this.available_;
  },

  /** @private */
  onMarginSettingsChange_: function() {
    const margins = this.getSettingValue('customMargins');
    this.shadowRoot.querySelectorAll('print-preview-margin-control')
        .forEach(control => {
          const key = MARGIN_KEY_MAP.get(control.side);
          const newValue = margins[key] || 0;
          control.setPositionInPts(newValue);
          control.setTextboxValue(this.serializeValueFromPts_(newValue));
        });
  },

  /**
   * @param {!print_preview.ticket_items.CustomMarginsOrientation} orientation
   *     Orientation value to test.
   * @return {boolean} Whether the given orientation is TOP or BOTTOM.
   * @private
   */
  isTopOrBottom_: function(orientation) {
    return orientation ==
        print_preview.ticket_items.CustomMarginsOrientation.TOP ||
        orientation ==
        print_preview.ticket_items.CustomMarginsOrientation.BOTTOM;
  },

  /**
   * @param {!HTMLElement} control Control being repositioned.
   * @param {!print_preview.Coordinate2d} posInPixels Desired position, in
   *     pixels.
   * @return {number} The new position for the control, in pts. Returns the
   *     position for the dimension that the control operates in, i.e.
   *     x direction for the left/right controls, y direction otherwise.
   * @private
   */
  posInPixelsToPts_: function(control, posInPixels) {
    const side =
        /** @type {print_preview.ticket_items.CustomMarginsOrientation} */ (
            control.side);
    return this.clipAndRoundValue_(
        side,
        control.convertPixelsToPts(
            this.isTopOrBottom_(side) ? posInPixels.y : posInPixels.x));
  },

  /**
   * Moves the position of the given control to the desired position in pts
   * within some constraint minimum and maximum.
   * @param {!HTMLElement} control Control to move.
   * @param {number} posInPts Desired position to move to, in pts. Position is
   *     1 dimensional and represents position in the x direction if control is
   *     for the left or right margin, and the y direction otherwise.
   * @private
   */
  moveControlWithConstraints_: function(control, posInPts) {
    control.setPositionInPts(posInPts);
    control.setTextboxValue(this.serializeValueFromPts_(posInPts));
  },

  /**
   * @param {number} value Value in points to serialize.
   * @return {string} String representation of the value in the system's local
   *     units.
   * @private
   */
  serializeValueFromPts_: function(value) {
    assert(this.measurementSystem);
    value = this.measurementSystem.convertFromPoints(value);
    value = this.measurementSystem.roundValue(value);
    return value + this.measurementSystem.unitSymbol;
  },

  /**
   * Translates the position of the margin control relative to the pointer
   * position in pixels.
   * @param {!print_preview.Coordinate2d} pointerPosition New position of
   *     the pointer.
   * @return {!print_preview.Coordinate2d} New position of the margin control.
   */
  translatePointerToPositionInPixels: function(pointerPosition) {
    return new print_preview.Coordinate2d(
        pointerPosition.x - this.pointerStartPositionInPixels_.x +
            this.marginStartPositionInPixels_.x,
        pointerPosition.y - this.pointerStartPositionInPixels_.y +
            this.marginStartPositionInPixels_.y);
  },

  /**
   * Called when the pointer moves in the custom margins component. Moves the
   * dragged margin control.
   * @param {!PointerEvent} event Contains the position of the pointer.
   * @private
   */
  onPointerMove_: function(event) {
    const control =
        /** @type {!PrintPreviewMarginControlElement} */ (event.target);
    const posInPts = this.posInPixelsToPts_(
        control,
        this.translatePointerToPositionInPixels(
            new print_preview.Coordinate2d(event.x, event.y)));
    this.moveControlWithConstraints_(control, posInPts);
  },

  /**
   * Called when the pointer is released in the custom margins component.
   * Releases the dragged margin control.
   * @param {!PointerEvent} event Contains the position of the pointer.
   * @private
   */
  onPointerUp_: function(event) {
    const control =
        /** @type {!PrintPreviewMarginControlElement} */ (event.target);
    this.dragging_ = '';
    const posInPixels = this.translatePointerToPositionInPixels(
        new print_preview.Coordinate2d(event.x, event.y));
    const posInPts = this.posInPixelsToPts_(control, posInPixels);
    this.moveControlWithConstraints_(control, posInPts);
    this.setMargin_(control.side, posInPts);
    this.updateClippingMask(this.clipSize_);
    this.unlisten(control, 'pointercancel', 'onPointerUp_');
    this.unlisten(control, 'pointerup', 'onPointerUp_');
    this.unlisten(control, 'pointermove', 'onPointerMove_');

    this.fire('margin-drag-changed', false);
  },

  /**
   * @param {boolean} invisible Whether the margin controls should be
   *     invisible.
   */
  setInvisible: function(invisible) {
    // Ignore changes if the margin controls are not available.
    if (!this.available_)
      return;

    // Do not set the controls invisible if the user is dragging one of them.
    if (invisible && this.dragging_ != '')
      return;

    this.invisible_ = invisible;
  },

  /**
   * @param {string} side The margin side. Must be a CustomMarginsOrientation.
   * @param {number} marginValue New value for the margin in points.
   * @private
   */
  setMargin_: function(side, marginValue) {
    const marginSide =
        /** @type {!print_preview.ticket_items.CustomMarginsOrientation} */ (
            side);
    const oldMargins = /** @type {print_preview.MarginsSetting} */ (
        this.getSettingValue('customMargins'));
    const key = MARGIN_KEY_MAP.get(marginSide);
    if (oldMargins[key] == marginValue)
      return;
    const newMargins = Object.assign({}, oldMargins);
    newMargins[key] = marginValue;
    this.setSetting('customMargins', newMargins);
  },

  /**
   * @param {string} side The margin side. Must be a CustomMarginsOrientation.
   * @param {number} value The new margin value in points.
   * @return {number} The clipped margin value in points.
   * @private
   */
  clipAndRoundValue_: function(side, value) {
    const marginSide =
        /** @type {!print_preview.ticket_items.CustomMarginsOrientation} */ (
            side);
    if (value < 0)
      return 0;
    const Orientation = print_preview.ticket_items.CustomMarginsOrientation;
    let limit = 0;
    const margins = this.getSettingValue('customMargins');
    if (marginSide == Orientation.TOP)
      limit = this.pageSize.height - margins.marginBottom - MINIMUM_DISTANCE;
    else if (marginSide == Orientation.RIGHT)
      limit = this.pageSize.width - margins.marginLeft - MINIMUM_DISTANCE;
    else if (marginSide == Orientation.BOTTOM)
      limit = this.pageSize.height - margins.marginTop - MINIMUM_DISTANCE;
    else {
      assert(marginSide == Orientation.LEFT);
      limit = this.pageSize.width - margins.marginRight - MINIMUM_DISTANCE;
    }
    return Math.round(Math.min(value, limit));
  },

  /**
   * @param {!CustomEvent} e Event containing the new textbox value.
   * @private
   */
  onTextChange_: function(e) {
    const marginValue = this.parseValueToPts_(/** @type {string} */ (e.detail));
    const control =
        /** @type {!PrintPreviewMarginControlElement} */ (e.target);
    if (marginValue == null) {
      control.invalid = true;
      return;
    }
    control.invalid = false;
    const clippedValue = this.clipAndRoundValue_(control.side, marginValue);
    control.setPositionInPts(clippedValue);
    this.setMargin_(control.side, clippedValue);
  },

  /**
   * @param {!CustomEvent} e Event fired when a control with an invalid value's
   *     text field is blurred.
   * @private
   */
  onTextBlur_: function(e) {
    const control =
        /** @type {!PrintPreviewMarginControlElement} */ (e.target);
    control.setTextboxValue(
        this.serializeValueFromPts_(control.getPositionInPts()));
    control.invalid = false;
  },

  /**
   * @param {!PointerEvent} e Fired when pointerdown occurs on a margin control.
   * @private
   */
  onPointerDown_: function(e) {
    const control =
        /** @type {!PrintPreviewMarginControlElement} */ (e.target);
    if (!control.shouldDrag(e))
      return;

    this.pointerStartPositionInPixels_ =
        new print_preview.Coordinate2d(e.x, e.y);
    this.marginStartPositionInPixels_ =
        new print_preview.Coordinate2d(control.offsetLeft, control.offsetTop);
    this.dragging_ =
        this.isTopOrBottom_(
            /** @type {print_preview.ticket_items.CustomMarginsOrientation} */ (
                control.side)) ?
        'dragging-vertical' :
        'dragging-horizontal';
    this.listen(control, 'pointercancel', 'onPointerUp_');
    this.listen(control, 'pointerup', 'onPointerUp_');
    this.listen(control, 'pointermove', 'onPointerMove_');
    control.setPointerCapture(e.pointerId);

    this.fire('margin-drag-changed', true);
  },

  /**
   * Set display:none after the opacity transition for the controls is done.
   * @private
   */
  onTransitionEnd_: function() {
    if (this.invisible_)
      this.style.display = 'none';
  },

  /**
   * @param {string} value Value to parse to points. E.g. '3.40"' or '200mm'.
   * @return {?number} Value in points represented by the input value.
   * @private
   */
  parseValueToPts_: function(value) {
    // Removing whitespace anywhere in the string.
    value = value.replace(/\s*/g, '');
    if (value.length == 0) {
      return null;
    }
    assert(this.measurementSystem);
    const validationRegex = new RegExp(
        '^(^-?)(\\d)+(\\' + this.measurementSystem.thousandsDelimeter +
        '\\d{3})*(\\' + this.measurementSystem.decimalDelimeter + '\\d*)?' +
        '(' + this.measurementSystem.unitSymbol + ')?$');
    if (validationRegex.test(value)) {
      // Replacing decimal point with the dot symbol in order to use
      // parseFloat() properly.
      const replacementRegex =
          new RegExp('\\' + this.measurementSystem.decimalDelimeter + '{1}');
      value = value.replace(replacementRegex, '.');
      return this.measurementSystem.convertToPoints(parseFloat(value));
    }
    return null;
  },

  /**
   * Updates the translation transformation that translates pixel values in
   * the space of the HTML DOM.
   * @param {print_preview.Coordinate2d} translateTransform Updated value of
   *     the translation transformation.
   */
  updateTranslationTransform: function(translateTransform) {
    if (!translateTransform.equals(this.translateTransform_)) {
      this.translateTransform_ = translateTransform;
    }
  },

  /**
   * Updates the scaling transform that scales pixels values to point values.
   * @param {number} scaleTransform Updated value of the scale transform.
   */
  updateScaleTransform: function(scaleTransform) {
    if (scaleTransform != this.scaleTransform_)
      this.scaleTransform_ = scaleTransform;
  },

  /**
   * Clips margin controls to the given clip size in pixels.
   * @param {print_preview.Size} clipSize Size to clip the margin controls to.
   */
  updateClippingMask: function(clipSize) {
    if (!clipSize) {
      return;
    }
    this.clipSize_ = clipSize;
    this.notifyPath('clipSize_');
  },
});
})();
