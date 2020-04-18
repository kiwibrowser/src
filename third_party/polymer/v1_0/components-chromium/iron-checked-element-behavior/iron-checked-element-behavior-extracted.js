/**
   * Use `Polymer.IronCheckedElementBehavior` to implement a custom element
   * that has a `checked` property, which can be used for validation if the
   * element is also `required`. Element instances implementing this behavior
   * will also be registered for use in an `iron-form` element.
   *
   * @demo demo/index.html
   * @polymerBehavior Polymer.IronCheckedElementBehavior
   */
  Polymer.IronCheckedElementBehaviorImpl = {

    properties: {
      /**
       * Fired when the checked state changes.
       *
       * @event iron-change
       */

      /**
       * Gets or sets the state, `true` is checked and `false` is unchecked.
       */
      checked: {
        type: Boolean,
        value: false,
        reflectToAttribute: true,
        notify: true,
        observer: '_checkedChanged'
      },

      /**
       * If true, the button toggles the active state with each tap or press
       * of the spacebar.
       */
      toggles: {
        type: Boolean,
        value: true,
        reflectToAttribute: true
      },

      /* Overriden from Polymer.IronFormElementBehavior */
      value: {
        type: String,
        value: 'on',
        observer: '_valueChanged'
      }
    },

    observers: [
      '_requiredChanged(required)'
    ],

    created: function() {
      // Used by `iron-form` to handle the case that an element with this behavior
      // doesn't have a role of 'checkbox' or 'radio', but should still only be
      // included when the form is serialized if `this.checked === true`.
      this._hasIronCheckedElementBehavior = true;
    },

    /**
     * Returns false if the element is required and not checked, and true otherwise.
     * @param {*=} _value Ignored.
     * @return {boolean} true if `required` is false or if `checked` is true.
     */
    _getValidity: function(_value) {
      return this.disabled || !this.required || this.checked;
    },

    /**
     * Update the aria-required label when `required` is changed.
     */
    _requiredChanged: function() {
      if (this.required) {
        this.setAttribute('aria-required', 'true');
      } else {
        this.removeAttribute('aria-required');
      }
    },

    /**
     * Fire `iron-changed` when the checked state changes.
     */
    _checkedChanged: function() {
      this.active = this.checked;
      this.fire('iron-change');
    },

    /**
     * Reset value to 'on' if it is set to `undefined`.
     */
    _valueChanged: function() {
      if (this.value === undefined || this.value === null) {
        this.value = 'on';
      }
    }
  };

  /** @polymerBehavior Polymer.IronCheckedElementBehavior */
  Polymer.IronCheckedElementBehavior = [
    Polymer.IronFormElementBehavior,
    Polymer.IronValidatableBehavior,
    Polymer.IronCheckedElementBehaviorImpl
  ];