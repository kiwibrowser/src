// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cr.ui', function() {

  /**
   * Returns the TabBox for a Tab or a TabPanel.
   * @param {Tab|TabPanel} el The tab or tabpanel element.
   * @return {TabBox} The tab box if found.
   */
  function getTabBox(el) {
    return findAncestor(el, function(node) {
      return node.tagName == 'TABBOX';
    });
  }

  /**
   * Returns whether an element is a tab related object.
   * @param {HTMLElement} el The element whose tag is being checked
   * @return {boolean} Whether the element is a tab related element.
   */
  function isTabElement(el) {
    return el.tagName == 'TAB' || el.tagName == 'TABPANEL';
  }

  /**
   * Set hook for the selected property for Tab and TabPanel.
   * This sets the selectedIndex on the parent TabBox.
   * @param {boolean} newValue The new selected value
   * @param {boolean} oldValue The old selected value. (This is ignored atm.)
   * @this {Tab|TabPanel}
   */
  function selectedSetHook(newValue, oldValue) {
    var tabBox;
    if (newValue && (tabBox = getTabBox(this)))
      tabBox.selectedIndex = Array.prototype.indexOf.call(p.children, this);
  }

  /**
   * Decorates all the children of an element.
   * @this {HTMLElement}
   */
  function decorateChildren() {
    var map = {
      TABBOX: TabBox,
      TABS: Tabs,
      TAB: Tab,
      TABPANELS: TabPanels,
      TABPANEL: TabPanel
    };

    Object.keys(map).forEach(function(tagName) {
      var children = this.getElementsByTagName(tagName);
      var constr = map[tagName];
      for (var i = 0; child = children[i]; i++) {
        cr.ui.decorate(child, constr);
      }
    }.bind(this));
  }

  /**
   * Set hook for TabBox selectedIndex.
   * @param {number} selectedIndex The new selected index.
   * @this {TabBox}
   */
  function selectedIndexSetHook(selectedIndex) {
    var child, tabChild, element;
    element = this.querySelector('tabs');
    if (element) {
      for (var i = 0; child = element.children[i]; i++) {
        child.selected = i == selectedIndex;
      }
    }

    element = this.querySelector('tabpanels');
    if (element) {
      for (var i = 0; child = element.children[i]; i++) {
        child.selected = i == selectedIndex;
      }
    }
  }

  /**
   * Creates a new tabbox element.
   * @param {Object=} opt_propertyBag Optional properties.
   * @constructor
   * @extends {HTMLElement}
   */
  var TabBox = cr.ui.define('tabbox');

  TabBox.prototype = {
    __proto__: HTMLElement.prototype,
    decorate: function() {
      decorateChildren.call(this);
      this.addEventListener('selectedChange', this.handleSelectedChange_, true);
      this.selectedIndex = 0;
    },

    /**
     * Callback for when a Tab or TabPanel changes its selected property.
     * @param {Event} e The property change event.
     * @private
     */
    handleSelectedChange_: function(e) {
      var target = e.target;
      if (e.newValue && isTabElement(target) && getTabBox(target) == this) {
        var index =
            Array.prototype.indexOf.call(target.parentElement.children, target);
        this.selectedIndex = index;
      }
    },

    selectedIndex_: -1
  };

  /**
   * The index of the selected tab or -1 if no tab is selected.
   * @type {number}
   */
  cr.defineProperty(
      TabBox, 'selectedIndex', cr.PropertyKind.JS_PROP, selectedIndexSetHook);

  /**
   * Creates a new tabs element.
   * @param {string} opt_label The text label for the item.
   * @constructor
   * @extends {HTMLElement}
   */
  var Tabs = cr.ui.define('tabs');
  Tabs.prototype = {
    __proto__: HTMLElement.prototype,
    decorate: function() {
      decorateChildren.call(this);

      // Make the Tabs element focusable.
      this.tabIndex = 0;
      this.addEventListener('keydown', this.handleKeyDown_.bind(this));

      // Get (and initializes a focus outline manager.
      this.focusOutlineManager_ =
          cr.ui.FocusOutlineManager.forDocument(this.ownerDocument);
    },

    /**
     * Handle keydown to change the selected tab when the user presses the
     * arrow keys.
     * @param {Event} e The keyboard event.
     * @private
     */
    handleKeyDown_: function(e) {
      var delta = 0;
      switch (e.key) {
        case 'ArrowLeft':
        case 'ArrowUp':
          delta = -1;
          break;
        case 'ArrowRight':
        case 'ArrowDown':
          delta = 1;
          break;
      }

      if (!delta)
        return;

      var cs = this.ownerDocument.defaultView.getComputedStyle(this);
      if (cs.direction == 'rtl')
        delta *= -1;

      var count = this.children.length;
      var tabbox = getTabBox(this);
      var index = tabbox.selectedIndex;
      tabbox.selectedIndex = (index + delta + count) % count;

      // Show focus outline since we used the keyboard.
      this.focusOutlineManager_.visible = true;
    }
  };

  /**
   * Creates a new tab element.
   * @param {string} opt_label The text label for the item.
   * @constructor
   * @extends {HTMLElement}
   */
  var Tab = cr.ui.define('tab');
  Tab.prototype = {
    __proto__: HTMLElement.prototype,
    decorate: function() {
      var self = this;
      this.addEventListener(cr.isMac ? 'click' : 'mousedown', function() {
        self.selected = true;
      });
    }
  };

  /**
   * Whether the tab is selected.
   * @type {boolean}
   */
  cr.defineProperty(Tab, 'selected', cr.PropertyKind.BOOL_ATTR);

  /**
   * Creates a new tabpanels element.
   * @param {string} opt_label The text label for the item.
   * @constructor
   * @extends {HTMLElement}
   */
  var TabPanels = cr.ui.define('tabpanels');
  TabPanels.prototype = {
    __proto__: HTMLElement.prototype,
    decorate: decorateChildren
  };

  /**
   * Creates a new tabpanel element.
   * @param {string} opt_label The text label for the item.
   * @constructor
   * @extends {HTMLElement}
   */
  var TabPanel = cr.ui.define('tabpanel');
  TabPanel.prototype = {
    __proto__: HTMLElement.prototype,
    decorate: function() {}
  };

  /**
   * Whether the tab is selected.
   * @type {boolean}
   */
  cr.defineProperty(TabPanel, 'selected', cr.PropertyKind.BOOL_ATTR);

  return {
    TabBox: TabBox,
    Tabs: Tabs,
    Tab: Tab,
    TabPanels: TabPanels,
    TabPanel: TabPanel
  };
});
