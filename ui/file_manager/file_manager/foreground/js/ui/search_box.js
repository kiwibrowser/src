// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Search box.
 *
 * @param {!Element} element Root element of the search box.
 * @param {!Element} searchButton Search button.
 * @extends {cr.EventTarget}
 * @constructor
 */
function SearchBox(element, searchButton) {
  cr.EventTarget.call(this);

  /**
   * Autocomplete List.
   * @type {!SearchBox.AutocompleteList}
   */
  this.autocompleteList = new SearchBox.AutocompleteList(element.ownerDocument);

  /**
   * Root element of the search box.
   * @type {!Element}
   */
  this.element = element;

  /**
   * Search button.
   * @type {!Element}
   */
  this.searchButton = searchButton;

  /**
   * Ripple effect of search button.
   * @private {!FilesToggleRipple}
   * @const
   */
  this.searchButtonToggleRipple_ =
      /** @type {!FilesToggleRipple} */ (queryRequiredElement(
          'files-toggle-ripple', this.searchButton));

  /**
   * Text input of the search box.
   * @type {!HTMLInputElement}
   */
  this.inputElement = /** @type {!HTMLInputElement} */ (
      element.querySelector('input'));

  /**
   * Clear button of the search box.
   * @private {!Element}
   */
  this.clearButton_ = assert(element.querySelector('.clear'));

  // Register events.
  this.inputElement.addEventListener('input', this.onInput_.bind(this));
  this.inputElement.addEventListener('keydown', this.onKeyDown_.bind(this));
  this.inputElement.addEventListener('focus', this.onFocus_.bind(this));
  this.inputElement.addEventListener('blur', this.onBlur_.bind(this));
  this.inputElement.ownerDocument.addEventListener(
      'dragover',
      this.onDragEnter_.bind(this),
      true);
  this.inputElement.ownerDocument.addEventListener(
      'dragend',
      this.onDragEnd_.bind(this));
  this.searchButton.addEventListener(
      'click',
      this.onSearchButtonClick_.bind(this));
  this.clearButton_.addEventListener(
      'click',
      this.onClearButtonClick_.bind(this));
  var dispatchItemSelect =
      cr.dispatchSimpleEvent.bind(cr, this, SearchBox.EventType.ITEM_SELECT);
  this.autocompleteList.handleEnterKeydown = dispatchItemSelect;
  this.autocompleteList.addEventListener('mousedown', dispatchItemSelect);

  // Append dynamically created element.
  element.parentNode.appendChild(this.autocompleteList);
}

SearchBox.prototype = {
  __proto__: cr.EventTarget.prototype
};

/**
 * Event type.
 * @enum {string}
 */
SearchBox.EventType = {
  // Dispatched when the text in the search box is changed.
  TEXT_CHANGE: 'textchange',
  // Dispatched when the item in the auto complete list is selected.
  ITEM_SELECT: 'itemselect'
};

/**
 * Autocomplete list for search box.
 * @param {Document} document Document.
 * @constructor
 * @extends {cr.ui.AutocompleteList}
 */
SearchBox.AutocompleteList = function(document) {
  var self = cr.ui.AutocompleteList.call(this);
  self.__proto__ = SearchBox.AutocompleteList.prototype;
  self.id = 'autocomplete-list';
  self.autoExpands = true;
  self.itemConstructor = SearchBox.AutocompleteListItem_.bind(null, document);
  self.addEventListener('mouseover', self.onMouseOver_.bind(self));
  return self;
};

SearchBox.AutocompleteList.prototype = {
  __proto__: cr.ui.AutocompleteList.prototype
};

/**
 * Do nothing when a suggestion is selected.
 * @override
 */
SearchBox.AutocompleteList.prototype.handleSelectedSuggestion = function() {};

/**
 * Change the selection by a mouse over instead of just changing the
 * color of moused over element with :hover in CSS. Here's why:
 *
 * 1) The user selects an item A with up/down keys (item A is highlighted)
 * 2) Then the user moves the cursor to another item B
 *
 * If we just change the color of moused over element (item B), both
 * the item A and B are highlighted. This is bad. We should change the
 * selection so only the item B is highlighted.
 *
 * @param {Event} event Event.
 * @private
 */
SearchBox.AutocompleteList.prototype.onMouseOver_ = function(event) {
  if (event.target.itemInfo)
    this.selectedItem = event.target.itemInfo;
};

/**
 * ListItem element for autocomplete.
 *
 * @param {Document} document Document.
 * @param {SearchItem|SearchResult} item An object representing a suggestion.
 * @constructor
 * @private
 */
SearchBox.AutocompleteListItem_ = function(document, item) {
  var li = new cr.ui.ListItem();
  li.itemInfo = item;

  var icon = document.createElement('div');
  icon.className = 'detail-icon';

  var text = document.createElement('div');
  text.className = 'detail-text';

  if (item.isHeaderItem) {
    icon.setAttribute('search-icon', '');
    text.innerHTML =
        strf('SEARCH_DRIVE_HTML', util.htmlEscape(item.searchQuery));
  } else {
    var iconType = FileType.getIcon(item.entry);
    icon.setAttribute('file-type-icon', iconType);
    // highlightedBaseName is a piece of HTML with meta characters properly
    // escaped. See the comment at fileManagerPrivate.searchDriveMetadata().
    text.innerHTML = item.highlightedBaseName;
  }
  li.appendChild(icon);
  li.appendChild(text);
  return li;
};

/**
 * Clears the search query.
 */
SearchBox.prototype.clear = function() {
  this.inputElement.value = '';
  this.updateStyles_();
};

/**
 * Sets hidden attribute for components of search box.
 * @param {boolean} hidden True when the search box need to be hidden.
 */
SearchBox.prototype.setHidden = function(hidden) {
  this.element.hidden = hidden;
  this.searchButton.hidden = hidden;
};

/**
 * @private
 */
SearchBox.prototype.onInput_ = function() {
  this.updateStyles_();
  cr.dispatchSimpleEvent(this, SearchBox.EventType.TEXT_CHANGE);
};

/**
 * Handles a focus event of the search box.
 * @private
 */
SearchBox.prototype.onFocus_ = function() {
  this.element.classList.toggle('has-cursor', true);
  this.autocompleteList.attachToInput(this.inputElement);
  this.updateStyles_();
  this.searchButtonToggleRipple_.activated = true;
  metrics.recordUserAction('SelectSearch');
};

/**
 * Handles a blur event of the search box.
 * @private
 */
SearchBox.prototype.onBlur_ = function() {
  this.element.classList.toggle('has-cursor', false);
  this.autocompleteList.detach();
  this.updateStyles_();
  this.searchButtonToggleRipple_.activated = false;
  // When input has any text we keep it displayed with current search.
  this.inputElement.hidden = this.inputElement.value.length == 0;
};

/**
 * Handles a keydown event of the search box.
 * @param {Event} event
 * @private
 */
SearchBox.prototype.onKeyDown_ = function(event) {
  event = /** @type {KeyboardEvent} */ (event);
  // Handle only Esc key now.
  if (event.key != 'Escape' || this.inputElement.value)
    return;

  this.inputElement.tabIndex = -1;  // Focus to default element after blur.
  this.inputElement.blur();
};

/**
 * Handles a dragenter event and refuses a drag source of files.
 * @param {Event} event The dragenter event.
 * @private
 */
SearchBox.prototype.onDragEnter_ = function(event) {
  event = /** @type {DragEvent} */ (event);
  // For normal elements, they does not accept drag drop by default, and accept
  // it by using event.preventDefault. But input elements accept drag drop
  // by default. So disable the input element here to prohibit drag drop.
  if (event.dataTransfer.types.indexOf('text/plain') === -1)
    this.inputElement.style.pointerEvents = 'none';
};

/**
 * Handles a dragend event.
 * @private
 */
SearchBox.prototype.onDragEnd_ = function() {
  this.inputElement.style.pointerEvents = '';
};

/**
 * Updates styles of the search box.
 * @private
 */
SearchBox.prototype.updateStyles_ = function() {
  var hasText = !!this.inputElement.value;
  this.element.classList.toggle('has-text', hasText);
  var hasFocusOnInput = this.element.classList.contains('has-cursor');

  // See go/filesapp-tabindex for tabindexes.
  this.inputElement.tabIndex = (hasText || hasFocusOnInput) ? 14 : -1;
  this.searchButton.tabIndex = (hasText || hasFocusOnInput) ? -1 : 13;
};

/**
 * @private
 */
SearchBox.prototype.onSearchButtonClick_ = function() {
  this.inputElement.hidden = false;
  this.inputElement.focus();
};

/**
 * @private
 */
SearchBox.prototype.onClearButtonClick_ = function() {
  this.inputElement.value = '';
  this.onInput_();
};
