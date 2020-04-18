Polymer({

    is: 'iron-autogrow-textarea',

    behaviors: [
      Polymer.IronValidatableBehavior,
      Polymer.IronControlState
    ],

    properties: {
      /**
       * Use this property instead of `bind-value` for two-way data binding.
       * @type {string|number}
       */
      value: {
        observer: '_valueChanged',
        type: String,
        notify: true
      },

      /**
       * This property is deprecated, and just mirrors `value`. Use `value` instead.
       * @type {string|number}
       */
      bindValue: {
        observer: '_bindValueChanged',
        type: String,
        notify: true
      },

      /**
       * The initial number of rows.
       *
       * @attribute rows
       * @type number
       * @default 1
       */
      rows: {
        type: Number,
        value: 1,
        observer: '_updateCached'
      },

      /**
       * The maximum number of rows this element can grow to until it
       * scrolls. 0 means no maximum.
       *
       * @attribute maxRows
       * @type number
       * @default 0
       */
      maxRows: {
       type: Number,
       value: 0,
       observer: '_updateCached'
      },

      /**
       * Bound to the textarea's `autocomplete` attribute.
       */
      autocomplete: {
        type: String,
        value: 'off'
      },

      /**
       * Bound to the textarea's `autofocus` attribute.
       */
      autofocus: {
        type: Boolean,
        value: false
      },

      /**
       * Bound to the textarea's `inputmode` attribute.
       */
      inputmode: {
        type: String
      },

      /**
       * Bound to the textarea's `placeholder` attribute.
       */
      placeholder: {
        type: String
      },

      /**
       * Bound to the textarea's `readonly` attribute.
       */
      readonly: {
        type: String
      },

      /**
       * Set to true to mark the textarea as required.
       */
      required: {
        type: Boolean
      },

      /**
       * The minimum length of the input value.
       */
      minlength: {
        type: Number
      },

      /**
       * The maximum length of the input value.
       */
      maxlength: {
        type: Number
      },

      /**
       * Bound to the textarea's `aria-label` attribute.
       */
      label: {
        type: String
      }

    },

    listeners: {
      'input': '_onInput'
    },

    /**
     * Returns the underlying textarea.
     * @type HTMLTextAreaElement
     */
    get textarea() {
      return this.$.textarea;
    },

    /**
     * Returns textarea's selection start.
     * @type Number
     */
    get selectionStart() {
      return this.$.textarea.selectionStart;
    },

    /**
     * Returns textarea's selection end.
     * @type Number
     */
    get selectionEnd() {
      return this.$.textarea.selectionEnd;
    },

    /**
     * Sets the textarea's selection start.
     */
    set selectionStart(value) {
      this.$.textarea.selectionStart = value;
    },

    /**
     * Sets the textarea's selection end.
     */
    set selectionEnd(value) {
      this.$.textarea.selectionEnd = value;
    },

    attached: function() {
      /* iOS has an arbitrary left margin of 3px that isn't present
       * in any other browser, and means that the paper-textarea's cursor
       * overlaps the label.
       * See https://github.com/PolymerElements/paper-input/issues/468.
       */
      var IS_IOS = navigator.userAgent.match(/iP(?:[oa]d|hone)/);
      if (IS_IOS) {
        this.$.textarea.style.marginLeft = '-3px';
      }
    },

    /**
     * Returns true if `value` is valid. The validator provided in `validator`
     * will be used first, if it exists; otherwise, the `textarea`'s validity
     * is used.
     * @return {boolean} True if the value is valid.
     */
    validate: function() {
      // Use the nested input's native validity.
      var valid =  this.$.textarea.validity.valid;

      // Only do extra checking if the browser thought this was valid.
      if (valid) {
        // Empty, required input is invalid
        if (this.required && this.value === '') {
          valid = false;
        } else if (this.hasValidator()) {
          valid = Polymer.IronValidatableBehavior.validate.call(this, this.value);
        }
      }

      this.invalid = !valid;
      this.fire('iron-input-validate');
      return valid;
    },

    _bindValueChanged: function(bindValue) {
      this.value = bindValue;
    },

    _valueChanged: function(value) {
      var textarea = this.textarea;
      if (!textarea) {
        return;
      }

      // If the bindValue changed manually, then we need to also update
      // the underlying textarea's value. Otherwise this change was probably
      // generated from the _onInput handler, and the two values are already
      // the same.
      if (textarea.value !== value) {
        textarea.value = !(value || value === 0) ? '' : value;
      }

      this.bindValue = value;
      this.$.mirror.innerHTML = this._valueForMirror();

      // Manually notify because we don't want to notify until after setting value.
      this.fire('bind-value-changed', {value: this.bindValue});
    },

    _onInput: function(event) {
      var eventPath = Polymer.dom(event).path;
      this.value = eventPath ? eventPath[0].value : event.target.value;
    },

    _constrain: function(tokens) {
      var _tokens;
      tokens = tokens || [''];
      // Enforce the min and max heights for a multiline input to avoid measurement
      if (this.maxRows > 0 && tokens.length > this.maxRows) {
        _tokens = tokens.slice(0, this.maxRows);
      } else {
        _tokens = tokens.slice(0);
      }
      while (this.rows > 0 && _tokens.length < this.rows) {
        _tokens.push('');
      }
      // Use &#160; instead &nbsp; of to allow this element to be used in XHTML.
      return _tokens.join('<br/>') + '&#160;';
    },

    _valueForMirror: function() {
      var input = this.textarea;
      if (!input) {
        return;
      }
      this.tokens = (input && input.value) ? input.value.replace(/&/gm, '&amp;').replace(/"/gm, '&quot;').replace(/'/gm, '&#39;').replace(/</gm, '&lt;').replace(/>/gm, '&gt;').split('\n') : [''];
      return this._constrain(this.tokens);
    },

    _updateCached: function() {
      this.$.mirror.innerHTML = this._constrain(this.tokens);
    },
  });