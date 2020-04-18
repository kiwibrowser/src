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

import com.android.setupwizardlib.SetupWizardRecyclerLayout;

public class SetupWizardRecyclerLayoutTest extends InstrumentationTestCase {

    private Context mContext;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = new ContextThemeWrapper(getInstrumentation().getContext(),
                R.style.SuwThemeMaterial_Light);
    }

    @SmallTest
    public void testDefaultTemplate() {
        SetupWizardRecyclerLayout layout = new TestLayout(mContext);
        assertRecyclerTemplateInflated(layout);
    }

    @SmallTest
    public void testInflateFromXml() {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        SetupWizardRecyclerLayout layout = (SetupWizardRecyclerLayout)
                inflater.inflate(R.layout.test_recycler_layout, null);
        assertRecyclerTemplateInflated(layout);
    }

    @SmallTest
    public void testGetRecyclerView() {
        SetupWizardRecyclerLayout layout = new TestLayout(mContext);
        assertRecyclerTemplateInflated(layout);
        assertNotNull("getRecyclerView should not be null", layout.getRecyclerView());
    }

    @SmallTest
    public void testAdapter() {
        SetupWizardRecyclerLayout layout = new TestLayout(mContext);
        assertRecyclerTemplateInflated(layout);

        final RecyclerView.Adapter adapter = new RecyclerView.Adapter() {
            @Override
            public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int position) {
                return new RecyclerView.ViewHolder(new View(parent.getContext())) {};
            }

            @Override
            public void onBindViewHolder(RecyclerView.ViewHolder viewHolder, int position) {
            }

            @Override
            public int getItemCount() {
                return 0;
            }
        };
        layout.setAdapter(adapter);

        final RecyclerView.Adapter gotAdapter = layout.getAdapter();
        // Note: The wrapped adapter should be returned, not the HeaderAdapter.
        assertSame("Adapter got from SetupWizardLayout should be same as set",
                adapter, gotAdapter);
    }

    @SmallTest
    public void testDividerInset() {
        SetupWizardRecyclerLayout layout = new TestLayout(mContext);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            layout.setLayoutDirection(View.LAYOUT_DIRECTION_LTR);
        }
        assertRecyclerTemplateInflated(layout);

        layout.setDividerInset(10);
        assertEquals("Divider inset should be 10", 10, layout.getDividerInset());

        final Drawable divider = layout.getDivider();
        assertTrue("Divider should be instance of InsetDrawable", divider instanceof InsetDrawable);
    }

    private void assertRecyclerTemplateInflated(SetupWizardRecyclerLayout layout) {
        View recyclerView = layout.findViewById(R.id.suw_recycler_view);
        assertTrue("@id/suw_recycler_view should be a RecyclerView",
                recyclerView instanceof RecyclerView);

        if (layout instanceof TestLayout) {
            assertNotNull("Header text view should not be null",
                    ((TestLayout) layout).findManagedViewById(R.id.suw_layout_title));
            assertNotNull("Decoration view should not be null",
                    ((TestLayout) layout).findManagedViewById(R.id.suw_layout_decor));
        }
    }

    // Make some methods public for testing
    public static class TestLayout extends SetupWizardRecyclerLayout {

        public TestLayout(Context context) {
            super(context);
        }

        @Override
        public View findManagedViewById(int id) {
            return super.findManagedViewById(id);
        }

    }
}
