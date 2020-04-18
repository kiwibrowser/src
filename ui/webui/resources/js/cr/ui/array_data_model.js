// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview This is a data model representin
 */

// The include directives are put into Javascript-style comments to prevent
// parsing errors in non-flattened mode. The flattener still sees them.
// Note that this makes the flattener to comment out the first line of the
// included file but that's all right since any javascript file should start
// with a copyright comment anyway.

// <include src="../../assert.js">

cr.define('cr.ui', function() {
  /** @const */ var EventTarget = cr.EventTarget;

  /**
   * A data model that wraps a simple array and supports sorting by storing
   * initial indexes of elements for each position in sorted array.
   * @param {!Array} array The underlying array.
   * @constructor
   * @extends {cr.EventTarget}
   */
  function ArrayDataModel(array) {
    this.array_ = array;
    this.indexes_ = [];
    this.compareFunctions_ = {};

    for (var i = 0; i < array.length; i++) {
      this.indexes_.push(i);
    }
  }

  ArrayDataModel.prototype = {
    __proto__: EventTarget.prototype,

    /**
     * The length of the data model.
     * @type {number}
     */
    get length() {
      return this.array_.length;
    },

    /**
     * Returns the item at the given index.
     * This implementation returns the item at the given index in the sorted
     * array.
     * @param {number} index The index of the element to get.
     * @return {*} The element at the given index.
     */
    item: function(index) {
      if (index >= 0 && index < this.length)
        return this.array_[this.indexes_[index]];
      return undefined;
    },

    /**
     * Returns compare function set for given field.
     * @param {string} field The field to get compare function for.
     * @return {function(*, *): number} Compare function set for given field.
     */
    compareFunction: function(field) {
      return this.compareFunctions_[field];
    },

    /**
     * Sets compare function for given field.
     * @param {string} field The field to set compare function.
     * @param {function(*, *): number} compareFunction Compare function to set
     *     for given field.
     */
    setCompareFunction: function(field, compareFunction) {
      if (!this.compareFunctions_) {
        this.compareFunctions_ = {};
      }
      this.compareFunctions_[field] = compareFunction;
    },

    /**
     * Returns true if the field has a compare function.
     * @param {string} field The field to check.
     * @return {boolean} True if the field is sortable.
     */
    isSortable: function(field) {
      return this.compareFunctions_ && field in this.compareFunctions_;
    },

    /**
     * Returns current sort status.
     * @return {!Object} Current sort status.
     */
    get sortStatus() {
      if (this.sortStatus_) {
        return this.createSortStatus(
            this.sortStatus_.field, this.sortStatus_.direction);
      } else {
        return this.createSortStatus(null, null);
      }
    },

    /**
     * Returns the first matching item.
     * @param {*} item The item to find.
     * @param {number=} opt_fromIndex If provided, then the searching start at
     *     the {@code opt_fromIndex}.
     * @return {number} The index of the first found element or -1 if not found.
     */
    indexOf: function(item, opt_fromIndex) {
      for (var i = opt_fromIndex || 0; i < this.indexes_.length; i++) {
        if (item === this.item(i))
          return i;
      }
      return -1;
    },

    /**
     * Returns an array of elements in a selected range.
     * @param {number=} opt_from The starting index of the selected range.
     * @param {number=} opt_to The ending index of selected range.
     * @return {Array} An array of elements in the selected range.
     */
    slice: function(opt_from, opt_to) {
      var arr = this.array_;
      return this.indexes_.slice(opt_from, opt_to).map(function(index) {
        return arr[index];
      });
    },

    /**
     * This removes and adds items to the model.
     * This dispatches a splice event.
     * This implementation runs sort after splice and creates permutation for
     * the whole change.
     * @param {number} index The index of the item to update.
     * @param {number} deleteCount The number of items to remove.
     * @param {...*} var_args The items to add.
     * @return {!Array} An array with the removed items.
     */
    splice: function(index, deleteCount, var_args) {
      var addCount = arguments.length - 2;
      var newIndexes = [];
      var deletePermutation = [];
      var deletedItems = [];
      var newArray = [];
      index = Math.min(index, this.indexes_.length);
      deleteCount = Math.min(deleteCount, this.indexes_.length - index);
      // Copy items before the insertion point.
      for (var i = 0; i < index; i++) {
        newIndexes.push(newArray.length);
        deletePermutation.push(i);
        newArray.push(this.array_[this.indexes_[i]]);
      }
      // Delete items.
      for (; i < index + deleteCount; i++) {
        deletePermutation.push(-1);
        deletedItems.push(this.array_[this.indexes_[i]]);
      }
      // Insert new items instead deleted ones.
      for (var j = 0; j < addCount; j++) {
        newIndexes.push(newArray.length);
        newArray.push(arguments[j + 2]);
      }
      // Copy items after the insertion point.
      for (; i < this.indexes_.length; i++) {
        newIndexes.push(newArray.length);
        deletePermutation.push(i - deleteCount + addCount);
        newArray.push(this.array_[this.indexes_[i]]);
      }

      this.indexes_ = newIndexes;

      this.array_ = newArray;

      // TODO(arv): Maybe unify splice and change events?
      var spliceEvent = new Event('splice');
      spliceEvent.removed = deletedItems;
      spliceEvent.added = Array.prototype.slice.call(arguments, 2);

      var status = this.sortStatus;
      // if sortStatus.field is null, this restores original order.
      var sortPermutation =
          this.doSort_(this.sortStatus.field, this.sortStatus.direction);
      if (sortPermutation) {
        var splicePermutation = deletePermutation.map(function(element) {
          return element != -1 ? sortPermutation[element] : -1;
        });
        this.dispatchPermutedEvent_(splicePermutation);
        spliceEvent.index = sortPermutation[index];
      } else {
        this.dispatchPermutedEvent_(deletePermutation);
        spliceEvent.index = index;
      }

      this.dispatchEvent(spliceEvent);

      // If real sorting is needed, we should first call prepareSort (data may
      // change), and then sort again.
      // Still need to finish the sorting above (including events), so
      // list will not go to inconsistent state.
      if (status.field)
        this.delayedSort_(status.field, status.direction);

      return deletedItems;
    },

    /**
     * Appends items to the end of the model.
     *
     * This dispatches a splice event.
     *
     * @param {...*} var_args The items to append.
     * @return {number} The new length of the model.
     */
    push: function(var_args) {
      var args = Array.prototype.slice.call(arguments);
      args.unshift(this.length, 0);
      this.splice.apply(this, args);
      return this.length;
    },

    /**
     * Updates the existing item with the new item.
     *
     * The existing item and the new item are regarded as the same item and the
     * permutation tracks these indexes.
     *
     * @param {*} oldItem Old item that is contained in the model. If the item
     *     is not found in the model, the method call is just ignored.
     * @param {*} newItem New item.
     */
    replaceItem: function(oldItem, newItem) {
      var index = this.indexOf(oldItem);
      if (index < 0)
        return;
      this.array_[this.indexes_[index]] = newItem;
      this.updateIndex(index);
    },

    /**
     * Use this to update a given item in the array. This does not remove and
     * reinsert a new item.
     * This dispatches a change event.
     * This runs sort after updating.
     * @param {number} index The index of the item to update.
     */
    updateIndex: function(index) {
      this.updateIndexes([index]);
    },

    /**
     * Notifies of update of the items in the array. This does not remove and
     * reinsert new items.
     * This dispatches one or more change events.
     * This runs sort after updating.
     * @param {Array<number>} indexes The index list of items to update.
     */
    updateIndexes: function(indexes) {
      indexes.forEach(function(index) {
        assert(index >= 0 && index < this.length, 'Invalid index');
      }, this);

      for (var i = 0; i < indexes.length; i++) {
        var e = new Event('change');
        e.index = indexes[i];
        this.dispatchEvent(e);
      }

      if (this.sortStatus.field) {
        var status = this.sortStatus;
        var sortPermutation =
            this.doSort_(this.sortStatus.field, this.sortStatus.direction);
        if (sortPermutation)
          this.dispatchPermutedEvent_(sortPermutation);
        // We should first call prepareSort (data may change), and then sort.
        // Still need to finish the sorting above (including events), so
        // list will not go to inconsistent state.
        this.delayedSort_(status.field, status.direction);
      }
    },

    /**
     * Creates sort status with given field and direction.
     * @param {?string} field Sort field.
     * @param {?string} direction Sort direction.
     * @return {!Object} Created sort status.
     */
    createSortStatus: function(field, direction) {
      return {field: field, direction: direction};
    },

    /**
     * Called before a sort happens so that you may fetch additional data
     * required for the sort.
     *
     * @param {string} field Sort field.
     * @param {function()} callback The function to invoke when preparation
     *     is complete.
     */
    prepareSort: function(field, callback) {
      callback();
    },

    /**
     * Sorts data model according to given field and direction and dispathes
     * sorted event with delay. If no need to delay, use sort() instead.
     * @param {string} field Sort field.
     * @param {string} direction Sort direction.
     * @private
     */
    delayedSort_: function(field, direction) {
      var self = this;
      setTimeout(function() {
        // If the sort status has been changed, sorting has already done
        // on the change event.
        if (field == self.sortStatus.field &&
            direction == self.sortStatus.direction) {
          self.sort(field, direction);
        }
      }, 0);
    },

    /**
     * Sorts data model according to given field and direction and dispathes
     * sorted event.
     * @param {string} field Sort field.
     * @param {string} direction Sort direction.
     */
    sort: function(field, direction) {
      var self = this;

      this.prepareSort(field, function() {
        var sortPermutation = self.doSort_(field, direction);
        if (sortPermutation)
          self.dispatchPermutedEvent_(sortPermutation);
        self.dispatchSortEvent_();
      });
    },

    /**
     * Sorts data model according to given field and direction.
     * @param {string} field Sort field.
     * @param {string} direction Sort direction.
     * @private
     */
    doSort_: function(field, direction) {
      var compareFunction = this.sortFunction_(field, direction);
      var positions = [];
      for (var i = 0; i < this.length; i++) {
        positions[this.indexes_[i]] = i;
      }
      var sorted = this.indexes_.every(function(element, index, array) {
        return index == 0 || compareFunction(element, array[index - 1]) >= 0;
      });
      if (!sorted)
        this.indexes_.sort(compareFunction);
      this.sortStatus_ = this.createSortStatus(field, direction);
      var sortPermutation = [];
      var changed = false;
      for (var i = 0; i < this.length; i++) {
        if (positions[this.indexes_[i]] != i)
          changed = true;
        sortPermutation[positions[this.indexes_[i]]] = i;
      }
      if (changed)
        return sortPermutation;
      return null;
    },

    dispatchSortEvent_: function() {
      var e = new Event('sorted');
      this.dispatchEvent(e);
    },

    dispatchPermutedEvent_: function(permutation) {
      var e = new Event('permuted');
      e.permutation = permutation;
      e.newLength = this.length;
      this.dispatchEvent(e);
    },

    /**
     * Creates compare function for the field.
     * Returns the function set as sortFunction for given field
     * or default compare function
     * @param {string} field Sort field.
     * @return {function(*, *): number} Compare function.
     * @private
     */
    createCompareFunction_: function(field) {
      var compareFunction =
          this.compareFunctions_ ? this.compareFunctions_[field] : null;
      var defaultValuesCompareFunction = this.defaultValuesCompareFunction;
      if (compareFunction) {
        return compareFunction;
      } else {
        return function(a, b) {
          return defaultValuesCompareFunction.call(null, a[field], b[field]);
        };
      }
    },

    /**
     * Creates compare function for given field and direction.
     * @param {string} field Sort field.
     * @param {string} direction Sort direction.
     * @private
     */
    sortFunction_: function(field, direction) {
      var compareFunction = null;
      if (field !== null)
        compareFunction = this.createCompareFunction_(field);
      var dirMultiplier = direction == 'desc' ? -1 : 1;

      return function(index1, index2) {
        var item1 = this.array_[index1];
        var item2 = this.array_[index2];

        var compareResult = 0;
        if (typeof(compareFunction) === 'function')
          compareResult = compareFunction.call(null, item1, item2);
        if (compareResult != 0)
          return dirMultiplier * compareResult;
        return dirMultiplier *
            this.defaultValuesCompareFunction(index1, index2);
      }.bind(this);
    },

    /**
     * Default compare function.
     */
    defaultValuesCompareFunction: function(a, b) {
      // We could insert i18n comparisons here.
      if (a < b)
        return -1;
      if (a > b)
        return 1;
      return 0;
    }
  };

  return {ArrayDataModel: ArrayDataModel};
});
