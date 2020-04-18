// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Class that represents a UI component.
   * @constructor
   * @extends {cr.EventTarget}
   */
  function Component() {
    cr.EventTarget.call(this);

    /**
     * Component's HTML element.
     * @protected {Element}
     */
    this.element_ = null;

    this.isInDocument_ = false;

    /**
     * Component's event tracker.
     * @protected {!EventTracker}
     */
    this.tracker_ = new EventTracker();

    /**
     * Component's WebUI listener tracker.
     * @private {!WebUIListenerTracker}
     */
    this.listenerTracker_ = new WebUIListenerTracker();

    /**
     * Child components of the component.
     * @private {!Array<!print_preview.Component>}
     */
    this.children_ = [];
  }

  Component.prototype = {
    __proto__: cr.EventTarget.prototype,

    /** Gets the component's element. */
    getElement: function() {
      return this.element_;
    },

    /** @return {!EventTracker} Component's event tracker. */
    get tracker() {
      return this.tracker_;
    },

    /** @return {!WebUIListenerTracker} Component's Web UI listener tracker. */
    get listenerTracker() {
      return this.listenerTracker_;
    },

    /**
     * @return {boolean} Whether the element of the component is already in the
     *     HTML document.
     */
    get isInDocument() {
      return this.isInDocument_;
    },

    /**
     * Creates the root element of the component. Sub-classes should override
     * this method.
     */
    createDom: function() {
      this.element_ = cr.doc.createElement('div');
    },

    /**
     * Called when the component's element is known to be in the document.
     * Anything using document.getElementById etc. should be done at this stage.
     * Sub-classes should extend this method and attach listeners.
     */
    enterDocument: function() {
      this.isInDocument_ = true;
      this.children_.forEach(function(child) {
        if (!child.isInDocument && child.getElement()) {
          child.enterDocument();
        }
      });
    },

    /** Removes all event listeners. */
    exitDocument: function() {
      this.children_.forEach(function(child) {
        if (child.isInDocument) {
          child.exitDocument();
        }
      });
      this.tracker_.removeAll();
      this.listenerTracker_.removeAll();
      this.isInDocument_ = false;
    },

    /**
     * Renders this UI component and appends the element to the given parent
     * element.
     * @param {!Element} parentElement Element to render the component's
     *     element into.
     */
    render: function(parentElement) {
      assert(!this.isInDocument, 'Component is already in the document');
      if (!this.element_) {
        this.createDom();
      }
      parentElement.appendChild(this.element_);
      this.enterDocument();
    },

    /**
     * Decorates an existing DOM element. Sub-classes should override the
     * override the decorateInternal method.
     * @param {Element} element Element to decorate.
     */
    decorate: function(element) {
      assert(!this.isInDocument, 'Component is already in the document');
      this.setElementInternal(element);
      this.decorateInternal();
      this.enterDocument();
    },

    /**
     * @return {!Array<!print_preview.Component>} Child components of this
     *     component.
     */
    get children() {
      return this.children_;
    },

    /**
     * @param {!print_preview.Component} child Component to add as a child of
     *     this component.
     */
    addChild: function(child) {
      this.children_.push(child);
    },

    /**
     * @param {!print_preview.Component} child Component to remove from this
     *     component's children.
     */
    removeChild: function(child) {
      const childIdx = this.children_.indexOf(child);
      if (childIdx != -1) {
        this.children_.splice(childIdx, 1);
      }
      if (child.isInDocument) {
        child.exitDocument();
        if (child.getElement()) {
          child.getElement().parentNode.removeChild(child.getElement());
        }
      }
    },

    /** Removes all of the component's children. */
    removeChildren: function() {
      while (this.children_.length > 0) {
        this.removeChild(this.children_[0]);
      }
    },

    /**
     * @param {string} query Selector query to select an element starting from
     *     the component's root element using a depth first search for the first
     *     element that matches the query.
     * @return {!HTMLElement} Element selected by the given query.
     */
    getChildElement: function(query) {
      return /** @type {!HTMLElement} */ (
          assert(this.element_.querySelector(query)));
    },

    /**
     * Sets the component's element.
     * @param {Element} element HTML element to set as the component's element.
     * @protected
     */
    setElementInternal: function(element) {
      this.element_ = element;
    },

    /**
     * Decorates the given element for use as the element of the component.
     * @protected
     */
    decorateInternal: function() { /*abstract*/ },

    /**
     * Clones a template HTML DOM tree.
     * @param {string} templateId Template element ID.
     * @param {boolean=} opt_keepHidden Whether to leave the cloned template
     *     hidden after cloning.
     * @return {Element} Cloned element with its 'id' attribute stripped.
     * @protected
     */
    cloneTemplateInternal: function(templateId, opt_keepHidden) {
      const templateEl = $(templateId);
      assert(
          templateEl != null, 'Could not find element with ID: ' + templateId);
      const el = assertInstanceof(templateEl.cloneNode(true), HTMLElement);
      el.id = '';
      if (!opt_keepHidden) {
        setIsVisible(el, true);
      }
      return el;
    }
  };

  return {Component: Component};
});
