// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Module for functions which produce action objects. These are
 * listed in one place to document available actions and their parameters.
 */

cr.define('bookmarks.actions', function() {
  /**
   * @param {string} id
   * @param {BookmarkTreeNode} treeNode
   */
  function createBookmark(id, treeNode) {
    return {
      name: 'create-bookmark',
      id: id,
      parentId: treeNode.parentId,
      parentIndex: treeNode.index,
      node: bookmarks.util.normalizeNode(treeNode),
    };
  }

  /**
   * @param {string} id
   * @param {{title: string, url: (string|undefined)}} changeInfo
   * @return {!Action}
   */
  function editBookmark(id, changeInfo) {
    return {
      name: 'edit-bookmark',
      id: id,
      changeInfo: changeInfo,
    };
  }

  /**
   * @param {string} id
   * @param {string} parentId
   * @param {number} index
   * @param {string} oldParentId
   * @param {number} oldIndex
   * @return {!Action}
   */
  function moveBookmark(id, parentId, index, oldParentId, oldIndex) {
    return {
      name: 'move-bookmark',
      id: id,
      parentId: parentId,
      index: index,
      oldParentId: oldParentId,
      oldIndex: oldIndex,
    };
  }

  /**
   * @param {string} id
   * @param {!Array<string>} newChildIds
   */
  function reorderChildren(id, newChildIds) {
    return {
      name: 'reorder-children',
      id: id,
      children: newChildIds,
    };
  }

  /**
   * @param {string} id
   * @param {string} parentId
   * @param {number} index
   * @param {NodeMap} nodes
   * @return {!Action}
   */
  function removeBookmark(id, parentId, index, nodes) {
    const descendants = bookmarks.util.getDescendants(nodes, id);
    return {
      name: 'remove-bookmark',
      id: id,
      descendants: descendants,
      parentId: parentId,
      index: index,
    };
  }

  /**
   * @param {NodeMap} nodeMap
   * @return {!Action}
   */
  function refreshNodes(nodeMap) {
    return {
      name: 'refresh-nodes',
      nodes: nodeMap,
    };
  }

  /**
   * @param {string} id
   * @param {NodeMap} nodes Current node state. Can be ommitted in tests.
   * @return {?Action}
   */
  function selectFolder(id, nodes) {
    if (nodes && (id == ROOT_NODE_ID || !nodes[id] || nodes[id].url)) {
      console.warn('Tried to select invalid folder: ' + id);
      return null;
    }

    return {
      name: 'select-folder',
      id: id,
    };
  }

  /**
   * @param {string} id
   * @param {boolean} open
   * @return {!Action}
   */
  function changeFolderOpen(id, open) {
    return {
      name: 'change-folder-open',
      id: id,
      open: open,
    };
  }

  /** @return {!Action} */
  function clearSearch() {
    return {
      name: 'clear-search',
    };
  }

  /** @return {!Action} */
  function deselectItems() {
    return {
      name: 'deselect-items',
    };
  }

  /**
   * @param {string} id
   * @param {BookmarksPageState} state
   * @param {{
   *     clear: boolean,
   *     range: boolean,
   *     toggle: boolean}} config Options for how the selection should change:
   *   - clear: If true, clears the previous selection before adding this one
   *   - range: If true, selects all items from the anchor to this item
   *   - toggle: If true, toggles the selection state of the item. Cannot be
   *     used with clear or range.
   * @return {!Action}
   */
  function selectItem(id, state, config) {
    assert(!config.toggle || !config.range);
    assert(!config.toggle || !config.clear);

    const anchor = state.selection.anchor;
    const toSelect = [];
    let newAnchor = id;

    if (config.range && anchor) {
      const displayedList = bookmarks.util.getDisplayedList(state);
      const selectedIndex = displayedList.indexOf(id);
      assert(selectedIndex != -1);
      let anchorIndex = displayedList.indexOf(anchor);
      if (anchorIndex == -1)
        anchorIndex = selectedIndex;

      // When performing a range selection, don't change the anchor from what
      // was used in this selection.
      newAnchor = displayedList[anchorIndex];

      const startIndex = Math.min(anchorIndex, selectedIndex);
      const endIndex = Math.max(anchorIndex, selectedIndex);

      for (let i = startIndex; i <= endIndex; i++)
        toSelect.push(displayedList[i]);
    } else {
      toSelect.push(id);
    }

    return {
      name: 'select-items',
      clear: config.clear,
      toggle: config.toggle,
      anchor: newAnchor,
      items: toSelect,
    };
  }

  /**
   * @param {Array<string>} ids
   * @param {BookmarksPageState} state
   * @param {string=} anchor
   * @return {!Action}
   */
  function selectAll(ids, state, anchor) {
    return {
      name: 'select-items',
      clear: true,
      toggle: false,
      anchor: anchor ? anchor : state.selection.anchor,
      items: ids,
    };
  }

  /**
   * @param {string} id
   * @return {!Action}
   */
  function updateAnchor(id) {
    return {
      name: 'update-anchor',
      anchor: id,
    };
  }

  /**
   * @param {string} term
   * @return {!Action}
   */
  function setSearchTerm(term) {
    if (!term)
      return clearSearch();

    return {
      name: 'start-search',
      term: term,
    };
  }

  /**
   * @param {!Array<string>} ids
   * @return {!Action}
   */
  function setSearchResults(ids) {
    return {
      name: 'finish-search',
      results: ids,
    };
  }

  /**
   * @param {IncognitoAvailability} availability
   * @return {!Action}
   */
  function setIncognitoAvailability(availability) {
    assert(availability != IncognitoAvailability.FORCED);
    return {
      name: 'set-incognito-availability',
      value: availability,
    };
  }

  /**
   * @param {boolean} canEdit
   * @return {!Action}
   */
  function setCanEditBookmarks(canEdit) {
    return {
      name: 'set-can-edit',
      value: canEdit,
    };
  }

  return {
    changeFolderOpen: changeFolderOpen,
    clearSearch: clearSearch,
    createBookmark: createBookmark,
    deselectItems: deselectItems,
    editBookmark: editBookmark,
    moveBookmark: moveBookmark,
    refreshNodes: refreshNodes,
    removeBookmark: removeBookmark,
    reorderChildren: reorderChildren,
    selectAll: selectAll,
    selectFolder: selectFolder,
    selectItem: selectItem,
    setCanEditBookmarks: setCanEditBookmarks,
    setIncognitoAvailability: setIncognitoAvailability,
    setSearchResults: setSearchResults,
    setSearchTerm: setSearchTerm,
    updateAnchor: updateAnchor,
  };
});
