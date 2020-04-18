// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Module of functions which produce a new page state in response
 * to an action. Reducers (in the same sense as Array.prototype.reduce) must be
 * pure functions: they must not modify existing state objects, or make any API
 * calls.
 */

cr.define('bookmarks', function() {
  const SelectionState = {};

  /**
   * @param {SelectionState} selectionState
   * @param {Object} action
   * @return {SelectionState}
   */
  SelectionState.selectItems = function(selectionState, action) {
    let newItems = new Set();
    if (!action.clear)
      newItems = new Set(selectionState.items);

    action.items.forEach(function(id) {
      let add = true;
      if (action.toggle)
        add = !newItems.has(id);

      if (add)
        newItems.add(id);
      else
        newItems.delete(id);
    });

    return /** @type {SelectionState} */ (Object.assign({}, selectionState, {
      items: newItems,
      anchor: action.anchor,
    }));
  };

  /**
   * @param {SelectionState} selectionState
   * @return {SelectionState}
   */
  SelectionState.deselectAll = function(selectionState) {
    return {
      items: new Set(),
      anchor: null,
    };
  };

  /**
   * @param {SelectionState} selectionState
   * @param {!Set<string>} deleted
   * @return SelectionState
   */
  SelectionState.deselectItems = function(selectionState, deleted) {
    return /** @type {SelectionState} */ (Object.assign({}, selectionState, {
      items: bookmarks.util.removeIdsFromSet(selectionState.items, deleted),
      anchor: !selectionState.anchor || deleted.has(selectionState.anchor) ?
          null :
          selectionState.anchor,
    }));
  };

  /**
   * @param {SelectionState} selectionState
   * @param {Object} action
   * @return {SelectionState}
   */
  SelectionState.updateAnchor = function(selectionState, action) {
    return /** @type {SelectionState} */ (Object.assign({}, selectionState, {
      anchor: action.anchor,
    }));
  };

  /**
   * @param {SelectionState} selection
   * @param {Object} action
   * @return {SelectionState}
   */
  SelectionState.updateSelection = function(selection, action) {
    switch (action.name) {
      case 'clear-search':
      case 'finish-search':
      case 'select-folder':
      case 'deselect-items':
        return SelectionState.deselectAll(selection);
      case 'select-items':
        return SelectionState.selectItems(selection, action);
      case 'remove-bookmark':
        return SelectionState.deselectItems(selection, action.descendants);
      case 'move-bookmark':
        // Deselect items when they are moved to another folder, since they will
        // no longer be visible on screen (for simplicity, ignores items visible
        // in search results).
        if (action.parentId != action.oldParentId &&
            selection.items.has(action.id)) {
          return SelectionState.deselectItems(selection, new Set([action.id]));
        }
        return selection;
      case 'update-anchor':
        return SelectionState.updateAnchor(selection, action);
      default:
        return selection;
    }
  };

  const SearchState = {};

  /**
   * @param {SearchState} search
   * @param {Object} action
   * @return {SearchState}
   */
  SearchState.startSearch = function(search, action) {
    return {
      term: action.term,
      inProgress: true,
      results: search.results,
    };
  };

  /**
   * @param {SearchState} search
   * @param {Object} action
   * @return {SearchState}
   */
  SearchState.finishSearch = function(search, action) {
    return /** @type {SearchState} */ (Object.assign({}, search, {
      inProgress: false,
      results: action.results,
    }));
  };

  /** @return {SearchState} */
  SearchState.clearSearch = function() {
    return {
      term: '',
      inProgress: false,
      results: null,
    };
  };

  /**
   * @param {SearchState} search
   * @param {!Set<string>} deletedIds
   * @return {SearchState}
   */
  SearchState.removeDeletedResults = function(search, deletedIds) {
    if (!search.results)
      return search;

    const newResults = [];
    search.results.forEach(function(id) {
      if (!deletedIds.has(id))
        newResults.push(id);
    });
    return /** @type {SearchState} */ (Object.assign({}, search, {
      results: newResults,
    }));
  };

  /**
   * @param {SearchState} search
   * @param {Object} action
   * @return {SearchState}
   */
  SearchState.updateSearch = function(search, action) {
    switch (action.name) {
      case 'start-search':
        return SearchState.startSearch(search, action);
      case 'select-folder':
      case 'clear-search':
        return SearchState.clearSearch();
      case 'finish-search':
        return SearchState.finishSearch(search, action);
      case 'remove-bookmark':
        return SearchState.removeDeletedResults(search, action.descendants);
      default:
        return search;
    }
  };

  const NodeState = {};

  /**
   * @param {NodeMap} nodes
   * @param {string} id
   * @param {function(BookmarkNode):BookmarkNode} callback
   * @return {NodeMap}
   */
  NodeState.modifyNode_ = function(nodes, id, callback) {
    const nodeModification = {};
    nodeModification[id] = callback(nodes[id]);
    return Object.assign({}, nodes, nodeModification);
  };

  /**
   * @param {NodeMap} nodes
   * @param {Object} action
   * @return {NodeMap}
   */
  NodeState.createBookmark = function(nodes, action) {
    const nodeModifications = {};
    nodeModifications[action.id] = action.node;

    const parentNode = nodes[action.parentId];
    const newChildren = parentNode.children.slice();
    newChildren.splice(action.parentIndex, 0, action.id);
    nodeModifications[action.parentId] = Object.assign({}, parentNode, {
      children: newChildren,
    });

    return Object.assign({}, nodes, nodeModifications);
  };

  /**
   * @param {NodeMap} nodes
   * @param {Object} action
   * @return {NodeMap}
   */
  NodeState.editBookmark = function(nodes, action) {
    // Do not allow folders to change URL (making them no longer folders).
    if (!nodes[action.id].url && action.changeInfo.url)
      delete action.changeInfo.url;

    return NodeState.modifyNode_(nodes, action.id, function(node) {
      return /** @type {BookmarkNode} */ (
          Object.assign({}, node, action.changeInfo));
    });
  };

  /**
   * @param {NodeMap} nodes
   * @param {Object} action
   * @return {NodeMap}
   */
  NodeState.moveBookmark = function(nodes, action) {
    const nodeModifications = {};
    const id = action.id;

    // Change node's parent.
    nodeModifications[id] =
        Object.assign({}, nodes[id], {parentId: action.parentId});

    // Remove from old parent.
    const oldParentId = action.oldParentId;
    const oldParentChildren = nodes[oldParentId].children.slice();
    oldParentChildren.splice(action.oldIndex, 1);
    nodeModifications[oldParentId] =
        Object.assign({}, nodes[oldParentId], {children: oldParentChildren});

    // Add to new parent.
    const parentId = action.parentId;
    const parentChildren = oldParentId == parentId ?
        oldParentChildren :
        nodes[parentId].children.slice();
    parentChildren.splice(action.index, 0, action.id);
    nodeModifications[parentId] =
        Object.assign({}, nodes[parentId], {children: parentChildren});

    return Object.assign({}, nodes, nodeModifications);
  };

  /**
   * @param {NodeMap} nodes
   * @param {Object} action
   * @return {NodeMap}
   */
  NodeState.removeBookmark = function(nodes, action) {
    const newState =
        NodeState.modifyNode_(nodes, action.parentId, function(node) {
          const newChildren = node.children.slice();
          newChildren.splice(action.index, 1);
          return /** @type {BookmarkNode} */ (
              Object.assign({}, node, {children: newChildren}));
        });

    return bookmarks.util.removeIdsFromObject(newState, action.descendants);
  };

  /**
   * @param {NodeMap} nodes
   * @param {Object} action
   * @return {NodeMap}
   */
  NodeState.reorderChildren = function(nodes, action) {
    return NodeState.modifyNode_(nodes, action.id, function(node) {
      return /** @type {BookmarkNode} */ (
          Object.assign({}, node, {children: action.children}));
    });
  };

  /**
   * @param {NodeMap} nodes
   * @param {Object} action
   * @return {NodeMap}
   */
  NodeState.updateNodes = function(nodes, action) {
    switch (action.name) {
      case 'create-bookmark':
        return NodeState.createBookmark(nodes, action);
      case 'edit-bookmark':
        return NodeState.editBookmark(nodes, action);
      case 'move-bookmark':
        return NodeState.moveBookmark(nodes, action);
      case 'remove-bookmark':
        return NodeState.removeBookmark(nodes, action);
      case 'reorder-children':
        return NodeState.reorderChildren(nodes, action);
      case 'refresh-nodes':
        return action.nodes;
      default:
        return nodes;
    }
  };

  const SelectedFolderState = {};

  /**
   * @param {NodeMap} nodes
   * @param {string} ancestorId
   * @param {string} childId
   * @return {boolean}
   */
  SelectedFolderState.isAncestorOf = function(nodes, ancestorId, childId) {
    let currentId = childId;
    // Work upwards through the tree from child.
    while (currentId) {
      if (currentId == ancestorId)
        return true;
      currentId = nodes[currentId].parentId;
    }
    return false;
  };

  /**
   * @param {string} selectedFolder
   * @param {Object} action
   * @param {NodeMap} nodes
   * @return {string}
   */
  SelectedFolderState.updateSelectedFolder = function(
      selectedFolder, action, nodes) {
    switch (action.name) {
      case 'select-folder':
        return action.id;
      case 'change-folder-open':
        // When hiding the selected folder by closing its ancestor, select
        // that ancestor instead.
        if (!action.open && selectedFolder &&
            SelectedFolderState.isAncestorOf(
                nodes, action.id, selectedFolder)) {
          return action.id;
        }
        return selectedFolder;
      case 'remove-bookmark':
        // When deleting the selected folder (or its ancestor), select the
        // parent of the deleted node.
        if (selectedFolder &&
            SelectedFolderState.isAncestorOf(nodes, action.id, selectedFolder))
          return assert(nodes[action.id].parentId);
        return selectedFolder;
      default:
        return selectedFolder;
    }
  };

  const FolderOpenState = {};

  /**
   * @param {FolderOpenState} folderOpenState
   * @param {string|undefined} id
   * @param {NodeMap} nodes
   * @return {FolderOpenState}
   */
  FolderOpenState.openFolderAndAncestors = function(
      folderOpenState, id, nodes) {
    const newFolderOpenState =
        /** @type {FolderOpenState} */ (new Map(folderOpenState));
    for (let currentId = id; currentId; currentId = nodes[currentId].parentId)
      newFolderOpenState.set(currentId, true);

    return newFolderOpenState;
  };

  /**
   * @param {FolderOpenState} folderOpenState
   * @param {Object} action
   * @return {FolderOpenState}
   */
  FolderOpenState.changeFolderOpen = function(folderOpenState, action) {
    const newFolderOpenState =
        /** @type {FolderOpenState} */ (new Map(folderOpenState));
    newFolderOpenState.set(action.id, action.open);

    return newFolderOpenState;
  };

  /**
   * @param {FolderOpenState} folderOpenState
   * @param {Object} action
   * @param {NodeMap} nodes
   * @return {FolderOpenState}
   */
  FolderOpenState.updateFolderOpenState = function(
      folderOpenState, action, nodes) {
    switch (action.name) {
      case 'change-folder-open':
        return FolderOpenState.changeFolderOpen(folderOpenState, action);
      case 'select-folder':
        return FolderOpenState.openFolderAndAncestors(
            folderOpenState, nodes[action.id].parentId, nodes);
      case 'move-bookmark':
        if (!nodes[action.id].children)
          return folderOpenState;

        return FolderOpenState.openFolderAndAncestors(
            folderOpenState, action.parentId, nodes);
      case 'remove-bookmark':
        return bookmarks.util.removeIdsFromMap(
            folderOpenState, action.descendants);
      default:
        return folderOpenState;
    }
  };

  const PreferencesState = {};

  /**
   * @param {PreferencesState} prefs
   * @param {Object} action
   * @return {PreferencesState}
   */
  PreferencesState.updatePrefs = function(prefs, action) {
    switch (action.name) {
      case 'set-incognito-availability':
        return /** @type {PreferencesState} */ (Object.assign({}, prefs, {
          incognitoAvailability: action.value,
        }));
      case 'set-can-edit':
        return /** @type {PreferencesState} */ (Object.assign({}, prefs, {
          canEdit: action.value,
        }));
      default:
        return prefs;
    }
  };

  /**
   * Root reducer for the Bookmarks page. This is called by the store in
   * response to an action, and the return value is used to update the UI.
   * @param {!BookmarksPageState} state
   * @param {Object} action
   * @return {!BookmarksPageState}
   */
  function reduceAction(state, action) {
    return {
      nodes: NodeState.updateNodes(state.nodes, action),
      selectedFolder: SelectedFolderState.updateSelectedFolder(
          state.selectedFolder, action, state.nodes),
      folderOpenState: FolderOpenState.updateFolderOpenState(
          state.folderOpenState, action, state.nodes),
      prefs: PreferencesState.updatePrefs(state.prefs, action),
      search: SearchState.updateSearch(state.search, action),
      selection: SelectionState.updateSelection(state.selection, action),
    };
  }

  return {
    reduceAction: reduceAction,
    FolderOpenState: FolderOpenState,
    NodeState: NodeState,
    PreferencesState: PreferencesState,
    SearchState: SearchState,
    SelectedFolderState: SelectedFolderState,
    SelectionState: SelectionState,
  };
});
