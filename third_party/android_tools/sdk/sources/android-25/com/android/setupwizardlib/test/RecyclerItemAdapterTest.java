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

package com.android.setupwizardlib.test;

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.setupwizardlib.items.Item;
import com.android.setupwizardlib.items.ItemGroup;
import com.android.setupwizardlib.items.ItemHierarchy;
import com.android.setupwizardlib.items.RecyclerItemAdapter;

import java.util.Arrays;
import java.util.HashSet;

public class RecyclerItemAdapterTest extends AndroidTestCase {

    private Item[] mItems = new Item[5];
    private ItemGroup mItemGroup = new ItemGroup();

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        for (int i = 0; i < 5; i++) {
            Item item = new Item();
            item.setTitle("TestTitle" + i);
            item.setId(i);
            // Layout resource: 0 -> 1, 1 -> 11, 2 -> 21, 3 -> 1, 4 -> 11.
            // (Resource IDs cannot be 0)
            item.setLayoutResource((i % 3) * 10 + 1);
            mItems[i] = item;
            mItemGroup.addChild(item);
        }
    }

    @SmallTest
    public void testAdapter() {
        RecyclerItemAdapter adapter = new RecyclerItemAdapter(mItemGroup);
        assertEquals("Adapter should have 5 items", 5, adapter.getItemCount());
        assertEquals("Adapter should return the first item", mItems[0], adapter.getItem(0));
        assertEquals("ID should be same as position", 2, adapter.getItemId(2));

        // ViewType is same as layout resource for RecyclerItemAdapter
        assertEquals("Second item should have view type 21", 21, adapter.getItemViewType(2));
    }

    @SmallTest
    public void testGetRootItemHierarchy() {
        RecyclerItemAdapter adapter = new RecyclerItemAdapter(mItemGroup);
        ItemHierarchy root = adapter.getRootItemHierarchy();
        assertSame("Root item hierarchy should be mItemGroup", mItemGroup, root);
    }
}
