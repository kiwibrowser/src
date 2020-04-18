// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This Polymer element is used as a header for the media router interface.
Polymer({
  is: 'media-router-header',

  properties: {
    /**
     * The name of the icon used as the back button. This is set once, when
     * the |this| is ready.
     * @private {string|undefined}
     */
    arrowDropIcon_: {
      type: String,
    },

    /**
     * Whether or not the arrow drop icon should be disabled.
     * @type {boolean}
     */
    arrowDropIconDisabled: {
      type: Boolean,
      value: false,
    },

    /**
     * The header text to show.
     * @type {string|undefined}
     */
    headingText: {
      type: String,
    },

    /**
     * The height of the header when it shows the user email.
     * @private {number}
     */
    headerWithEmailHeight_: {
      type: Number,
      readOnly: true,
      value: 62,
    },

    /**
     * The height of the header when it doesn't show the user email.
     * @private {number}
     */
    headerWithoutEmailHeight_: {
      type: Number,
      readOnly: true,
      value: 52,
    },

    /**
     * Whether to show the user email in the header.
     * @type {boolean|undefined}
     */
    showEmail: {
      type: Boolean,
      observer: 'maybeChangeHeaderHeight_',
    },

    /**
     * The text to show in the tooltip.
     * @type {string|undefined}
     */
    tooltip: {
      type: String,
    },

    /**
     * The user email if they are signed in.
     * @type {string|undefined}
     */
    userEmail: {
      type: String,
    },

    /**
     * The current view that this header should reflect.
     * @type {?media_router.MediaRouterView|undefined}
     */
    view: {
      type: String,
      observer: 'updateHeaderCursorStyle_',
    },
  },

  behaviors: [
    I18nBehavior,
  ],

  ready: function() {
    this.$$('#header').style.height = this.headerWithoutEmailHeight_ + 'px';
  },

  attached: function() {
    // isRTL() only works after i18n_template.js runs to set <html dir>.
    // Set the back button icon based on text direction.
    this.arrowDropIcon_ =
        isRTL() ? 'media-router:arrow-forward' : 'media-router:arrow-back';
  },

  /**
   * @param {?media_router.MediaRouterView} view The current view.
   * @return {string} The icon to use.
   * @private
   */
  computeArrowDropIcon_: function(view) {
    return view == media_router.MediaRouterView.CAST_MODE_LIST ?
        'media-router:arrow-drop-up' :
        'media-router:arrow-drop-down';
  },

  /**
   * @param {?media_router.MediaRouterView} view The current view.
   * @return {boolean} Whether or not the arrow drop icon should be hidden.
   * @private
   */
  computeArrowDropIconHidden_: function(view) {
    return view != media_router.MediaRouterView.SINK_LIST &&
        view != media_router.MediaRouterView.CAST_MODE_LIST;
  },

  /**
   * @param {?media_router.MediaRouterView} view The current view.
   * @return {string} The title text for the arrow drop button.
   * @private
   */
  computeArrowDropTitle_: function(view) {
    return view == media_router.MediaRouterView.CAST_MODE_LIST ?
        this.i18n('viewDeviceListButtonTitle') :
        this.i18n('viewCastModeListButtonTitle');
  },

  /**
   * @param {?media_router.MediaRouterView} view The current view.
   * @return {boolean} Whether or not the back button should be shown.
   * @private
   */
  computeBackButtonShown_: function(view) {
    return view == media_router.MediaRouterView.ROUTE_DETAILS ||
        view == media_router.MediaRouterView.FILTER;
  },

  /**
   * Returns whether given string is undefined, null, empty, or whitespace only.
   * @param {?string} str String to be tested.
   * @return {boolean} |true| if the string is undefined, null, empty, or
   *     whitespace.
   * @private
   */
  isEmptyOrWhitespace_: function(str) {
    return str === undefined || str === null || (/^\s*$/).test(str);
  },

  /**
   * Handles a click on the back button by firing a back-click event.
   *
   * @private
   */
  onBackButtonClick_: function() {
    this.fire('back-click');
  },

  /**
   * Handles a click on the close button by firing a close-button-click event.
   *
   * @private
   */
  onCloseButtonClick_: function() {
    this.fire('close-dialog', {
      pressEscToClose: false,
    });
  },

  /**
   * Handles a click on the arrow button by firing an arrow-click event.
   *
   * @private
   */
  onHeaderOrArrowClick_: function() {
    if (this.view == media_router.MediaRouterView.SINK_LIST ||
        this.view == media_router.MediaRouterView.CAST_MODE_LIST) {
      this.fire('header-or-arrow-click');
    }
  },

  /**
   * Updates header height to accomodate email text. This is called on changes
   * to |showEmail| and will return early if the value has not changed.
   *
   * @param {boolean} newValue The new value of |showEmail|.
   * @param {boolean} oldValue The previous value of |showEmail|.
   * @private
   */
  maybeChangeHeaderHeight_: function(newValue, oldValue) {
    if (oldValue == newValue)
      return;

    // Ensures conditional templates are stamped.
    this.async(function() {
      var currentHeight = this.offsetHeight;

      this.$$('#header').style.height =
          this.showEmail && !this.isEmptyOrWhitespace_(this.userEmail) ?
          this.headerWithEmailHeight_ + 'px' :
          this.headerWithoutEmailHeight_ + 'px';

      // Only fire if height actually changed.
      if (currentHeight != this.offsetHeight) {
        this.fire('header-height-changed');
      }
    });
  },

  /**
   * Updates the cursor style for the header text when the view changes. When
   * the drop arrow is also shown, the header text is also clickable.
   *
   * @param {?media_router.MediaRouterView} view The current view.
   * @private
   */
  updateHeaderCursorStyle_: function(view) {
    this.$$('#header-text').style.cursor =
        view == media_router.MediaRouterView.SINK_LIST ||
            view == media_router.MediaRouterView.CAST_MODE_LIST ?
        'pointer' :
        'auto';
  },
});
