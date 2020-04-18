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

/**
 * Representation of zero or more items in a list. Each instance of ItemHierarchy should be capable
 * of being wrapped in ItemAdapter and be displayed.
 *
 * For example, {@link com.android.setupwizardlib.items.Item} is a representation of a single item,
 * typically with data provided from XML. {@link com.android.setupwizardlib.items.ItemGroup}
 * represents a list of child item hierarchies it contains, but itself does not do any display.
 */
public interface ItemHierarchy {

    /**
     * Observer for any changes in this hierarchy. If anything updated that causes this hierarchy to
     * show different content, this observer should be called.
     */
    interface Observer {
        /**
         * Called when an underlying data update that can cause this hierarchy to show different
         * content has occurred.
         */
        void onChanged(ItemHierarchy itemHierarchy);
    }

    /**
     * Register an observer to observe changes for this item hierarchy.
     */
    void registerObserver(Observer observer);

    /**
     * Unregister a previously registered observer.
     */
    void unregisterObserver(Observer observer);

    /**
     * @return the number of items this item hierarchy represent.
     */
    int getCount();

    /**
     * Get the item at position.
     *
     * @param position An integer from 0 to {@link #getCount()}}, which indicates the position in
     *                 this item hierarchy to get the child item.
     * @return A representation of the item at {@code position}. Must not be {@code null}.
     */
    IItem getItemAt(int position);

    /**
     * Find an item hierarchy within this hierarchy which has the given ID. Or null if no match is
     * found. This hierarchy will be returned if our ID matches. Same restrictions for Android
     * resource IDs apply to this ID. In fact, typically this ID is a resource ID generated from
     * XML.
     *
     * @param id An ID to search for in this item hierarchy.
     * @return An ItemHierarchy which matches the given ID.
     */
    ItemHierarchy findItemById(int id);
}
