Polymer({

    is: 'iron-collapse',

    behaviors: [
      Polymer.IronResizableBehavior
    ],

    properties: {

      /**
       * If true, the orientation is horizontal; otherwise is vertical.
       *
       * @attribute horizontal
       */
      horizontal: {
        type: Boolean,
        value: false,
        observer: '_horizontalChanged'
      },

      /**
       * Set opened to true to show the collapse element and to false to hide it.
       *
       * @attribute opened
       */
      opened: {
        type: Boolean,
        value: false,
        notify: true,
        observer: '_openedChanged'
      },

      /**
       * When true, the element is transitioning its opened state. When false,
       * the element has finished opening/closing.
       *
       * @attribute transitioning
       */
      transitioning: {
        type: Boolean,
        notify: true,
        readOnly: true
      },

      /**
       * Set noAnimation to true to disable animations.
       *
       * @attribute noAnimation
       */
      noAnimation: {
        type: Boolean
      },

      /**
       * Stores the desired size of the collapse body.
       * @private
       */
      _desiredSize: {
        type: String,
        value: ''
      }
    },

    get dimension() {
      return this.horizontal ? 'width' : 'height';
    },

    /**
     * `maxWidth` or `maxHeight`.
     * @private
     */
    get _dimensionMax() {
      return this.horizontal ? 'maxWidth' : 'maxHeight';
    },

    /**
     * `max-width` or `max-height`.
     * @private
     */
    get _dimensionMaxCss() {
      return this.horizontal ? 'max-width' : 'max-height';
    },

    hostAttributes: {
      role: 'group',
      'aria-hidden': 'true',
    },

    listeners: {
      transitionend: '_onTransitionEnd'
    },

    /**
     * Toggle the opened state.
     *
     * @method toggle
     */
    toggle: function() {
      this.opened = !this.opened;
    },

    show: function() {
      this.opened = true;
    },

    hide: function() {
      this.opened = false;
    },

    /**
     * Updates the size of the element.
     * @param {string} size The new value for `maxWidth`/`maxHeight` as css property value, usually `auto` or `0px`.
     * @param {boolean=} animated if `true` updates the size with an animation, otherwise without.
     */
    updateSize: function(size, animated) {
      // Consider 'auto' as '', to take full size.
      size = size === 'auto' ? '' : size;

      var willAnimate = animated && !this.noAnimation &&
                        this.isAttached && this._desiredSize !== size;

      this._desiredSize = size;

      this._updateTransition(false);
      // If we can animate, must do some prep work.
      if (willAnimate) {
        // Animation will start at the current size.
        var startSize = this._calcSize();
        // For `auto` we must calculate what is the final size for the animation.
        // After the transition is done, _transitionEnd will set the size back to `auto`.
        if (size === '') {
          this.style[this._dimensionMax] = '';
          size = this._calcSize();
        }
        // Go to startSize without animation.
        this.style[this._dimensionMax] = startSize;
        // Force layout to ensure transition will go. Set scrollTop to itself
        // so that compilers won't remove it.
        this.scrollTop = this.scrollTop;
        // Enable animation.
        this._updateTransition(true);
        // If final size is the same as startSize it will not animate.
        willAnimate = (size !== startSize);
      }
      // Set the final size.
      this.style[this._dimensionMax] = size;
      // If it won't animate, call transitionEnd to set correct classes.
      if (!willAnimate) {
        this._transitionEnd();
      }
    },

    /**
     * enableTransition() is deprecated, but left over so it doesn't break existing code.
     * Please use `noAnimation` property instead.
     *
     * @method enableTransition
     * @deprecated since version 1.0.4
     */
    enableTransition: function(enabled) {
      Polymer.Base._warn('`enableTransition()` is deprecated, use `noAnimation` instead.');
      this.noAnimation = !enabled;
    },

    _updateTransition: function(enabled) {
      this.style.transitionDuration = (enabled && !this.noAnimation) ? '' : '0s';
    },

    _horizontalChanged: function() {
      this.style.transitionProperty = this._dimensionMaxCss;
      var otherDimension = this._dimensionMax === 'maxWidth' ? 'maxHeight' : 'maxWidth';
      this.style[otherDimension] = '';
      this.updateSize(this.opened ? 'auto' : '0px', false);
    },

    _openedChanged: function() {
      this.setAttribute('aria-hidden', !this.opened);

      this._setTransitioning(true);
      this.toggleClass('iron-collapse-closed', false);
      this.toggleClass('iron-collapse-opened', false);
      this.updateSize(this.opened ? 'auto' : '0px', true);

      // Focus the current collapse.
      if (this.opened) {
        this.focus();
      }
    },

    _transitionEnd: function() {
      this.style[this._dimensionMax] = this._desiredSize;
      this.toggleClass('iron-collapse-closed', !this.opened);
      this.toggleClass('iron-collapse-opened', this.opened);
      this._updateTransition(false);
      this.notifyResize();
      this._setTransitioning(false);
    },

    _onTransitionEnd: function(event) {
      if (Polymer.dom(event).rootTarget === this) {
        this._transitionEnd();
      }
    },

    _calcSize: function() {
      return this.getBoundingClientRect()[this.dimension] + 'px';
    }

  });