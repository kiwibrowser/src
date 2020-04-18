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

public class ItemGroupTest extends AndroidTestCase {

    public static final Item CHILD_1 = new Item();
    public static final Item CHILD_2 = new Item();
    public static final Item CHILD_3 = new Item();
    public static final Item CHILD_4 = new Item();

    @SmallTest
    public void testGroup() {
        ItemGroup itemGroup = new ItemGroup();
        itemGroup.addChild(CHILD_1);
        itemGroup.addChild(CHILD_2);

        assertSame("Item at position 0 should be child1", CHILD_1, itemGroup.getItemAt(0));
        assertSame("Item at position 1 should be child2", CHILD_2, itemGroup.getItemAt(1));
        assertEquals("Should have 2 children", 2, itemGroup.getCount());
    }

    @SmallTest
    public void testRemoveChild() {
        ItemGroup itemGroup = new ItemGroup();
        itemGroup.addChild(CHILD_1);
        itemGroup.addChild(CHILD_2);

        itemGroup.removeChild(CHILD_1);

        assertSame("Item at position 0 should be child2", CHILD_2, itemGroup.getItemAt(0));
        assertEquals("Should have 1 child", 1, itemGroup.getCount());
    }

    @SmallTest
    public void testClear() {
        ItemGroup itemGroup = new ItemGroup();
        itemGroup.addChild(CHILD_1);
        itemGroup.addChild(CHILD_2);

        itemGroup.clear();

        assertEquals("Should have 0 child", 0, itemGroup.getCount());
    }

    @SmallTest
    public void testNestedGroup() {
        ItemGroup parentGroup = new ItemGroup();
        ItemGroup childGroup = new ItemGroup();

        parentGroup.addChild(CHILD_1);
        childGroup.addChild(CHILD_2);
        childGroup.addChild(CHILD_3);
        parentGroup.addChild(childGroup);
        parentGroup.addChild(CHILD_4);

        CHILD_1.setTitle("CHILD1");
        CHILD_2.setTitle("CHILD2");
        CHILD_3.setTitle("CHILD3");
        CHILD_4.setTitle("CHILD4");

        assertSame("Position 0 should be child 1", CHILD_1, parentGroup.getItemAt(0));
        assertSame("Position 1 should be child 2", CHILD_2, parentGroup.getItemAt(1));
        assertSame("Position 2 should be child 3", CHILD_3, parentGroup.getItemAt(2));
        assertSame("Position 3 should be child 4", CHILD_4, parentGroup.getItemAt(3));
    }

    @SmallTest
    public void testEmptyChildGroup() {
        ItemGroup parentGroup = new ItemGroup();
        ItemGroup childGroup = new ItemGroup();

        parentGroup.addChild(CHILD_1);
        parentGroup.addChild(childGroup);
        parentGroup.addChild(CHILD_2);

        assertSame("Position 0 should be child 1", CHILD_1, parentGroup.getItemAt(0));
        assertSame("Position 1 should be child 2", CHILD_2, parentGroup.getItemAt(1));
    }

    @SmallTest
    public void testFindItemById() {
        ItemGroup itemGroup = new ItemGroup();
        CHILD_1.setId(12345);
        CHILD_2.setId(23456);

        itemGroup.addChild(CHILD_1);
        itemGroup.addChild(CHILD_2);

        assertSame("Find item 23456 should return child 2", CHILD_2, itemGroup.findItemById(23456));
    }

    @SmallTest
    public void testFindItemByIdNotFound() {
        ItemGroup itemGroup = new ItemGroup();
        CHILD_1.setId(12345);
        CHILD_2.setId(23456);

        itemGroup.addChild(CHILD_1);
        itemGroup.addChild(CHILD_2);

        assertNull("ID not found should return null", itemGroup.findItemById(56789));
    }
}
