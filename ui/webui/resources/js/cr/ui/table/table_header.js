// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview This implements a table header.
 */

cr.define('cr.ui.table', function() {
  /** @const */ var TableSplitter = cr.ui.TableSplitter;

  /**
   * Creates a new table header.
   * @param {Object=} opt_propertyBag Optional properties.
   * @constructor
   * @extends {HTMLDivElement}
   */
  var TableHeader = cr.ui.define('div');

  TableHeader.prototype = {
    __proto__: HTMLDivElement.prototype,

    table_: null,

    /**
     * Initializes the element.
     */
    decorate: function() {
      this.className = 'table-header';

      this.headerInner_ = this.ownerDocument.createElement('div');
      this.headerInner_.className = 'table-header-inner';
      this.appendChild(this.headerInner_);
      this.addEventListener(
          'touchstart', this.handleTouchStart_.bind(this), false);
    },

    /**
     * Updates table header width. Header width depends on list having a
     * vertical scrollbar.
     */
    updateWidth: function() {
      // Header should not span over the vertical scrollbar of the list.
      var list = this.table_.querySelector('list');
      this.headerInner_.style.width = list.clientWidth + 'px';
    },

    /**
     * Resizes columns.
     */
    resize: function() {
      var headerCells = this.querySelectorAll('.table-header-cell');
      if (this.needsFullRedraw_(headerCells)) {
        this.redraw();
        return;
      }

      var cm = this.table_.columnModel;
      for (var i = 0; i < cm.size; i++) {
        headerCells[i].style.width = cm.getWidth(i) + 'px';
      }
      this.placeSplitters_(this.querySelectorAll('.table-header-splitter'));
    },

    batchCount_: 0,

    startBatchUpdates: function() {
      this.batchCount_++;
    },

    endBatchUpdates: function() {
      this.batchCount_--;
      if (this.batchCount_ == 0)
        this.redraw();
    },

    /**
     * Redraws table header.
     */
    redraw: function() {
      if (this.batchCount_ != 0)
        return;

      var cm = this.table_.columnModel;
      var dm = this.table_.dataModel;

      this.updateWidth();
      this.headerInner_.textContent = '';

      if (!cm || !dm) {
        return;
      }

      for (var i = 0; i < cm.size; i++) {
        var cell = this.ownerDocument.createElement('div');
        cell.style.width = cm.getWidth(i) + 'px';
        // Don't display cells for hidden columns. Don't omit the cell
        // completely, as it's much simpler if the number of cell elements and
        // columns are in sync.
        cell.hidden = !cm.isVisible(i);
        cell.className = 'table-header-cell';
        if (dm.isSortable(cm.getId(i)))
          cell.addEventListener(
              'click', this.createSortFunction_(i).bind(this));

        cell.appendChild(this.createHeaderLabel_(i));
        this.headerInner_.appendChild(cell);
      }
      this.appendSplitters_();
    },

    /**
     * Appends column splitters to the table header.
     */
    appendSplitters_: function() {
      var cm = this.table_.columnModel;
      var splitters = [];
      for (var i = 0; i < cm.size; i++) {
        // splitter should use CSS for background image.
        var splitter = new TableSplitter({table: this.table_});
        splitter.columnIndex = i;
        splitter.addEventListener(
            'dblclick', this.handleDblClick_.bind(this, i));
        // Don't display splitters for hidden columns.  Don't omit the splitter
        // completely, as it's much simpler if the number of splitter elements
        // and columns are in sync.
        splitter.hidden = !cm.isVisible(i);

        this.headerInner_.appendChild(splitter);
        splitters.push(splitter);
      }
      this.placeSplitters_(splitters);
    },

    /**
     * Place splitters to right positions.
     * @param {Array<HTMLElement>|NodeList} splitters Array of splitters.
     */
    placeSplitters_: function(splitters) {
      var cm = this.table_.columnModel;
      var place = 0;
      for (var i = 0; i < cm.size; i++) {
        // Don't account for the widths of hidden columns.
        if (splitters[i].hidden)
          continue;
        place += cm.getWidth(i);
        splitters[i].style.webkitMarginStart = place + 'px';
      }
    },

    /**
     * Renders column header. Appends text label and sort arrow if needed.
     * @param {number} index Column index.
     */
    createHeaderLabel_: function(index) {
      var cm = this.table_.columnModel;
      var dm = this.table_.dataModel;

      var labelDiv = this.ownerDocument.createElement('div');
      labelDiv.className = 'table-header-label';

      if (cm.isEndAlign(index))
        labelDiv.style.textAlign = 'end';
      var span = this.ownerDocument.createElement('span');
      span.appendChild(cm.renderHeader(index, this.table_));
      span.style.padding = '0';

      if (dm) {
        if (dm.sortStatus.field == cm.getId(index)) {
          if (dm.sortStatus.direction == 'desc')
            span.className = 'table-header-sort-image-desc';
          else
            span.className = 'table-header-sort-image-asc';
        }
      }
      labelDiv.appendChild(span);
      return labelDiv;
    },

    /**
     * Creates sort function for given column.
     * @param {number} index The index of the column to sort by.
     */
    createSortFunction_: function(index) {
      return function() {
        this.table_.sort(index);
      }.bind(this);
    },

    /**
     * Handles the touchstart event. If the touch happened close enough
     * to a splitter starts dragging.
     * @param {Event} e The touch event.
     */
    handleTouchStart_: function(e) {
      e = /** @type {TouchEvent} */ (e);
      if (e.touches.length != 1)
        return;
      var clientX = e.touches[0].clientX;

      var minDistance = TableHeader.TOUCH_DRAG_AREA_WIDTH;
      var candidate;

      var splitters = /** @type {NodeList<cr.ui.TableSplitter>} */ (
          this.querySelectorAll('.table-header-splitter'));
      for (var i = 0; i < splitters.length; i++) {
        var r = splitters[i].getBoundingClientRect();
        if (clientX <= r.left && r.left - clientX <= minDistance) {
          minDistance = r.left - clientX;
          candidate = splitters[i];
        }
        if (clientX >= r.right && clientX - r.right <= minDistance) {
          minDistance = clientX - r.right;
          candidate = splitters[i];
        }
      }
      if (candidate)
        candidate.startDrag(clientX, true);
      // Splitter itself shouldn't handle this event.
      e.stopPropagation();
    },

    /**
     * Handles the double click on a column separator event.
     * Adjusts column width.
     * @param {number} index Column index.
     * @param {Event} e The double click event.
     */
    handleDblClick_: function(index, e) {
      this.table_.fitColumn(index);
    },

    /**
     * Determines whether a full redraw is required.
     * @param {!NodeList} headerCells
     * @return {boolean}
     */
    needsFullRedraw_: function(headerCells) {
      var cm = this.table_.columnModel;
      // If the number of columns in the model has changed, a full redraw is
      // needed.
      if (headerCells.length != cm.size)
        return true;
      // If the column visibility has changed, a full redraw is required.
      for (var i = 0; i < cm.size; i++) {
        if (cm.isVisible(i) == headerCells[i].hidden)
          return true;
      }
      return false;
    },
  };

  /**
   * The table associated with the header.
   * @type {Element}
   */
  cr.defineProperty(TableHeader, 'table');

  /**
   * Rectangular area around the splitters sensitive to touch events
   * (in pixels).
   */
  TableHeader.TOUCH_DRAG_AREA_WIDTH = 30;

  return {TableHeader: TableHeader};
});
