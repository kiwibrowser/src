Polymer({
      is: 'iron-input',

      behaviors: [Polymer.IronValidatableBehavior],

      /**
       * Fired whenever `validate()` is called.
       *
       * @event iron-input-validate
       */

      properties: {

        /**
         * Use this property instead of `value` for two-way data binding, or to
         * set a default value for the input. **Do not** use the distributed
         * input's `value` property to set a default value.
         */
        bindValue: {type: String, value: ''},

        /**
         * Computed property that echoes `bindValue` (mostly used for Polymer 1.0
         * backcompatibility, if you were one-way binding to the Polymer 1.0
         * `input is="iron-input"` value attribute).
         */
        value: {type: String, computed: '_computeValue(bindValue)'},

        /**
         * Regex-like list of characters allowed as input; all characters not in the
         * list will be rejected. The recommended format should be a list of allowed
         * characters, for example, `[a-zA-Z0-9.+-!;:]`.
         *
         * This pattern represents the allowed characters for the field; as the user
         * inputs text, each individual character will be checked against the
         * pattern (rather than checking the entire value as a whole). If a
         * character is not a match, it will be rejected.
         *
         * Pasted input will have each character checked individually; if any
         * character doesn't match `allowedPattern`, the entire pasted string will
         * be rejected.
         *
         * Note: if you were using `iron-input` in 1.0, you were also required to
         * set `prevent-invalid-input`. This is no longer needed as of Polymer 2.0,
         * and will be set automatically for you if an `allowedPattern` is provided.
         *
         */
        allowedPattern: {type: String},

        /**
         * Set to true to auto-validate the input value as you type.
         */
        autoValidate: {type: Boolean, value: false},

        /**
         * The native input element.
         */
        _inputElement: Object,
      },

      observers: ['_bindValueChanged(bindValue, _inputElement)'],

      listeners: {'input': '_onInput', 'keypress': '_onKeypress'},

      created: function() {
        Polymer.IronA11yAnnouncer.requestAvailability();
        this._previousValidInput = '';
        this._patternAlreadyChecked = false;
      },

      attached: function() {
        // If the input is added at a later time, update the internal reference.
        this._observer = Polymer.dom(this).observeNodes(function(info) {
          this._initSlottedInput();
        }.bind(this));
      },

      detached: function() {
        if (this._observer) {
          Polymer.dom(this).unobserveNodes(this._observer);
          this._observer = null;
        }
      },

      /**
       * Returns the distributed input element.
       */
      get inputElement() {
        return this._inputElement;
      },

      _initSlottedInput: function() {
        this._inputElement = this.getEffectiveChildren()[0];

        if (this.inputElement && this.inputElement.value) {
          this.bindValue = this.inputElement.value;
        }

        this.fire('iron-input-ready');
      },

      get _patternRegExp() {
        var pattern;
        if (this.allowedPattern) {
          pattern = new RegExp(this.allowedPattern);
        } else {
          switch (this.inputElement.type) {
            case 'number':
              pattern = /[0-9.,e-]/;
              break;
          }
        }
        return pattern;
      },

      /**
       * @suppress {checkTypes}
       */
      _bindValueChanged: function(bindValue, inputElement) {
        // The observer could have run before attached() when we have actually
        // initialized this property.
        if (!inputElement) {
          return;
        }

        if (bindValue === undefined) {
          inputElement.value = null;
        } else if (bindValue !== inputElement.value) {
          this.inputElement.value = bindValue;
        }

        if (this.autoValidate) {
          this.validate();
        }

        // manually notify because we don't want to notify until after setting value
        this.fire('bind-value-changed', {value: bindValue});
      },

      _onInput: function() {
        // Need to validate each of the characters pasted if they haven't
        // been validated inside `_onKeypress` already.
        if (this.allowedPattern && !this._patternAlreadyChecked) {
          var valid = this._checkPatternValidity();
          if (!valid) {
            this._announceInvalidCharacter(
                'Invalid string of characters not entered.');
            this.inputElement.value = this._previousValidInput;
          }
        }
        this.bindValue = this._previousValidInput = this.inputElement.value;
        this._patternAlreadyChecked = false;
      },

      _isPrintable: function(event) {
        // What a control/printable character is varies wildly based on the browser.
        // - most control characters (arrows, backspace) do not send a `keypress`
        // event
        //   in Chrome, but the *do* on Firefox
        // - in Firefox, when they do send a `keypress` event, control chars have
        //   a charCode = 0, keyCode = xx (for ex. 40 for down arrow)
        // - printable characters always send a keypress event.
        // - in Firefox, printable chars always have a keyCode = 0. In Chrome, the
        // keyCode
        //   always matches the charCode.
        // None of this makes any sense.

        // For these keys, ASCII code == browser keycode.
        var anyNonPrintable = (event.keyCode == 8) ||  // backspace
            (event.keyCode == 9) ||                    // tab
            (event.keyCode == 13) ||                   // enter
            (event.keyCode == 27);                     // escape

        // For these keys, make sure it's a browser keycode and not an ASCII code.
        var mozNonPrintable = (event.keyCode == 19) ||  // pause
            (event.keyCode == 20) ||                    // caps lock
            (event.keyCode == 45) ||                    // insert
            (event.keyCode == 46) ||                    // delete
            (event.keyCode == 144) ||                   // num lock
            (event.keyCode == 145) ||                   // scroll lock
            (event.keyCode > 32 &&
             event.keyCode < 41) ||  // page up/down, end, home, arrows
            (event.keyCode > 111 && event.keyCode < 124);  // fn keys

        return !anyNonPrintable && !(event.charCode == 0 && mozNonPrintable);
      },

      _onKeypress: function(event) {
        if (!this.allowedPattern && this.inputElement.type !== 'number') {
          return;
        }
        var regexp = this._patternRegExp;
        if (!regexp) {
          return;
        }

        // Handle special keys and backspace
        if (event.metaKey || event.ctrlKey || event.altKey) {
          return;
        }

        // Check the pattern either here or in `_onInput`, but not in both.
        this._patternAlreadyChecked = true;

        var thisChar = String.fromCharCode(event.charCode);
        if (this._isPrintable(event) && !regexp.test(thisChar)) {
          event.preventDefault();
          this._announceInvalidCharacter(
              'Invalid character ' + thisChar + ' not entered.');
        }
      },

      _checkPatternValidity: function() {
        var regexp = this._patternRegExp;
        if (!regexp) {
          return true;
        }
        for (var i = 0; i < this.inputElement.value.length; i++) {
          if (!regexp.test(this.inputElement.value[i])) {
            return false;
          }
        }
        return true;
      },

      /**
       * Returns true if `value` is valid. The validator provided in `validator`
       * will be used first, then any constraints.
       * @return {boolean} True if the value is valid.
       */
      validate: function() {
        if (!this.inputElement) {
          this.invalid = false;
          return true;
        }

        // Use the nested input's native validity.
        var valid = this.inputElement.checkValidity();

        // Only do extra checking if the browser thought this was valid.
        if (valid) {
          // Empty, required input is invalid
          if (this.required && this.bindValue === '') {
            valid = false;
          } else if (this.hasValidator()) {
            valid =
                Polymer.IronValidatableBehavior.validate.call(this, this.bindValue);
          }
        }

        this.invalid = !valid;
        this.fire('iron-input-validate');
        return valid;
      },

      _announceInvalidCharacter: function(message) {
        this.fire('iron-announce', {text: message});
      },

      _computeValue: function(bindValue) {
        return bindValue;
      }
    });