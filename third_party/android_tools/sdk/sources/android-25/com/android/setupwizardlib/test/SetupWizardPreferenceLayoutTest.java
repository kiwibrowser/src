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

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.InsetDrawable;
import android.os.Build;
import android.support.v7.widget.RecyclerView;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.setupwizardlib.SetupWizardPreferenceLayout;

public class SetupWizardPreferenceLayoutTest extends InstrumentationTestCase {

    private Context mContext;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = new ContextThemeWrapper(getInstrumentation().getContext(),
                R.style.SuwThemeMaterial_Light);
    }

    @SmallTest
    public void testDefaultTemplate() {
        SetupWizardPreferenceLayout layout = new TestLayout(mContext);
        assertPreferenceTemplateInflated(layout);
    }

    @SmallTest
    public void testGetRecyclerView() {
        SetupWizardPreferenceLayout layout = new TestLayout(mContext);
        assertPreferenceTemplateInflated(layout);
        assertNotNull("getRecyclerView should not be null", layout.getRecyclerView());
    }

    @SmallTest
    public void testOnCreateRecyclerView() {
        SetupWizardPreferenceLayout layout = new TestLayout(mContext);
        assertPreferenceTemplateInflated(layout);
        final RecyclerView recyclerView = layout.onCreateRecyclerView(LayoutInflater.from(mContext),
                layout, null /* savedInstanceState */);
        assertNotNull("RecyclerView created should not be null", recyclerView);
    }

    @SmallTest
    public void testDividerInset() {
        SetupWizardPreferenceLayout layout = new TestLayout(mContext);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            layout.setLayoutDirection(View.LAYOUT_DIRECTION_LTR);
        }
        assertPreferenceTemplateInflated(layout);

        layout.addView(layout.onCreateRecyclerView(LayoutInflater.from(mContext), layout,
                null /* savedInstanceState */));

        layout.setDividerInset(10);
        assertEquals("Divider inset should be 10", 10, layout.getDividerInset());

        final Drawable divider = layout.getDivider();
        assertTrue("Divider should be instance of InsetDrawable", divider instanceof InsetDrawable);
    }

    private void assertPreferenceTemplateInflated(SetupWizardPreferenceLayout layout) {
        View contentContainer = layout.findViewById(R.id.suw_layout_content);
        assertTrue("@id/suw_layout_content should be a ViewGroup",
                contentContainer instanceof ViewGroup);

        if (layout instanceof TestLayout) {
            assertNotNull("Header text view should not be null",
                    ((TestLayout) layout).findManagedViewById(R.id.suw_layout_title));
            assertNotNull("Decoration view should not be null",
                    ((TestLayout) layout).findManagedViewById(R.id.suw_layout_decor));
        }
    }

    // Make some methods public for testing
    public static class TestLayout extends SetupWizardPreferenceLayout {

        public TestLayout(Context context) {
            super(context);
        }

        @Override
        public View findManagedViewById(int id) {
            return super.findManagedViewById(id);
        }
    }
}
