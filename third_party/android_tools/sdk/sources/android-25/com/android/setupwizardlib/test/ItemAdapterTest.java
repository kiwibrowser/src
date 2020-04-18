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
import com.android.setupwizardlib.items.ItemAdapter;
import com.android.setupwizardlib.items.ItemGroup;
import com.android.setupwizardlib.items.ItemHierarchy;

import java.util.Arrays;
import java.util.HashSet;

public class ItemAdapterTest extends AndroidTestCase {

    private Item[] mItems = new Item[5];
    private ItemGroup mItemGroup = new ItemGroup();

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        for (int i = 0; i < 5; i++) {
            Item item = new Item();
            item.setTitle("TestTitle" + i);
            item.setId(i);
            item.setLayoutResource(((i % 3) + 1) * 10);
            mItems[i] = item;
            mItemGroup.addChild(item);
        }
    }

    @SmallTest
    public void testAdapter() {
        ItemAdapter adapter = new ItemAdapter(mItemGroup);
        assertEquals("Adapter should have 5 items", 5, adapter.getCount());
        assertEquals("Adapter should return the first item", mItems[0], adapter.getItem(0));
        assertEquals("ID should be same as position", 2, adapter.getItemId(2));

        // Each test item has its own layout resource, and therefore its own view type
        assertEquals("Should have 3 different view types", 3, adapter.getViewTypeCount());
        HashSet<Integer> viewTypes = new HashSet<>(3);
        viewTypes.add(adapter.getItemViewType(0));
        viewTypes.add(adapter.getItemViewType(1));
        viewTypes.add(adapter.getItemViewType(2));

        assertEquals("View types should be 0, 1, 2",
                new HashSet<>(Arrays.asList(0, 1, 2)), viewTypes);
    }

    @SmallTest
    public void testGetRootItemHierarchy() {
        ItemAdapter adapter = new ItemAdapter(mItemGroup);
        ItemHierarchy root = adapter.getRootItemHierarchy();
        assertSame("Root item hierarchy should be mItemGroup", mItemGroup, root);
    }
}
