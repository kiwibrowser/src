// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cr.ui', function() {
  /** @const */ var ArrayDataModel = cr.ui.ArrayDataModel;
  /** @const */ var List = cr.ui.List;
  /** @const */ var ListItem = cr.ui.ListItem;

  /**
   * Creates a new autocomplete list item.
   * This is suitable for selecting a web site, and used by default.
   * A different behavior can be set by AutocompleteListItem.itemConstructor.
   * @param {*} pageInfo The page this item represents.
   * @constructor
   * @extends {cr.ui.ListItem}
   */
  function AutocompleteListItem(pageInfo) {
    var el = cr.doc.createElement('div');
    el.pageInfo_ = pageInfo;
    AutocompleteListItem.decorate(el);
    return el;
  }

  /**
   * Decorates an element as an autocomplete list item.
   * @param {!HTMLElement} el The element to decorate.
   */
  AutocompleteListItem.decorate = function(el) {
    el.__proto__ = AutocompleteListItem.prototype;
    /** @type {AutocompleteListItem} */ (el).decorate();
  };

  AutocompleteListItem.prototype = {
    __proto__: ListItem.prototype,

    /** @override */
    decorate: function() {
      ListItem.prototype.decorate.call(this);

      var title = this.pageInfo_['title'];
      var url = this.pageInfo_['displayURL'];
      var titleEl = this.ownerDocument.createElement('span');
      titleEl.className = 'title';
      titleEl.textContent = title || url;
      this.appendChild(titleEl);

      if (title && title.length > 0 && url != title) {
        var separatorEl = this.ownerDocument.createTextNode(' - ');
        this.appendChild(separatorEl);

        var urlEl = this.ownerDocument.createElement('span');
        urlEl.className = 'url';
        urlEl.textContent = url;
        this.appendChild(urlEl);
      }
    },
  };

  /**
   * Creates a new autocomplete list popup.
   * @constructor
   * @extends {cr.ui.List}
   */
  var AutocompleteList = cr.ui.define('list');

  AutocompleteList.prototype = {
    __proto__: List.prototype,

    /**
     * The text field the autocomplete popup is currently attached to, if any.
     * @type {HTMLElement}
     * @private
     */
    targetInput_: null,

    /**
     * Keydown event listener to attach to a text field.
     * @type {Function}
     * @private
     */
    textFieldKeyHandler_: null,

    /**
     * Input event listener to attach to a text field.
     * @type {Function}
     * @private
     */
    textFieldInputHandler_: null,

    /** @override */
    decorate: function() {
      List.prototype.decorate.call(this);
      this.classList.add('autocomplete-suggestions');
      this.selectionModel = new cr.ui.ListSingleSelectionModel;

      this.itemConstructor = AutocompleteListItem;
      this.textFieldKeyHandler_ = this.handleAutocompleteKeydown_.bind(this);
      var self = this;
      this.textFieldInputHandler_ = function(e) {
        self.requestSuggestions(self.targetInput_.value);
      };
      this.addEventListener('change', function(e) {
        if (self.selectedItem)
          self.handleSelectedSuggestion(self.selectedItem);
      });
      // Start hidden; adding suggestions will unhide.
      this.hidden = true;
    },

    /** @override */
    createItem: function(pageInfo) {
      return new this.itemConstructor(pageInfo);
    },

    /**
     * The suggestions to show.
     * @type {Array}
     */
    set suggestions(suggestions) {
      this.dataModel = new ArrayDataModel(suggestions);
      this.hidden = !this.targetInput_ || suggestions.length == 0;
    },

    /**
     * Requests new suggestions. Called when new suggestions are needed.
     * @param {string} query the text to autocomplete from.
     */
    requestSuggestions: function(query) {},

    /**
     * Handles the Enter keydown event.
     * By default, clears and hides the autocomplete popup. Note that the
     * keydown event bubbles up, so the input field can handle the event.
     */
    handleEnterKeydown: function() {
      this.suggestions = [];
    },

    /**
     * Handles the selected suggestion. Called when a suggestion is selected.
     * By default, sets the target input element's value to the 'url' field
     * of the selected suggestion.
     * @param {Object} selectedSuggestion
     */
    handleSelectedSuggestion: function(selectedSuggestion) {
      var input = this.targetInput_;
      if (!input)
        return;
      input.value = selectedSuggestion['url'];
      // Programatically change the value won't trigger a change event, but
      // clients are likely to want to know when changes happen, so fire one.
      cr.dispatchSimpleEvent(input, 'change', true);
    },

    /**
     * Attaches the popup to the given input element. Requires
     * that the input be wrapped in a block-level container of the same width.
     * @param {HTMLElement} input The input element to attach to.
     */
    attachToInput: function(input) {
      if (this.targetInput_ == input)
        return;

      this.detach();
      this.targetInput_ = input;
      this.style.width = input.getBoundingClientRect().width + 'px';
      this.hidden = false;  // Necessary for positionPopupAroundElement to work.
      cr.ui.positionPopupAroundElement(input, this, cr.ui.AnchorType.BELOW);
      // Start hidden; when the data model gets results the list will show.
      this.hidden = true;

      input.addEventListener('keydown', this.textFieldKeyHandler_, true);
      input.addEventListener('input', this.textFieldInputHandler_);

      if (!this.boundSyncWidthAndPositionToInput_) {
        this.boundSyncWidthAndPositionToInput_ =
            this.syncWidthAndPositionToInput.bind(this);
      }
      // We need to call syncWidthAndPositionToInput whenever page zoom level or
      // page size is changed.
      window.addEventListener('resize', this.boundSyncWidthAndPositionToInput_);
    },

    /**
     * Detaches the autocomplete popup from its current input element, if any.
     */
    detach: function() {
      var input = this.targetInput_;
      if (!input)
        return;

      input.removeEventListener('keydown', this.textFieldKeyHandler_, true);
      input.removeEventListener('input', this.textFieldInputHandler_);
      this.targetInput_ = null;
      this.suggestions = [];
      if (this.boundSyncWidthAndPositionToInput_) {
        window.removeEventListener(
            'resize', this.boundSyncWidthAndPositionToInput_);
      }
    },

    /**
     * Makes sure that the suggestion list matches the width and the position
     * of the input it is attached to. Should be called any time the input is
     * resized.
     */
    syncWidthAndPositionToInput: function() {
      var input = this.targetInput_;
      if (input) {
        this.style.width = input.getBoundingClientRect().width + 'px';
        cr.ui.positionPopupAroundElement(input, this, cr.ui.AnchorType.BELOW);
      }
    },

    /**
     * syncWidthAndPositionToInput function bound to |this|.
     * @type {!Function|undefined}
     * @private
     */
    boundSyncWidthAndPositionToInput_: undefined,

    /**
     * @return {HTMLElement} The text field the autocomplete popup is currently
     *     attached to, if any.
     */
    get targetInput() {
      return this.targetInput_;
    },

    /**
     * Handles input field key events that should be interpreted as autocomplete
     * commands.
     * @param {Event} event The keydown event.
     * @private
     */
    handleAutocompleteKeydown_: function(event) {
      if (this.hidden)
        return;
      var handled = false;
      switch (event.key) {
        case 'Escape':
          this.suggestions = [];
          handled = true;
          break;
        case 'Enter':
          // If the user has already selected an item using the arrow keys then
          // presses Enter, keep |handled| = false, so the input field can
          // handle the event as well.
          this.handleEnterKeydown();
          break;
        case 'ArrowUp':
        case 'ArrowDown':
          this.dispatchEvent(new KeyboardEvent(event.type, event));
          handled = true;
          break;
      }
      // Don't let arrow keys affect the text field, or bubble up to, e.g.,
      // an enclosing list item.
      if (handled) {
        event.preventDefault();
        event.stopPropagation();
      }
    },
  };

  return {AutocompleteList: AutocompleteList};
});
