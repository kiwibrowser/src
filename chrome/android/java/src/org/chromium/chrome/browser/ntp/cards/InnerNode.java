// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.annotation.Nullable;

import org.chromium.base.Callback;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder.PartialBindCallback;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * An inner node in the tree: the root of a subtree, with a list of child nodes.
 */
public class InnerNode extends ChildNode implements NodeParent {
    private final List<TreeNode> mChildren = new ArrayList<>();

    private int getChildIndexForPosition(int position) {
        checkIndex(position);
        int numItems = 0;
        int numChildren = mChildren.size();
        for (int i = 0; i < numChildren; i++) {
            numItems += mChildren.get(i).getItemCount();
            if (position < numItems) return i;
        }
        // checkIndex() will have caught this case already.
        assert false;
        return -1;
    }

    private int getStartingOffsetForChildIndex(int childIndex) {
        if (childIndex < 0 || childIndex >= mChildren.size()) {
            throw new IndexOutOfBoundsException(childIndex + "/" + mChildren.size());
        }

        int offset = 0;
        for (int i = 0; i < childIndex; i++) {
            offset += mChildren.get(i).getItemCount();
        }
        return offset;
    }

    int getStartingOffsetForChild(TreeNode child) {
        return getStartingOffsetForChildIndex(mChildren.indexOf(child));
    }

    /**
     * Returns the child whose subtree contains the item at the given position.
     */
    TreeNode getChildForPosition(int position) {
        return mChildren.get(getChildIndexForPosition(position));
    }

    @Override
    protected int getItemCountForDebugging() {
        int numItems = 0;
        for (TreeNode child : mChildren) {
            numItems += child.getItemCount();
        }
        return numItems;
    }

    @Override
    @ItemViewType
    public int getItemViewType(int position) {
        int index = getChildIndexForPosition(position);
        return mChildren.get(index).getItemViewType(
                position - getStartingOffsetForChildIndex(index));
    }

    @Override
    public void onBindViewHolder(NewTabPageViewHolder holder, int position) {
        int index = getChildIndexForPosition(position);
        mChildren.get(index).onBindViewHolder(
                holder, position - getStartingOffsetForChildIndex(index));
    }

    @Override
    public Set<Integer> getItemDismissalGroup(int position) {
        int index = getChildIndexForPosition(position);
        int offset = getStartingOffsetForChildIndex(index);
        Set<Integer> itemDismissalGroup =
                getChildren().get(index).getItemDismissalGroup(position - offset);
        return offsetBy(itemDismissalGroup, offset);
    }

    @Override
    public void dismissItem(int position, Callback<String> itemRemovedCallback) {
        int index = getChildIndexForPosition(position);
        getChildren().get(index).dismissItem(
                position - getStartingOffsetForChildIndex(index), itemRemovedCallback);
    }

    @Override
    public void visitItems(NodeVisitor visitor) {
        for (TreeNode child : getChildren()) {
            child.visitItems(visitor);
        }
    }

    @Override
    public void onItemRangeChanged(
            TreeNode child, int index, int count, @Nullable PartialBindCallback callback) {
        notifyItemRangeChanged(getStartingOffsetForChild(child) + index, count, callback);
    }

    @Override
    public void onItemRangeInserted(TreeNode child, int index, int count) {
        notifyItemRangeInserted(getStartingOffsetForChild(child) + index, count);
    }

    @Override
    public void onItemRangeRemoved(TreeNode child, int index, int count) {
        notifyItemRangeRemoved(getStartingOffsetForChild(child) + index, count);
    }

    /**
     * Helper method that adds a new child node and notifies about its insertion.
     *
     * @param child The child node to be added.
     */
    protected void addChild(TreeNode child) {
        int insertedIndex = getItemCount();
        mChildren.add(child);
        child.setParent(this);

        int count = child.getItemCount();
        if (count > 0) notifyItemRangeInserted(insertedIndex, count);
    }

    /**
     * Helper method that adds all the children and notifies about the inserted items.
     */
    protected void addChildren(TreeNode... children) {
        int initialCount = getItemCount();
        int addedCount = 0;
        for (TreeNode child : children) {
            mChildren.add(child);
            child.setParent(this);
            addedCount += child.getItemCount();
        }

        if (addedCount > 0) notifyItemRangeInserted(initialCount, addedCount);
    }

    /**
     * Helper method that removes a child node and notifies about the removed items.
     *
     * @param child The child node to be removed.
     */
    protected void removeChild(TreeNode child) {
        int removedIndex = mChildren.indexOf(child);
        if (removedIndex == -1) throw new IndexOutOfBoundsException();

        int count = child.getItemCount();
        int childStartingOffset = getStartingOffsetForChildIndex(removedIndex);

        child.detach();
        mChildren.remove(removedIndex);
        if (count > 0) notifyItemRangeRemoved(childStartingOffset, count);
    }

    /**
     * Helper method that removes all the children and notifies about the removed items.
     */
    protected void removeChildren() {
        int itemCount = getItemCount();
        if (itemCount == 0) return;

        for (TreeNode child : mChildren) child.detach();
        mChildren.clear();
        notifyItemRangeRemoved(0, itemCount);
    }

    @VisibleForTesting
    final List<TreeNode> getChildren() {
        return mChildren;
    }

    /**
     * Helper method that adds an offset value to a set of integers.
     * @param set a set of integers
     * @param offset the offset to add to the set.
     * @return a set of integers with the {@code offset} value added to the input {@code set}.
     */
    private static Set<Integer> offsetBy(Set<Integer> set, int offset) {
        // Optimizations for empty and singleton sets:
        if (set.isEmpty()) return Collections.emptySet();
        if (set.size() == 1) {
            return Collections.singleton(set.iterator().next() + offset);
        }

        Set<Integer> offsetSet = new HashSet<>();
        for (int value : set) {
            offsetSet.add(value + offset);
        }
        return offsetSet;
    }
}
