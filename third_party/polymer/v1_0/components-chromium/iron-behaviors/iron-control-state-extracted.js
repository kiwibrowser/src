/**
   * @demo demo/index.html
   * @polymerBehavior
   */
  Polymer.IronControlState = {

    properties: {

      /**
       * If true, the element currently has focus.
       */
      focused: {
        type: Boolean,
        value: false,
        notify: true,
        readOnly: true,
        reflectToAttribute: true
      },

      /**
       * If true, the user cannot interact with this element.
       */
      disabled: {
        type: Boolean,
        value: false,
        notify: true,
        observer: '_disabledChanged',
        reflectToAttribute: true
      },

      /**
       * Value of the `tabindex` attribute before `disabled` was activated.
       * `null` means the attribute was not present.
       * @type {?string|undefined}
       */
      _oldTabIndex: {
        type: String
      },

      _boundFocusBlurHandler: {
        type: Function,
        value: function() {
          return this._focusBlurHandler.bind(this);
        }
      },

      __handleEventRetargeting: {
        type: Boolean,
        value: function() {
          return !this.shadowRoot && !Polymer.Element;
        }
      }
    },

    observers: [
      '_changedControlState(focused, disabled)'
    ],

    /**
     * @return {void}
     */
    ready: function() {
      this.addEventListener('focus', this._boundFocusBlurHandler, true);
      this.addEventListener('blur', this._boundFocusBlurHandler, true);
    },

    _focusBlurHandler: function(event) {
      // In Polymer 2.0, the library takes care of retargeting events.
      if (Polymer.Element) {
        this._setFocused(event.type === 'focus');
        return;
      }

      // NOTE(cdata):  if we are in ShadowDOM land, `event.target` will
      // eventually become `this` due to retargeting; if we are not in
      // ShadowDOM land, `event.target` will eventually become `this` due
      // to the second conditional which fires a synthetic event (that is also
      // handled). In either case, we can disregard `event.path`.
      if (event.target === this) {
        this._setFocused(event.type === 'focus');
      } else if (this.__handleEventRetargeting) {
        var target = /** @type {Node} */(Polymer.dom(event).localTarget);
        if (!this.isLightDescendant(target)) {
          this.fire(event.type, {sourceEvent: event}, {
            node: this,
            bubbles: event.bubbles,
            cancelable: event.cancelable
          });
        }
      }
    },

    _disabledChanged: function(disabled, old) {
      this.setAttribute('aria-disabled', disabled ? 'true' : 'false');
      this.style.pointerEvents = disabled ? 'none' : '';
      if (disabled) {
        // Read the `tabindex` attribute instead of the `tabIndex` property.
        // The property returns `-1` if there is no `tabindex` attribute.
        // This distinction is important when restoring the value because
        // leaving `-1` hides shadow root children from the tab order.
        this._oldTabIndex = this.getAttribute('tabindex');
        this._setFocused(false);
        this.tabIndex = -1;
        this.blur();
      } else if (this._oldTabIndex !== undefined) {
        if (this._oldTabIndex === null) {
          this.removeAttribute('tabindex');
        } else {
          this.setAttribute('tabindex', this._oldTabIndex);
        }
      }
    },

    _changedControlState: function() {
      // _controlStateChanged is abstract, follow-on behaviors may implement it
      if (this._controlStateChanged) {
        this._controlStateChanged();
      }
    }

  };