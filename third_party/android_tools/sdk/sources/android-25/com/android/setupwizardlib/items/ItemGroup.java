/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.setupwizardlib.items;

import android.content.Context;
import android.util.AttributeSet;
import android.util.SparseIntArray;

import java.util.ArrayList;
import java.util.List;

public class ItemGroup extends AbstractItemHierarchy implements ItemInflater.ItemParent,
        ItemHierarchy.Observer {

    /* static section */

    /**
     * Binary search for the closest value that's smaller than or equal to {@code value}, and
     * return the corresponding key.
     */
    private static int binarySearch(SparseIntArray array, int value) {
        final int size = array.size();
        int lo = 0;
        int hi = size - 1;

        while (lo <= hi) {
            final int mid = (lo + hi) >>> 1;
            final int midVal = array.valueAt(mid);

            if (midVal < value) {
                lo = mid + 1;
            } else if (midVal > value) {
                hi = mid - 1;
            } else {
                return array.keyAt(mid);  // value found
            }
        }
        // Value not found. Return the last item before our search range, which is the closest
        // value smaller than the value we are looking for.
        return array.keyAt(lo - 1);
    }

    /* non-static section */

    private List<ItemHierarchy> mChildren = new ArrayList<>();

    /**
     * A mapping from the index of an item hierarchy in mChildren, to the first position in which
     * the corresponding child hierarchy represents. For example:
     *
     *   ItemHierarchy                 Item               Item Position
     *       Index
     * 
     *         0            [         Wi-Fi AP 1       ]        0
     *                      |         Wi-Fi AP 2       |        1
     *                      |         Wi-Fi AP 3       |        2
     *                      |         Wi-Fi AP 4       |        3
     *                      [         Wi-Fi AP 5       ]        4
     * 
     *         1            [  <Empty Item Hierarchy>  ]
     * 
     *         2            [     Use cellular data    ]        5
     * 
     *         3            [       Don't connect      ]        6
     * 
     * For this example of Wi-Fi screen, the following mapping will be produced:
     *     [ 0 -> 0 | 2 -> 5 | 3 -> 6 ]
     * 
     * Also note how ItemHierarchy index 1 is not present in the map, because it is empty.
     *
     * ItemGroup uses this map to look for which ItemHierarchy an item at a given position belongs
     * to.
     */
    private SparseIntArray mHierarchyStart = new SparseIntArray();

    private int mCount = 0;
    private boolean mDirty = false;

    public ItemGroup() {
        super();
    }

    public ItemGroup(Context context, AttributeSet attrs) {
        // Constructor for XML inflation
        super(context, attrs);
    }

    /**
     * Add a child hierarchy to this item group.
     */
    @Override
    public void addChild(ItemHierarchy child) {
        mChildren.add(child);
        child.registerObserver(this);
        onHierarchyChanged();
    }

    /**
     * Remove a previously added child from this item group.
     *
     * @return True if there is a match for the child and it is removed. False if the child could
     *         not be found in our list of child hierarchies.
     */
    public boolean removeChild(ItemHierarchy child) {
        if (mChildren.remove(child)) {
            child.unregisterObserver(this);
            onHierarchyChanged();
            return true;
        }
        return false;
    }

    /**
     * Remove all children from this hierarchy.
     */
    public void clear() {
        if (mChildren.size() == 0) {
            return;
        }

        for (ItemHierarchy item : mChildren) {
            item.unregisterObserver(this);
        }
        mChildren.clear();
        onHierarchyChanged();
    }

    @Override
    public int getCount() {
        updateDataIfNeeded();
        return mCount;
    }

    @Override
    public IItem getItemAt(int position) {
        int itemIndex = getItemIndex(position);
        ItemHierarchy item = mChildren.get(itemIndex);
        int subpos = position - mHierarchyStart.get(itemIndex);
        return item.getItemAt(subpos);
    }

    @Override
    public void onChanged(ItemHierarchy hierarchy) {
        // Need to set dirty, because our children may have gotten more items.
        mDirty = true;
        notifyChanged();
    }

    private void onHierarchyChanged() {
        onChanged(null);
    }

    @Override
    public ItemHierarchy findItemById(int id) {
        if (id == getId()) {
            return this;
        }
        for (ItemHierarchy child : mChildren) {
            ItemHierarchy childFindItem = child.findItemById(id);
            if (childFindItem != null) {
                return childFindItem;
            }
        }
        return null;
    }

    /**
     * If dirty, this method will recalculate the number of items and mHierarchyStart.
     */
    private void updateDataIfNeeded() {
        if (mDirty) {
            mCount = 0;
            mHierarchyStart.clear();
            for (int itemIndex = 0; itemIndex < mChildren.size(); itemIndex++) {
                ItemHierarchy item = mChildren.get(itemIndex);
                if (item.getCount() > 0) {
                    mHierarchyStart.put(itemIndex, mCount);
                }
                mCount += item.getCount();
            }
            mDirty = false;
        }
    }

    /**
     * Use binary search to locate the item hierarchy a position is contained in.
     *
     * @return Index of the item hierarchy which is responsible for the item at {@code position}.
     */
    private int getItemIndex(int position) {
        updateDataIfNeeded();
        if (position < 0 || position >= mCount) {
            throw new IndexOutOfBoundsException("size=" + mCount + "; index=" + position);
        }
        int result = binarySearch(mHierarchyStart, position);
        if (result < 0) {
            throw new IllegalStateException("Cannot have item start index < 0");
        }
        return result;
    }
}
