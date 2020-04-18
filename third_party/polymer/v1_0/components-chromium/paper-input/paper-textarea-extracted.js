Polymer({
    is: 'paper-textarea',

    behaviors: [
      Polymer.PaperInputBehavior,
      Polymer.IronFormElementBehavior,
    ],

    properties: {
      _ariaLabelledBy: {
        observer: '_ariaLabelledByChanged',
        type: String,
      },

      _ariaDescribedBy: {
        observer: '_ariaDescribedByChanged',
        type: String,
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
      },
    },

    /**
     * @return {number}
     */
    get selectionStart() {
      return this.$.input.textarea.selectionStart;
    },
    set selectionStart(start) {
      this.$.input.textarea.selectionStart = start;
    },

    /**
     * @return {number}
     */
    get selectionEnd() {
      return this.$.input.textarea.selectionEnd;
    },
    set selectionEnd(end) {
      this.$.input.textarea.selectionEnd = end;
    },

    _ariaLabelledByChanged: function() {
      this._focusableElement.setAttribute('aria-label', this.label);
    },

    _ariaDescribedByChanged: function(ariaDescribedBy) {
      this._focusableElement.setAttribute('aria-describedby', ariaDescribedBy);
    },

    get _focusableElement() {
      return this.inputElement.textarea;
    },
  });