/*
 * Copyright (C) 2016 The Android Open Source Project
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
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;

import com.android.setupwizardlib.items.ButtonBarItem;
import com.android.setupwizardlib.items.ButtonItem;
import com.android.setupwizardlib.items.Item;
import com.android.setupwizardlib.items.ItemHierarchy;

public class ButtonBarItemTest extends AndroidTestCase {

    private ButtonItem mChild1;
    private ButtonItem mChild2;
    private ButtonItem mChild3;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mChild1 = new ButtonItem();
        mChild2 = new ButtonItem();
        mChild3 = new ButtonItem();
    }

    public void testFindItemById() {
        ButtonBarItem item = new ButtonBarItem();
        item.setId(888);

        mChild1.setId(123);
        mChild2.setId(456);
        mChild3.setId(789);
        item.addChild(mChild1);
        item.addChild(mChild2);
        item.addChild(mChild3);

        assertEquals("Finding 123 should return child1", mChild1, item.findItemById(123));
        assertEquals("Finding 456 should return child2", mChild2, item.findItemById(456));
        assertEquals("Finding 789 should return child3", mChild3, item.findItemById(789));

        assertEquals("Finding 888 should return ButtonBarItem itself", item,
                item.findItemById(888));

        assertNull("Finding 999 should return null", item.findItemById(999));
    }

    public void testBindEmpty() {
        ButtonBarItem item = new ButtonBarItem();
        final ViewGroup layout = createLayout();
        item.onBindView(layout);

        assertEquals("Binding empty ButtonBar should not create any children", 0,
                layout.getChildCount());
    }

    public void testBind() {
        ButtonBarItem item = new ButtonBarItem();

        item.addChild(mChild1);
        mChild1.setText("child1");
        item.addChild(mChild2);
        mChild2.setText("child2");
        item.addChild(mChild3);
        mChild3.setText("child3");

        final ViewGroup layout = createLayout();
        item.onBindView(layout);

        assertEquals("Binding ButtonBar should create 3 children", 3, layout.getChildCount());
        assertEquals("First button should have text \"child1\"", "child1",
                ((Button) layout.getChildAt(0)).getText());
        assertEquals("Second button should have text \"child2\"", "child2",
                ((Button) layout.getChildAt(1)).getText());
        assertEquals("Third button should have text \"child3\"", "child3",
                ((Button) layout.getChildAt(2)).getText());
    }

    public void testAddInvalidChild() {
        ButtonBarItem item = new ButtonBarItem();

        ItemHierarchy invalidChild = new Item();

        try {
            item.addChild(invalidChild);
            fail("Adding non ButtonItem to ButtonBarItem should throw exception");
        } catch (UnsupportedOperationException e) {
            // pass
        }
    }

    private ViewGroup createLayout() {
        return new LinearLayout(mContext);
    }
}
