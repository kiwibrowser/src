// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * DefaultTaskDialog contains a message, a list box, an ok button, and a
 * cancel button.
 * This dialog should be used as task picker for file operations.
 */
cr.define('cr.filebrowser', function() {

  /**
   * Creates dialog in DOM tree.
   *
   * @param {HTMLElement} parentNode Node to be parent for this dialog.
   * @constructor
   * @extends {FileManagerDialogBase}
   */
  function DefaultTaskDialog(parentNode) {
    FileManagerDialogBase.call(this, parentNode);

    this.frame_.id = 'default-task-dialog';

    this.list_ = new cr.ui.List();
    this.list_.id = 'default-tasks-list';
    this.frame_.insertBefore(this.list_, this.text_.nextSibling);

    this.selectionModel_ = this.list_.selectionModel =
        new cr.ui.ListSingleSelectionModel();
    this.dataModel_ = this.list_.dataModel = new cr.ui.ArrayDataModel([]);

    // List has max-height defined at css, so that list grows automatically,
    // but doesn't exceed predefined size.
    this.list_.autoExpands = true;
    this.list_.activateItemAtIndex = this.activateItemAtIndex_.bind(this);
    // Use 'click' instead of 'change' for keyboard users.
    this.list_.addEventListener('click', this.onSelected_.bind(this));
    this.list_.addEventListener('change', this.onListChange_.bind(this));

    this.initialFocusElement_ = this.list_;

    var self = this;

    // Binding stuff doesn't work with constructors, so we have to create
    // closure here.
    this.list_.itemConstructor = function(item) {
      return self.renderItem(item);
    };
  }

  DefaultTaskDialog.prototype = {
    __proto__: FileManagerDialogBase.prototype
  };

  /**
   * Renders item for list.
   * @param {Object} item Item to render.
   */
  DefaultTaskDialog.prototype.renderItem = function(item) {
    var result = this.document_.createElement('li');

    var div = this.document_.createElement('div');
    div.textContent = item.label;

    if (item.iconType) {
      div.setAttribute('file-type-icon', item.iconType);
    } else if (item.iconUrl) {
      div.style.backgroundImage = 'url(' + item.iconUrl + ')';
    }

    if (item.class)
      div.classList.add(item.class);

    result.appendChild(div);
    // A11y - make it focusable and readable.
    result.setAttribute('tabindex', '-1');

    cr.defineProperty(result, 'lead', cr.PropertyKind.BOOL_ATTR);
    cr.defineProperty(result, 'selected', cr.PropertyKind.BOOL_ATTR);

    return result;
  };

  /**
   * Shows dialog.
   *
   * @param {string} title Title in dialog caption.
   * @param {string} message Message in dialog caption.
   * @param {Array<Object>} items Items to render in the list.
   * @param {number} defaultIndex Item to select by default.
   * @param {function(Object)} onSelectedItem Callback which is called when an
   *     item is selected.
   */
  DefaultTaskDialog.prototype.showDefaultTaskDialog =
      function(title, message, items, defaultIndex, onSelectedItem) {

    this.onSelectedItemCallback_ = onSelectedItem;

    var show = FileManagerDialogBase.prototype.showTitleAndTextDialog.call(
        this, title, message);

    if (!show) {
      console.error('DefaultTaskDialog can\'t be shown.');
      return;
    }

    if (!message) {
      this.text_.setAttribute('hidden', 'hidden');
    } else {
      this.text_.removeAttribute('hidden');
    }

    this.list_.startBatchUpdates();
    this.dataModel_.splice(0, this.dataModel_.length);
    for (var i = 0; i < items.length; i++) {
      this.dataModel_.push(items[i]);
    }
    this.selectionModel_.selectedIndex = defaultIndex;
    this.list_.endBatchUpdates();
  };

  /**
   * List activation handler. Closes dialog and calls 'ok' callback.
   * @param {number} index Activated index.
   */
  DefaultTaskDialog.prototype.activateItemAtIndex_ = function(index) {
    this.hide();
    this.onSelectedItemCallback_(this.dataModel_.item(index));
  };

  /**
   * Closes dialog and invokes callback with currently-selected item.
   */
  DefaultTaskDialog.prototype.onSelected_ = function() {
    if (this.selectionModel_.selectedIndex !== -1)
      this.activateItemAtIndex_(this.selectionModel_.selectedIndex);
  };

  /**
   * Called when cr.ui.List triggers a change event, which means user
   * focused a new item on the list. Used here to isue .focus() on
   * currently active item so ChromeVox can read it out.
   * @param {!Event} event triggered by cr.ui.List.
   */
  DefaultTaskDialog.prototype.onListChange_ = function(event) {
    var list = /** @type {cr.ui.List} */ (event.target);
    var activeItem =
        list.getListItemByIndex(list.selectionModel_.selectedIndex);
    if (activeItem)
      activeItem.focus();
  };

  /**
   * @override
   */
  DefaultTaskDialog.prototype.onContainerKeyDown_ = function(event) {
    // Handle Escape.
    if (event.keyCode == 27) {
      this.hide();
      event.preventDefault();
    } else if (event.keyCode == 32 || event.keyCode == 13) {
      this.onSelected_();
      event.preventDefault();
    }
  };

  return {DefaultTaskDialog: DefaultTaskDialog};
});
