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

import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.setupwizardlib.items.Item;
import com.android.setupwizardlib.items.ItemGroup;
import com.android.setupwizardlib.items.ItemHierarchy;
import com.android.setupwizardlib.items.ItemInflater;

public class ItemInflaterTest extends InstrumentationTestCase {

    @SmallTest
    public void testDefaultPackage() {
        ItemInflater inflater = new ItemInflater(getInstrumentation().getContext());
        assertEquals("Default package should be the one containing Item class",
                "com.android.setupwizardlib.items.", inflater.getDefaultPackage());
    }

    @SmallTest
    public void testInflate() {
        ItemInflater inflater = new ItemInflater(getInstrumentation().getContext());
        ItemHierarchy item = inflater.inflate(R.xml.test_items);
        assertTrue("Inflated item should be ItemGroup", item instanceof ItemGroup);
        ItemGroup itemGroup = (ItemGroup) item;

        Item child0 = (Item) itemGroup.getItemAt(0);
        Item child1 = (Item) itemGroup.getItemAt(1);
        assertEquals("Title of first child should be Title1", "Title1", child0.getTitle());
        assertEquals("ID of second child should be test_item_2", R.id.test_item_2, child1.getId());
        assertEquals("Summary of second child should be Summary2", "Summary2",
                child1.getSummary());
    }
}
