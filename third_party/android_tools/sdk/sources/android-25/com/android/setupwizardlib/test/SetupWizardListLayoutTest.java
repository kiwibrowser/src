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
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.android.setupwizardlib.SetupWizardLayout;
import com.android.setupwizardlib.SetupWizardListLayout;
import com.android.setupwizardlib.view.NavigationBar;

public class SetupWizardListLayoutTest extends InstrumentationTestCase {

    private Context mContext;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = new ContextThemeWrapper(getInstrumentation().getContext(),
                R.style.SuwThemeMaterial_Light);
    }

    @SmallTest
    public void testDefaultTemplate() {
        SetupWizardListLayout layout = new SetupWizardListLayout(mContext);
        assertListTemplateInflated(layout);
    }

    @SmallTest
    public void testAddView() {
        SetupWizardListLayout layout = new SetupWizardListLayout(mContext);
        TextView tv = new TextView(mContext);
        try {
            layout.addView(tv);
            fail("Adding view to ListLayout should throw");
        } catch (UnsupportedOperationException e) {
            // Expected exception
        }
    }

    @SmallTest
    public void testInflateFromXml() {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        SetupWizardListLayout layout = (SetupWizardListLayout)
                inflater.inflate(R.layout.test_list_layout, null);
        assertListTemplateInflated(layout);
    }

    @SmallTest
    public void testShowProgressBar() {
        final SetupWizardListLayout layout = new SetupWizardListLayout(mContext);
        layout.showProgressBar();
        assertTrue("Progress bar should be shown", layout.isProgressBarShown());
        final View progressBar = layout.findViewById(R.id.suw_layout_progress);
        assertTrue("Progress bar view should be shown",
                progressBar instanceof ProgressBar && progressBar.getVisibility() == View.VISIBLE);
    }

    private void assertListTemplateInflated(SetupWizardLayout layout) {
        View decorView = layout.findViewById(R.id.suw_layout_decor);
        View navbar = layout.findViewById(R.id.suw_layout_navigation_bar);
        View title = layout.findViewById(R.id.suw_layout_title);
        View list = layout.findViewById(android.R.id.list);
        assertNotNull("@id/suw_layout_decor_view should not be null", decorView);
        assertTrue("@id/suw_layout_navigation_bar should be an instance of NavigationBar",
                navbar instanceof NavigationBar);
        assertNotNull("@id/suw_layout_title should not be null", title);
        assertTrue("@android:id/list should be an instance of ListView", list instanceof ListView);
    }
}
