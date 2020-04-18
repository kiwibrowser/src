Polymer({
    is: 'paper-input',

    behaviors: [
      Polymer.PaperInputBehavior,
      Polymer.IronFormElementBehavior
    ],

    beforeRegister: function() {
      var version = 'v1';  // Hard coded Polymer 2 style iron-input.
      var template = Polymer.DomModule.import('paper-input', 'template');
      var inputTemplate = Polymer.DomModule.import('paper-input', 'template#' + version);
      var inputPlaceholder = template.content.querySelector('#template-placeholder');
      if (inputPlaceholder) {
        inputPlaceholder.parentNode.replaceChild(inputTemplate.content, inputPlaceholder);
      }
      // else it's already been processed, probably in superclass
    },

    /**
     * Returns a reference to the focusable element. Overridden from PaperInputBehavior
     * to correctly focus the native input.
     *
     * @return {!HTMLElement}
     */
    get _focusableElement() {
      // TODO(hcarmona): remove this patch after polymer is v2.
      return this.inputElement._inputElement;
    },

    // Note: This event is only available in the 1.0 version of this element.
    // In 2.0, the functionality of `_onIronInputReady` is done in
    // PaperInputBehavior::attached.
    listeners: {
      'iron-input-ready': '_onIronInputReady'
    },

    _onIronInputReady: function() {
      // Even though this is only used in the next line, save this for
      // backwards compatibility, since the native input had this ID until 2.0.5.
      if (!this.$.nativeInput) {
        this.$.nativeInput = this.$$('input');
      }
      if (this.inputElement &&
          this._typesThatHaveText.indexOf(this.$.nativeInput.type) !== -1) {
        this.alwaysFloatLabel = true;
      }

      // Only validate when attached if the input already has a value.
      if (!!this.inputElement.bindValue) {
        this.$.container._handleValueAndAutoValidate(this.inputElement);
      }
    },
  });