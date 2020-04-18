Polymer({
    is: 'paper-radio-group',

    behaviors: [
      Polymer.IronMenubarBehavior
    ],

    hostAttributes: {
      role: 'radiogroup',
    },

    properties: {
      /**
       * Fired when the radio group selection changes.
       *
       * @event paper-radio-group-changed
       */

      /**
       * Overriden from Polymer.IronSelectableBehavior
       */
      attrForSelected: {
        type: String,
        value: 'name'
      },

      /**
       * Overriden from Polymer.IronSelectableBehavior
       */
      selectedAttribute: {
        type: String,
        value: 'checked'
      },

      /**
       * Overriden from Polymer.IronSelectableBehavior
       */
      selectable: {
        type: String,
        value: 'paper-radio-button'
      },

      /**
       * If true, radio-buttons can be deselected
       */
      allowEmptySelection: {
        type: Boolean,
        value: false
      }
    },

    /**
     * Selects the given value.
     */
    select: function(value) {
      var newItem = this._valueToItem(value);
      if (newItem && newItem.hasAttribute('disabled')) {
        return;
      }

      if (this.selected) {
        var oldItem = this._valueToItem(this.selected);

        if (this.selected == value) {
          // If deselecting is allowed we'll have to apply an empty selection.
          // Otherwise, we should force the selection to stay and make this
          // action a no-op.
          if (this.allowEmptySelection) {
            value = '';
          } else {
            if (oldItem)
              oldItem.checked = true;
            return;
          }
        }

        if (oldItem)
          oldItem.checked = false;
      }

      Polymer.IronSelectableBehavior.select.apply(this, [value]);
      this.fire('paper-radio-group-changed');
    },

    _activateFocusedItem: function() {
      this._itemActivate(this._valueForItem(this.focusedItem), this.focusedItem);
    },

    _onUpKey: function(event) {
      this._focusPrevious();
      event.preventDefault();
      this._activateFocusedItem();
    },

    _onDownKey: function(event) {
      this._focusNext();
      event.preventDefault();
      this._activateFocusedItem();
    },

    _onLeftKey: function(event) {
      Polymer.IronMenubarBehaviorImpl._onLeftKey.apply(this, arguments);
      this._activateFocusedItem();
    },

    _onRightKey: function(event) {
      Polymer.IronMenubarBehaviorImpl._onRightKey.apply(this, arguments);
      this._activateFocusedItem();
    }
  });