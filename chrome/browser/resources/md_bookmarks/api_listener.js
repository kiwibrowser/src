// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Listener functions which translate events from the
 * chrome.bookmarks API into actions to modify the local page state.
 */

cr.define('bookmarks.ApiListener', function() {

  /** @type {boolean} */
  let trackUpdates = false;
  /** @type {!Array<string>} */
  let updatedItems = [];

  let debouncer;

  /**
   * Batches UI updates so that no changes will be made to UI until the next
   * task after the last call to this method. This is useful for listeners which
   * can be called in a tight loop by UI actions.
   */
  function batchUIUpdates() {
    if (!debouncer) {
      debouncer = new bookmarks.Debouncer(
          () => bookmarks.Store.getInstance().endBatchUpdate());
    }

    if (debouncer.done()) {
      bookmarks.Store.getInstance().beginBatchUpdate();
      debouncer.reset();
    }

    debouncer.restartTimeout();
  }

  /**
   * Tracks any items that are created or moved.
   */
  function trackUpdatedItems() {
    trackUpdates = true;
  }

  function highlightUpdatedItemsImpl() {
    if (!trackUpdates)
      return;

    document.dispatchEvent(new CustomEvent('highlight-items', {
      detail: updatedItems,
    }));
    updatedItems = [];
    trackUpdates = false;
  }

  /**
   * Highlights any items that have been updated since |trackUpdatedItems| was
   * called. Should be called after a user action causes new items to appear in
   * the main list.
   */
  function highlightUpdatedItems() {
    // Ensure that the items are highlighted after the current batch update (if
    // there is one) is completed.
    assert(debouncer);
    debouncer.promise.then(highlightUpdatedItemsImpl);
  }

  /** @param {Action} action */
  function dispatch(action) {
    bookmarks.Store.getInstance().dispatch(action);
  }

  /**
   * @param {string} id
   * @param {{title: string, url: (string|undefined)}} changeInfo
   */
  function onBookmarkChanged(id, changeInfo) {
    dispatch(bookmarks.actions.editBookmark(id, changeInfo));
  }

  /**
   * @param {string} id
   * @param {BookmarkTreeNode} treeNode
   */
  function onBookmarkCreated(id, treeNode) {
    batchUIUpdates();
    if (trackUpdates)
      updatedItems.push(id);
    dispatch(bookmarks.actions.createBookmark(id, treeNode));
  }

  /**
   * @param {string} id
   * @param {{parentId: string, index: number}} removeInfo
   */
  function onBookmarkRemoved(id, removeInfo) {
    batchUIUpdates();
    const nodes = bookmarks.Store.getInstance().data.nodes;
    dispatch(bookmarks.actions.removeBookmark(
        id, removeInfo.parentId, removeInfo.index, nodes));
  }

  /**
   * @param {string} id
   * @param {{
   *    parentId: string,
   *    index: number,
   *    oldParentId: string,
   *    oldIndex: number
   * }} moveInfo
   */
  function onBookmarkMoved(id, moveInfo) {
    batchUIUpdates();
    if (trackUpdates)
      updatedItems.push(id);
    dispatch(bookmarks.actions.moveBookmark(
        id, moveInfo.parentId, moveInfo.index, moveInfo.oldParentId,
        moveInfo.oldIndex));
  }

  /**
   * @param {string} id
   * @param {{childIds: !Array<string>}} reorderInfo
   */
  function onChildrenReordered(id, reorderInfo) {
    dispatch(bookmarks.actions.reorderChildren(id, reorderInfo.childIds));
  }

  /**
   * Pauses the Created handler during an import. The imported nodes will all be
   * loaded at once when the import is finished.
   */
  function onImportBegan() {
    chrome.bookmarks.onCreated.removeListener(onBookmarkCreated);
  }

  function onImportEnded() {
    chrome.bookmarks.getTree(function(results) {
      dispatch(bookmarks.actions.refreshNodes(
          bookmarks.util.normalizeNodes(results[0])));
    });
    chrome.bookmarks.onCreated.addListener(onBookmarkCreated);
  }

  /**
   * @param {IncognitoAvailability} availability
   */
  function onIncognitoAvailabilityChanged(availability) {
    dispatch(bookmarks.actions.setIncognitoAvailability(availability));
  }

  /**
   * @param {boolean} canEdit
   */
  function onCanEditBookmarksChanged(canEdit) {
    dispatch(bookmarks.actions.setCanEditBookmarks(canEdit));
  }

  const listeners = [
    {api: chrome.bookmarks.onChanged, fn: onBookmarkChanged},
    {api: chrome.bookmarks.onChildrenReordered, fn: onChildrenReordered},
    {api: chrome.bookmarks.onCreated, fn: onBookmarkCreated},
    {api: chrome.bookmarks.onMoved, fn: onBookmarkMoved},
    {api: chrome.bookmarks.onRemoved, fn: onBookmarkRemoved},
    {api: chrome.bookmarks.onImportBegan, fn: onImportBegan},
    {api: chrome.bookmarks.onImportEnded, fn: onImportEnded},
  ];

  function init() {
    listeners.forEach((listener) => listener.api.addListener(listener.fn));

    cr.sendWithPromise('getIncognitoAvailability')
        .then(onIncognitoAvailabilityChanged);
    cr.addWebUIListener(
        'incognito-availability-changed', onIncognitoAvailabilityChanged);

    cr.sendWithPromise('getCanEditBookmarks').then(onCanEditBookmarksChanged);
    cr.addWebUIListener(
        'can-edit-bookmarks-changed', onCanEditBookmarksChanged);
  }

  function destroy() {
    listeners.forEach((listener) => listener.api.removeListener(listener.fn));
    cr.removeWebUIListener(
        'incognito-availability-changed', onIncognitoAvailabilityChanged);
    cr.removeWebUIListener(
        'can-edit-bookmarks-changed', onCanEditBookmarksChanged);
  }

  return {
    init: init,
    destroy: destroy,
    trackUpdatedItems: trackUpdatedItems,
    highlightUpdatedItems: highlightUpdatedItems,
  };
});
