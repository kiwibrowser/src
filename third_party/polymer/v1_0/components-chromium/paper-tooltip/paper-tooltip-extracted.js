Polymer({
      is: 'paper-tooltip',

      hostAttributes: {
        role: 'tooltip',
        tabindex: -1
      },

      behaviors: [
        Polymer.NeonAnimationRunnerBehavior
      ],

      properties: {
        /**
         * The id of the element that the tooltip is anchored to. This element
         * must be a sibling of the tooltip.
         */
        for: {
          type: String,
          observer: '_findTarget'
        },

        /**
         * Set this to true if you want to manually control when the tooltip
         * is shown or hidden.
         */
        manualMode: {
          type: Boolean,
          value: false,
          observer: '_manualModeChanged'
        },

        /**
         * Positions the tooltip to the top, right, bottom, left of its content.
         */
        position: {
          type: String,
          value: 'bottom'
        },

        /**
         * If true, no parts of the tooltip will ever be shown offscreen.
         */
        fitToVisibleBounds: {
          type: Boolean,
          value: false
        },

        /**
         * The spacing between the top of the tooltip and the element it is
         * anchored to.
         */
        offset: {
          type: Number,
          value: 14
        },

        /**
         * This property is deprecated, but left over so that it doesn't
         * break exiting code. Please use `offset` instead. If both `offset` and
         * `marginTop` are provided, `marginTop` will be ignored.
         * @deprecated since version 1.0.3
         */
        marginTop: {
          type: Number,
          value: 14
        },

        /**
         * The delay that will be applied before the `entry` animation is
         * played when showing the tooltip.
         */
        animationDelay: {
          type: Number,
          value: 500
        },

        /**
         * The entry and exit animations that will be played when showing and
         * hiding the tooltip. If you want to override this, you must ensure
         * that your animationConfig has the exact format below.
         */
        animationConfig: {
          type: Object,
          value: function() {
            return {
              'entry': [{
                name: 'fade-in-animation',
                node: this,
                timing: {delay: 0}
              }],
              'exit': [{
                name: 'fade-out-animation',
                node: this
              }]
            }
          }
        },

        _showing: {
          type: Boolean,
          value: false
        }
      },

      listeners: {
        'neon-animation-finish': '_onAnimationFinish',
      },

      /**
       * Returns the target element that this tooltip is anchored to. It is
       * either the element given by the `for` attribute, or the immediate parent
       * of the tooltip.
       */
      get target () {
        var parentNode = Polymer.dom(this).parentNode;
        // If the parentNode is a document fragment, then we need to use the host.
        var ownerRoot = Polymer.dom(this).getOwnerRoot();

        var target;
        if (this.for) {
          target = Polymer.dom(ownerRoot).querySelector('#' + this.for);
        } else {
          target = parentNode.nodeType == Node.DOCUMENT_FRAGMENT_NODE ?
              ownerRoot.host : parentNode;
        }

        return target;
      },

      attached: function() {
        this._findTarget();
      },

      detached: function() {
        if (!this.manualMode)
          this._removeListeners();
      },

      show: function() {
        // If the tooltip is already showing, there's nothing to do.
        if (this._showing)
          return;

        if (Polymer.dom(this).textContent.trim() === ''){
          // Check if effective children are also empty
          var allChildrenEmpty = true;
          var effectiveChildren = Polymer.dom(this).getEffectiveChildNodes();
          for (var i = 0; i < effectiveChildren.length; i++) {
            if (effectiveChildren[i].textContent.trim() !== '') {
              allChildrenEmpty = false;
              break;
            }
          }
          if (allChildrenEmpty) {
            return;
          }
        }


        this.cancelAnimation();
        this._showing = true;
        this.toggleClass('hidden', false, this.$.tooltip);
        this.updatePosition();

        this.animationConfig.entry[0].timing = this.animationConfig.entry[0].timing || {};
        this.animationConfig.entry[0].timing.delay = this.animationDelay;
        this._animationPlaying = true;
        this.playAnimation('entry');
      },

      hide: function() {
        // If the tooltip is already hidden, there's nothing to do.
        if (!this._showing) {
          return;
        }

        // If the entry animation is still playing, don't try to play the exit
        // animation since this will reset the opacity to 1. Just end the animation.
        if (this._animationPlaying) {
          this.cancelAnimation();
          this._showing = false;
          this._onAnimationFinish();
          return;
        }

        this._showing = false;
        this._animationPlaying = true;
        this.playAnimation('exit');
      },

      updatePosition: function() {
        if (!this._target || !this.offsetParent)
          return;

        var offset = this.offset;
        // If a marginTop has been provided by the user (pre 1.0.3), use it.
        if (this.marginTop != 14 && this.offset == 14)
          offset = this.marginTop;

        var parentRect = this.offsetParent.getBoundingClientRect();
        var targetRect = this._target.getBoundingClientRect();
        var thisRect = this.getBoundingClientRect();

        var horizontalCenterOffset = (targetRect.width - thisRect.width) / 2;
        var verticalCenterOffset = (targetRect.height - thisRect.height) / 2;

        var targetLeft = targetRect.left - parentRect.left;
        var targetTop = targetRect.top - parentRect.top;

        var tooltipLeft, tooltipTop;

        switch (this.position) {
          case 'top':
            tooltipLeft = targetLeft + horizontalCenterOffset;
            tooltipTop = targetTop - thisRect.height - offset;
            break;
          case 'bottom':
            tooltipLeft = targetLeft + horizontalCenterOffset;
            tooltipTop = targetTop + targetRect.height + offset;
            break;
          case 'left':
            tooltipLeft = targetLeft - thisRect.width - offset;
            tooltipTop = targetTop + verticalCenterOffset;
            break;
          case 'right':
            tooltipLeft = targetLeft + targetRect.width + offset;
            tooltipTop = targetTop + verticalCenterOffset;
            break;
        }

        // TODO(noms): This should use IronFitBehavior if possible.
        if (this.fitToVisibleBounds) {
          // Clip the left/right side
          if (parentRect.left + tooltipLeft + thisRect.width > window.innerWidth) {
            this.style.right = '0px';
            this.style.left = 'auto';
          } else {
            this.style.left = Math.max(0, tooltipLeft) + 'px';
            this.style.right = 'auto';
          }

          // Clip the top/bottom side.
          if (parentRect.top + tooltipTop + thisRect.height > window.innerHeight) {
            this.style.bottom = parentRect.height + 'px';
            this.style.top = 'auto';
          } else {
            this.style.top = Math.max(-parentRect.top, tooltipTop) + 'px';
            this.style.bottom = 'auto';
          }
        } else {
          this.style.left = tooltipLeft + 'px';
          this.style.top = tooltipTop + 'px';
        }

      },

      _addListeners: function() {
        if (this._target) {
          this.listen(this._target, 'mouseenter', 'show');
          this.listen(this._target, 'focus', 'show');
          this.listen(this._target, 'mouseleave', 'hide');
          this.listen(this._target, 'blur', 'hide');
          this.listen(this._target, 'tap', 'hide');
        }
        this.listen(this, 'mouseenter', 'hide');
      },

      _findTarget: function() {
        if (!this.manualMode)
          this._removeListeners();

        this._target = this.target;

        if (!this.manualMode)
          this._addListeners();
      },

      _manualModeChanged: function() {
        if (this.manualMode)
          this._removeListeners();
        else
          this._addListeners();
      },

      _onAnimationFinish: function() {
        this._animationPlaying = false;
        if (!this._showing) {
          this.toggleClass('hidden', true, this.$.tooltip);
        }
      },

      _removeListeners: function() {
        if (this._target) {
          this.unlisten(this._target, 'mouseenter', 'show');
          this.unlisten(this._target, 'focus', 'show');
          this.unlisten(this._target, 'mouseleave', 'hide');
          this.unlisten(this._target, 'blur', 'hide');
          this.unlisten(this._target, 'tap', 'hide');
        }
        this.unlisten(this, 'mouseenter', 'hide');
      }
    });