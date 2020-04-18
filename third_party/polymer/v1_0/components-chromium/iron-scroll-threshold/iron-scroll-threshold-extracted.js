Polymer({

    is: 'iron-scroll-threshold',

    properties: {

      /**
       * Distance from the top (or left, for horizontal) bound of the scroller
       * where the "upper trigger" will fire.
       */
      upperThreshold: {
        type: Number,
        value: 100
      },

      /**
       * Distance from the bottom (or right, for horizontal) bound of the scroller
       * where the "lower trigger" will fire.
       */
      lowerThreshold: {
        type: Number,
        value: 100
      },

      /**
       * Read-only value that tracks the triggered state of the upper threshold.
       */
      upperTriggered: {
        type: Boolean,
        value: false,
        notify: true,
        readOnly: true
      },

      /**
       * Read-only value that tracks the triggered state of the lower threshold.
       */
      lowerTriggered: {
        type: Boolean,
        value: false,
        notify: true,
        readOnly: true
      },

      /**
       * True if the orientation of the scroller is horizontal.
       */
      horizontal: {
        type: Boolean,
        value: false
      }
    },

    behaviors: [
      Polymer.IronScrollTargetBehavior
    ],

    observers: [
      '_setOverflow(scrollTarget)',
      '_initCheck(horizontal, isAttached)'
    ],

    get _defaultScrollTarget() {
      return this;
    },

    _setOverflow: function(scrollTarget) {
      this.style.overflow = scrollTarget === this ? 'auto' : '';
      this.style.webkitOverflowScrolling = scrollTarget === this ? 'touch' : '';
    },

    _scrollHandler: function() {
      // throttle the work on the scroll event
      var THROTTLE_THRESHOLD = 200;
      if (!this.isDebouncerActive('_checkTheshold')) {
        this.debounce('_checkTheshold', function() {
          this.checkScrollThresholds();
        }, THROTTLE_THRESHOLD);
      }
    },

    _initCheck: function(horizontal, isAttached) {
      if (isAttached) {
        this.debounce('_init', function() {
          this.clearTriggers();
          this.checkScrollThresholds();
        });
      }
    },

    /**
     * Checks the scroll thresholds.
     * This method is automatically called by iron-scroll-threshold.
     *
     * @method checkScrollThresholds
     */
    checkScrollThresholds: function() {
      if (!this.scrollTarget || (this.lowerTriggered && this.upperTriggered)) {
        return;
      }
      var upperScrollValue = this.horizontal ? this._scrollLeft : this._scrollTop;
      var lowerScrollValue = this.horizontal ?
          this.scrollTarget.scrollWidth - this._scrollTargetWidth - this._scrollLeft :
              this.scrollTarget.scrollHeight - this._scrollTargetHeight - this._scrollTop;

      // Detect upper threshold
      if (upperScrollValue <= this.upperThreshold && !this.upperTriggered) {
        this._setUpperTriggered(true);
        this.fire('upper-threshold');
      }
      // Detect lower threshold
      if (lowerScrollValue <= this.lowerThreshold && !this.lowerTriggered) {
        this._setLowerTriggered(true);
        this.fire('lower-threshold');
      }
    },

    checkScrollThesholds: function() {
      // iron-scroll-threshold/issues/16
      this.checkScrollThresholds();
    },

    /**
     * Clear the upper and lower threshold states.
     *
     * @method clearTriggers
     */
    clearTriggers: function() {
      this._setUpperTriggered(false);
      this._setLowerTriggered(false);
    }

    /**
     * Fires when the lower threshold has been reached.
     *
     * @event lower-threshold
     */

    /**
     * Fires when the upper threshold has been reached.
     *
     * @event upper-threshold
     */

  });