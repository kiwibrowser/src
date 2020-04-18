// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cr.ui', function() {
  // require cr.ui.define
  // require cr.ui.limitInputWidth

  /**
   * The number of pixels to indent per level.
   * @type {number}
   * @const
   */
  var INDENT = 20;

  /**
   * Returns the computed style for an element.
   * @param {!Element} el The element to get the computed style for.
   * @return {!CSSStyleDeclaration} The computed style.
   */
  function getComputedStyle(el) {
    return assert(el.ownerDocument.defaultView.getComputedStyle(el));
  }

  /**
   * Helper function that finds the first ancestor tree item.
   * @param {Node} node The node to start searching from.
   * @return {cr.ui.TreeItem} The found tree item or null if not found.
   */
  function findTreeItem(node) {
    while (node && !(node instanceof TreeItem)) {
      node = node.parentNode;
    }
    return node;
  }

  /**
   * Creates a new tree element.
   * @param {Object=} opt_propertyBag Optional properties.
   * @constructor
   * @extends {HTMLElement}
   */
  var Tree = cr.ui.define('tree');

  Tree.prototype = {
    __proto__: HTMLElement.prototype,

    /**
     * Initializes the element.
     */
    decorate: function() {
      // Make list focusable
      if (!this.hasAttribute('tabindex'))
        this.tabIndex = 0;

      this.addEventListener('click', this.handleClick);
      this.addEventListener('mousedown', this.handleMouseDown);
      this.addEventListener('dblclick', this.handleDblClick);
      this.addEventListener('keydown', this.handleKeyDown);

      if (!this.hasAttribute('role'))
        this.setAttribute('role', 'group');
    },

    /**
     * Returns the tree item that are children of this tree.
     */
    get items() {
      return this.children;
    },

    /**
     * Adds a tree item to the tree.
     * @param {!cr.ui.TreeItem} treeItem The item to add.
     */
    add: function(treeItem) {
      this.addAt(treeItem, 0xffffffff);
    },

    /**
     * Adds a tree item at the given index.
     * @param {!cr.ui.TreeItem} treeItem The item to add.
     * @param {number} index The index where we want to add the item.
     */
    addAt: function(treeItem, index) {
      this.insertBefore(treeItem, this.children[index]);
      treeItem.setDepth_(this.depth + 1);
    },

    /**
     * Removes a tree item child.
     *
     * TODO(dbeam): this method now conflicts with HTMLElement#remove(), which
     * is why the @param is optional. Rename.
     *
     * @param {!cr.ui.TreeItem=} treeItem The tree item to remove.
     */
    remove: function(treeItem) {
      this.removeChild(/** @type {!cr.ui.TreeItem} */ (treeItem));
    },

    /**
     * The depth of the node. This is 0 for the tree itself.
     * @type {number}
     */
    get depth() {
      return 0;
    },

    /**
     * Handles click events on the tree and forwards the event to the relevant
     * tree items as necesary.
     * @param {Event} e The click event object.
     */
    handleClick: function(e) {
      var treeItem = findTreeItem(/** @type {!Node} */ (e.target));
      if (treeItem)
        treeItem.handleClick(e);
    },

    handleMouseDown: function(e) {
      if (e.button == 2)  // right
        this.handleClick(e);
    },

    /**
     * Handles double click events on the tree.
     * @param {Event} e The dblclick event object.
     */
    handleDblClick: function(e) {
      var treeItem = findTreeItem(/** @type {!Node} */ (e.target));
      if (treeItem)
        treeItem.expanded = !treeItem.expanded;
    },

    /**
     * Handles keydown events on the tree and updates selection and exanding
     * of tree items.
     * @param {Event} e The click event object.
     */
    handleKeyDown: function(e) {
      var itemToSelect;
      if (e.ctrlKey)
        return;

      var item = this.selectedItem;
      if (!item)
        return;

      var rtl = getComputedStyle(item).direction == 'rtl';

      switch (e.key) {
        case 'ArrowUp':
          itemToSelect =
              item ? getPrevious(item) : this.items[this.items.length - 1];
          break;
        case 'ArrowDown':
          itemToSelect = item ? getNext(item) : this.items[0];
          break;
        case 'ArrowLeft':
        case 'ArrowRight':
          // Don't let back/forward keyboard shortcuts be used.
          if (!cr.isMac && e.altKey || cr.isMac && e.metaKey)
            break;

          if (e.key == 'ArrowLeft' && !rtl || e.key == 'ArrowRight' && rtl) {
            if (item.expanded)
              item.expanded = false;
            else
              itemToSelect = findTreeItem(item.parentNode);
          } else {
            if (!item.expanded)
              item.expanded = true;
            else
              itemToSelect = item.items[0];
          }
          break;
        case 'Home':
          itemToSelect = this.items[0];
          break;
        case 'End':
          itemToSelect = this.items[this.items.length - 1];
          break;
      }

      if (itemToSelect) {
        itemToSelect.selected = true;
        e.preventDefault();
      }
    },

    /**
     * The selected tree item or null if none.
     * @type {cr.ui.TreeItem}
     */
    get selectedItem() {
      return this.selectedItem_ || null;
    },
    set selectedItem(item) {
      var oldSelectedItem = this.selectedItem_;
      if (oldSelectedItem != item) {
        // Set the selectedItem_ before deselecting the old item since we only
        // want one change when moving between items.
        this.selectedItem_ = item;

        if (oldSelectedItem)
          oldSelectedItem.selected = false;

        if (item) {
          item.selected = true;
          if (item.id)
            this.setAttribute('aria-activedescendant', item.id);
        } else {
          this.removeAttribute('aria-activedescendant');
        }
        cr.dispatchSimpleEvent(this, 'change');
      }
    },

    /**
     * @return {!ClientRect} The rect to use for the context menu.
     */
    getRectForContextMenu: function() {
      // TODO(arv): Add trait support so we can share more code between trees
      // and lists.
      if (this.selectedItem)
        return this.selectedItem.rowElement.getBoundingClientRect();
      return this.getBoundingClientRect();
    }
  };

  /**
   * Determines the visibility of icons next to the treeItem labels. If set to
   * 'hidden', no space is reserved for icons and no icons are displayed next
   * to treeItem labels. If set to 'parent', folder icons will be displayed
   * next to expandable parent nodes. If set to 'all' folder icons will be
   * displayed next to all nodes. Icons can be set using the treeItem's icon
   * property.
   */
  cr.defineProperty(Tree, 'iconVisibility', cr.PropertyKind.ATTR);

  /**
   * Incremental counter for an auto generated ID of the tree item. This will
   * be incremented per element, so each element never share same ID.
   *
   * @type {number}
   */
  var treeItemAutoGeneratedIdCounter = 0;

  /**
   * This is used as a blueprint for new tree item elements.
   * @type {!HTMLElement}
   */
  var treeItemProto = (function() {
    var treeItem = cr.doc.createElement('div');
    treeItem.className = 'tree-item';
    treeItem.innerHTML = '<div class="tree-row">' +
        '<span class="expand-icon"></span>' +
        '<span class="tree-label"></span>' +
        '</div>' +
        '<div class="tree-children" role="group"></div>';
    treeItem.setAttribute('role', 'treeitem');
    return treeItem;
  })();

  /**
   * Creates a new tree item.
   * @param {Object=} opt_propertyBag Optional properties.
   * @constructor
   * @extends {HTMLElement}
   */
  var TreeItem = cr.ui.define(function() {
    var treeItem = treeItemProto.cloneNode(true);
    treeItem.id = 'tree-item-autogen-id-' + treeItemAutoGeneratedIdCounter++;
    return treeItem;
  });

  TreeItem.prototype = {
    __proto__: HTMLElement.prototype,

    /**
     * Initializes the element.
     */
    decorate: function() {
      var labelId =
          'tree-item-label-autogen-id-' + treeItemAutoGeneratedIdCounter;
      this.labelElement.id = labelId;
      this.setAttribute('aria-labelledby', labelId);
    },

    /**
     * The tree items children.
     */
    get items() {
      return this.lastElementChild.children;
    },

    /**
     * The depth of the tree item.
     * @type {number}
     */
    depth_: 0,
    get depth() {
      return this.depth_;
    },

    /**
     * Sets the depth.
     * @param {number} depth The new depth.
     * @private
     */
    setDepth_: function(depth) {
      if (depth != this.depth_) {
        this.rowElement.style.WebkitPaddingStart =
            Math.max(0, depth - 1) * INDENT + 'px';
        this.depth_ = depth;
        var items = this.items;
        for (var i = 0, item; item = items[i]; i++) {
          item.setDepth_(depth + 1);
        }
      }
    },

    /**
     * Adds a tree item as a child.
     * @param {!cr.ui.TreeItem} child The child to add.
     */
    add: function(child) {
      this.addAt(child, 0xffffffff);
    },

    /**
     * Adds a tree item as a child at a given index.
     * @param {!cr.ui.TreeItem} child The child to add.
     * @param {number} index The index where to add the child.
     */
    addAt: function(child, index) {
      this.lastElementChild.insertBefore(child, this.items[index]);
      if (this.items.length == 1)
        this.hasChildren = true;
      child.setDepth_(this.depth + 1);
    },

    /**
     * Removes a child.
     * @param {!cr.ui.TreeItem=} child The tree item child to remove.
     * @override
     */
    remove: function(child) {
      // If we removed the selected item we should become selected.
      var tree = this.tree;
      var selectedItem = tree.selectedItem;
      if (selectedItem && child.contains(selectedItem))
        this.selected = true;

      this.lastElementChild.removeChild(/** @type {!cr.ui.TreeItem} */ (child));
      if (this.items.length == 0)
        this.hasChildren = false;
    },

    /**
     * The parent tree item.
     * @type {!cr.ui.Tree|cr.ui.TreeItem}
     */
    get parentItem() {
      var p = this.parentNode;
      while (p && !(p instanceof TreeItem) && !(p instanceof Tree)) {
        p = p.parentNode;
      }
      return p;
    },

    /**
     * The tree that the tree item belongs to or null of no added to a tree.
     * @type {cr.ui.Tree}
     */
    get tree() {
      var t = this.parentItem;
      while (t && !(t instanceof Tree)) {
        t = t.parentItem;
      }
      return t;
    },

    /**
     * Whether the tree item is expanded or not.
     * @type {boolean}
     */
    get expanded() {
      return this.hasAttribute('expanded');
    },
    set expanded(b) {
      if (this.expanded == b)
        return;

      var treeChildren = this.lastElementChild;

      if (b) {
        if (this.mayHaveChildren_) {
          this.setAttribute('expanded', '');
          this.setAttribute('aria-expanded', 'true');
          treeChildren.setAttribute('expanded', '');
          cr.dispatchSimpleEvent(this, 'expand', true);
          this.scrollIntoViewIfNeeded(false);
        }
      } else {
        var tree = this.tree;
        if (tree && !this.selected) {
          var oldSelected = tree.selectedItem;
          if (oldSelected && this.contains(oldSelected))
            this.selected = true;
        }
        this.removeAttribute('expanded');
        if (this.mayHaveChildren_)
          this.setAttribute('aria-expanded', 'false');
        else
          this.removeAttribute('aria-expanded');
        treeChildren.removeAttribute('expanded');
        cr.dispatchSimpleEvent(this, 'collapse', true);
      }
    },

    /**
     * Expands all parent items.
     */
    reveal: function() {
      var pi = this.parentItem;
      while (pi && !(pi instanceof Tree)) {
        pi.expanded = true;
        pi = pi.parentItem;
      }
    },

    /**
     * The element representing the row that gets highlighted.
     * @type {!HTMLElement}
     */
    get rowElement() {
      return this.firstElementChild;
    },

    /**
     * The element containing the label text and the icon.
     * @type {!HTMLElement}
     */
    get labelElement() {
      return this.firstElementChild.lastElementChild;
    },

    /**
     * The label text.
     * @type {string}
     */
    get label() {
      return this.labelElement.textContent;
    },
    set label(s) {
      this.labelElement.textContent = s;
    },

    /**
     * The URL for the icon.
     * @type {string}
     */
    get icon() {
      return getComputedStyle(this.labelElement).backgroundImage.slice(4, -1);
    },
    set icon(icon) {
      return this.labelElement.style.backgroundImage = getUrlForCss(icon);
    },

    /**
     * Whether the tree item is selected or not.
     * @type {boolean}
     */
    get selected() {
      return this.hasAttribute('selected');
    },
    set selected(b) {
      if (this.selected == b)
        return;
      var rowItem = this.firstElementChild;
      var tree = this.tree;
      if (b) {
        this.setAttribute('selected', '');
        rowItem.setAttribute('selected', '');
        this.reveal();
        this.labelElement.scrollIntoViewIfNeeded(false);
        if (tree)
          tree.selectedItem = this;
      } else {
        this.removeAttribute('selected');
        rowItem.removeAttribute('selected');
        if (tree && tree.selectedItem == this)
          tree.selectedItem = null;
      }
    },

    /**
     * Whether the tree item has children.
     * @type {boolean}
     */
    get mayHaveChildren_() {
      return this.hasAttribute('may-have-children');
    },
    set mayHaveChildren_(b) {
      var rowItem = this.firstElementChild;
      if (b) {
        this.setAttribute('may-have-children', '');
        rowItem.setAttribute('may-have-children', '');
      } else {
        this.removeAttribute('may-have-children');
        rowItem.removeAttribute('may-have-children');
      }
    },

    /**
     * Whether the tree item has children.
     * @type {boolean}
     */
    get hasChildren() {
      return !!this.items[0];
    },

    /**
     * Whether the tree item has children.
     * @type {boolean}
     */
    set hasChildren(b) {
      var rowItem = this.firstElementChild;
      this.setAttribute('has-children', b);
      rowItem.setAttribute('has-children', b);
      if (b) {
        this.mayHaveChildren_ = true;
        this.setAttribute('aria-expanded', 'false');
      }
    },

    /**
     * Called when the user clicks on a tree item. This is forwarded from the
     * cr.ui.Tree.
     * @param {Event} e The click event.
     */
    handleClick: function(e) {
      if (e.target.className == 'expand-icon')
        this.expanded = !this.expanded;
      else
        this.selected = true;
    },

    /**
     * Makes the tree item user editable. If the user renamed the item a
     * bubbling {@code rename} event is fired.
     * @type {boolean}
     */
    set editing(editing) {
      var oldEditing = this.editing;
      if (editing == oldEditing)
        return;

      var self = this;
      var labelEl = this.labelElement;
      var text = this.label;
      var input;

      // Handles enter and escape which trigger reset and commit respectively.
      function handleKeydown(e) {
        // Make sure that the tree does not handle the key.
        e.stopPropagation();

        // Calling tree.focus blurs the input which will make the tree item
        // non editable.
        switch (e.key) {
          case 'Escape':
            input.value = text;
          // fall through
          case 'Enter':
            self.tree.focus();
        }
      }

      function stopPropagation(e) {
        e.stopPropagation();
      }

      if (editing) {
        this.selected = true;
        this.setAttribute('editing', '');
        this.draggable = false;

        // We create an input[type=text] and copy over the label value. When
        // the input loses focus we set editing to false again.
        input = this.ownerDocument.createElement('input');
        input.value = text;
        if (labelEl.firstChild)
          labelEl.replaceChild(input, labelEl.firstChild);
        else
          labelEl.appendChild(input);

        input.addEventListener('keydown', handleKeydown);
        input.addEventListener('blur', (function() {
                                         this.editing = false;
                                       }).bind(this));

        // Make sure that double clicks do not expand and collapse the tree
        // item.
        var eventsToStop = ['mousedown', 'mouseup', 'contextmenu', 'dblclick'];
        eventsToStop.forEach(function(type) {
          input.addEventListener(type, stopPropagation);
        });

        // Wait for the input element to recieve focus before sizing it.
        var rowElement = this.rowElement;
        var onFocus = function() {
          input.removeEventListener('focus', onFocus);
          // 20 = the padding and border of the tree-row
          cr.ui.limitInputWidth(input, rowElement, 100);
        };
        input.addEventListener('focus', onFocus);
        input.focus();
        input.select();

        this.oldLabel_ = text;
      } else {
        this.removeAttribute('editing');
        this.draggable = true;
        input = labelEl.firstChild;
        var value = input.value;
        if (/^\s*$/.test(value)) {
          labelEl.textContent = this.oldLabel_;
        } else {
          labelEl.textContent = value;
          if (value != this.oldLabel_) {
            cr.dispatchSimpleEvent(this, 'rename', true);
          }
        }
        delete this.oldLabel_;
      }
    },

    get editing() {
      return this.hasAttribute('editing');
    }
  };

  /**
   * Helper function that returns the next visible tree item.
   * @param {cr.ui.TreeItem} item The tree item.
   * @return {cr.ui.TreeItem} The found item or null.
   */
  function getNext(item) {
    if (item.expanded) {
      var firstChild = item.items[0];
      if (firstChild) {
        return firstChild;
      }
    }

    return getNextHelper(item);
  }

  /**
   * Another helper function that returns the next visible tree item.
   * @param {cr.ui.TreeItem} item The tree item.
   * @return {cr.ui.TreeItem} The found item or null.
   */
  function getNextHelper(item) {
    if (!item)
      return null;

    var nextSibling = item.nextElementSibling;
    if (nextSibling)
      return assertInstanceof(nextSibling, cr.ui.TreeItem);
    return getNextHelper(item.parentItem);
  }

  /**
   * Helper function that returns the previous visible tree item.
   * @param {cr.ui.TreeItem} item The tree item.
   * @return {cr.ui.TreeItem} The found item or null.
   */
  function getPrevious(item) {
    var previousSibling = item.previousElementSibling;
    if (previousSibling)
      return getLastHelper(assertInstanceof(previousSibling, cr.ui.TreeItem));
    return item.parentItem;
  }

  /**
   * Helper function that returns the last visible tree item in the subtree.
   * @param {cr.ui.TreeItem} item The item to find the last visible item for.
   * @return {cr.ui.TreeItem} The found item or null.
   */
  function getLastHelper(item) {
    if (!item)
      return null;
    if (item.expanded && item.hasChildren) {
      var lastChild = item.items[item.items.length - 1];
      return getLastHelper(lastChild);
    }
    return item;
  }

  // Export
  return {Tree: Tree, TreeItem: TreeItem};
});
