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
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;

import com.android.setupwizardlib.R;
import com.android.setupwizardlib.items.ButtonItem;

public class ButtonItemTest extends AndroidTestCase {

    private ViewGroup mParent;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mParent = new LinearLayout(getContext());
    }

    public void testDefaultItem() {
        ButtonItem item = new ButtonItem();

        assertTrue("ButtonItem should be enabled by default", item.isEnabled());
        assertEquals("ButtonItem should return count = 0", 0, item.getCount());
        assertEquals("ButtonItem should return layout resource = 0", 0, item.getLayoutResource());
        assertEquals("Default theme should be @style/SuwButtonItem", R.style.SuwButtonItem,
                item.getTheme());
        assertNull("Default text should be null", item.getText());
    }

    public void testOnBindView() {
        ButtonItem item = new ButtonItem();

        try {
            item.onBindView(new View(getContext()));
            fail("Calling onBindView on ButtonItem should throw UnsupportedOperationException");
        } catch (UnsupportedOperationException e) {
            // pass
        }
    }

    public void testCreateButton() {
        TestButtonItem item = new TestButtonItem();
        final Button button = item.createButton(mParent);

        assertTrue("Default button should be enabled", button.isEnabled());
        assertTrue("Default button text should be empty", TextUtils.isEmpty(button.getText()));
    }

    public void testCreateButtonTwice() {
        TestButtonItem item = new TestButtonItem();
        final Button button = item.createButton(mParent);

        FrameLayout frameLayout = new FrameLayout(getContext());
        frameLayout.addView(button);

        final Button button2 = item.createButton(mParent);
        assertSame("createButton should be reused", button, button2);
        assertNull("Should be removed from parent after createButton", button2.getParent());
    }

    public void testSetEnabledTrue() {
        TestButtonItem item = new TestButtonItem();
        item.setEnabled(true);

        final Button button = item.createButton(mParent);
        assertTrue("ButtonItem should be enabled", item.isEnabled());
        assertTrue("Button should be enabled", button.isEnabled());
    }

    public void testSetEnabledFalse() {
        TestButtonItem item = new TestButtonItem();
        item.setEnabled(false);

        final Button button = item.createButton(mParent);
        assertFalse("ButtonItem should be disabled", item.isEnabled());
        assertFalse("Button should be disabled", button.isEnabled());
    }

    public void testSetText() {
        TestButtonItem item = new TestButtonItem();
        item.setText("lorem ipsum");

        final Button button = item.createButton(mParent);
        assertEquals("ButtonItem text should be \"lorem ipsum\"", "lorem ipsum", item.getText());
        assertEquals("Button text should be \"lorem ipsum\"", "lorem ipsum", button.getText());
    }

    public void testSetTheme() {
        TestButtonItem item = new TestButtonItem();
        item.setTheme(12345);

        final Button button = item.createButton(mParent);
        assertEquals("ButtonItem theme should be 12345", 12345, item.getTheme());
        button.getContext().getTheme();
    }

    public void testOnClickListener() {
        TestButtonItem item = new TestButtonItem();
        final TestOnClickListener listener = new TestOnClickListener();
        item.setOnClickListener(listener);

        assertNull("Clicked item should be null before clicking", listener.clickedItem);

        final Button button = item.createButton(mParent);
        button.performClick();

        assertSame("Clicked item should be set", item, listener.clickedItem);
    }

    private static class TestOnClickListener implements ButtonItem.OnClickListener {

        public ButtonItem clickedItem = null;

        @Override
        public void onClick(ButtonItem item) {
            clickedItem = item;
        }
    }

    private static class TestButtonItem extends ButtonItem {

        @Override
        public Button createButton(ViewGroup parent) {
            // Make this method public for testing
            return super.createButton(parent);
        }
    }
}
