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
 * limitations under the License
 */

package com.android.setupwizardlib.items;

import android.content.Context;

/**
 * Inflate {@link Item} hierarchies from XML files.
 *
 * Modified from android.support.v7.preference.PreferenceInflater
 */
public class ItemInflater extends GenericInflater<ItemHierarchy> {

    private static final String TAG = "ItemInflater";

    public interface ItemParent {
        void addChild(ItemHierarchy child);
    }

    private final Context mContext;

    public ItemInflater(Context context) {
        super(context);
        mContext = context;
        setDefaultPackage(Item.class.getPackage().getName() + ".");
    }

    @Override
    public ItemInflater cloneInContext(Context newContext) {
        return new ItemInflater(newContext);
    }

    /**
     * Return the context we are running in, for access to resources, class
     * loader, etc.
     */
    @Override
    public Context getContext() {
        return mContext;
    }

    @Override
    protected void onAddChildItem(ItemHierarchy parent, ItemHierarchy child) {
        if (parent instanceof ItemParent) {
            ((ItemParent) parent).addChild(child);
        } else {
            throw new IllegalArgumentException("Cannot add child item to " + parent);
        }
    }
}
