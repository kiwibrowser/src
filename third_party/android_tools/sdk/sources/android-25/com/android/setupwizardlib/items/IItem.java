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

import android.view.View;

/**
 * Representation of an item in an {@link ItemHierarchy}.
 */
public interface IItem {

    /**
     * Get the Android resource ID for locating the layout for this item.
     *
     * @return Resource ID for the layout of this item. This layout will be used to inflate the View
     *         passed to {@link #onBindView(android.view.View)}.
     */
    int getLayoutResource();

    /**
     * Called by items framework to display the data specified by this item. This method should
     * update {@code view} to reflect its data.
     *
     * @param view A view inflated from {@link #getLayoutResource()}, which should be updated to
     *             display data from this item. This view may be recycled from other items with the
     *             same layout resource.
     */
    void onBindView(View view);

    /**
     * @return True if this item is enabled.
     */
    boolean isEnabled();
}
