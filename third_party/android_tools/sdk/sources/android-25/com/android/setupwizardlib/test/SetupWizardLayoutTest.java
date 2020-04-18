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
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Parcelable;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.SparseArray;
import android.view.AbsSavedState;
import android.view.ContextThemeWrapper;
import android.view.InflateException;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.android.setupwizardlib.SetupWizardLayout;
import com.android.setupwizardlib.view.NavigationBar;

public class SetupWizardLayoutTest extends InstrumentationTestCase {

    private Context mContext;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = new ContextThemeWrapper(getInstrumentation().getContext(),
                R.style.SuwThemeMaterial_Light);
    }

    @SmallTest
    public void testDefaultTemplate() {
        SetupWizardLayout layout = new SetupWizardLayout(mContext);
        assertDefaultTemplateInflated(layout);
    }

    @SmallTest
    public void testSetHeaderText() {
        SetupWizardLayout layout = new SetupWizardLayout(mContext);
        TextView title = (TextView) layout.findViewById(R.id.suw_layout_title);
        layout.setHeaderText("Abracadabra");
        assertEquals("Header text should be \"Abracadabra\"", "Abracadabra", title.getText());
    }

    @SmallTest
    public void testAddView() {
        SetupWizardLayout layout = new SetupWizardLayout(mContext);
        TextView tv = new TextView(mContext);
        tv.setId(R.id.test_view_id);
        layout.addView(tv);
        assertDefaultTemplateInflated(layout);
        View view = layout.findViewById(R.id.test_view_id);
        assertSame("The view added should be the same text view", tv, view);
    }

    @SmallTest
    public void testInflateFromXml() {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        SetupWizardLayout layout = (SetupWizardLayout) inflater.inflate(R.layout.test_layout, null);
        assertDefaultTemplateInflated(layout);
        View content = layout.findViewById(R.id.test_content);
        assertTrue("@id/test_content should be a TextView", content instanceof TextView);
    }

    @SmallTest
    public void testCustomTemplate() {
        SetupWizardLayout layout = new SetupWizardLayout(mContext, R.layout.test_template);
        View templateView = layout.findViewById(R.id.test_template_view);
        assertNotNull("@id/test_template_view should exist in template", templateView);

        TextView tv = new TextView(mContext);
        tv.setId(R.id.test_view_id);
        layout.addView(tv);

        templateView = layout.findViewById(R.id.test_template_view);
        assertNotNull("@id/test_template_view should exist in template", templateView);
        View contentView = layout.findViewById(R.id.test_view_id);
        assertSame("The view added should be the same text view", tv, contentView);

        // The following methods should be no-ops because the custom template doesn't contain the
        // corresponding optional views. Just check that they don't throw exceptions.
        layout.setHeaderText("Abracadabra");
        layout.setIllustration(new ColorDrawable(Color.MAGENTA));
        layout.setLayoutBackground(new ColorDrawable(Color.RED));
    }

    @SmallTest
    public void testGetNavigationBar() {
        final SetupWizardLayout layout = new SetupWizardLayout(mContext);
        final NavigationBar navigationBar = layout.getNavigationBar();
        assertEquals("Navigation bar should have ID = @id/suw_layout_navigation_bar",
                R.id.suw_layout_navigation_bar, navigationBar.getId());
    }

    @SmallTest
    public void testGetNavigationBarNull() {
        // test_template does not have navigation bar so getNavigationBar() should return null.
        final SetupWizardLayout layout = new SetupWizardLayout(mContext, R.layout.test_template);
        final NavigationBar navigationBar = layout.getNavigationBar();
        assertNull("getNavigationBar() in test_template should return null", navigationBar);
    }

    @SmallTest
    public void testShowProgressBar() {
        final SetupWizardLayout layout = new SetupWizardLayout(mContext);
        layout.showProgressBar();
        assertTrue("Progress bar should be shown", layout.isProgressBarShown());
        final View progressBar = layout.findViewById(R.id.suw_layout_progress);
        assertTrue("Progress bar view should be shown",
                progressBar instanceof ProgressBar && progressBar.getVisibility() == View.VISIBLE);
    }

    @SmallTest
    public void testHideProgressBar() {
        final SetupWizardLayout layout = new SetupWizardLayout(mContext);
        layout.showProgressBar();
        assertTrue("Progress bar should be shown", layout.isProgressBarShown());
        layout.hideProgressBar();
        assertFalse("Progress bar should be hidden", layout.isProgressBarShown());
        final View progressBar = layout.findViewById(R.id.suw_layout_progress);
        assertTrue("Progress bar view should exist",
                progressBar == null || progressBar.getVisibility() != View.VISIBLE);
    }

    @SmallTest
    public void testShowProgressBarNotExist() {
        // test_template does not have progress bar, so showNavigationBar() should do nothing.
        final SetupWizardLayout layout = new SetupWizardLayout(mContext, R.layout.test_template);
        layout.showProgressBar();
        assertFalse("Progress bar should not be shown", layout.isProgressBarShown());
    }

    @SmallTest
    public void testWrongTheme() {
        // Test the error message when using the wrong theme
        mContext = new ContextThemeWrapper(getInstrumentation().getContext(),
                android.R.style.Theme);
        try {
            new SetupWizardLayout(mContext);
            fail("Should have thrown InflateException");
        } catch (InflateException e) {
            assertEquals("Exception message should mention correct theme to use",
                    "Unable to inflate layout. Are you using @style/SuwThemeMaterial "
                            + "(or its descendant) as your theme?", e.getMessage());
        }
    }

    @SmallTest
    public void testOnRestoreFromInstanceState() {
        final SetupWizardLayout layout = new SetupWizardLayout(mContext);
        // noinspection ResourceType
        layout.setId(1234);

        SparseArray<Parcelable> container = new SparseArray<>();
        layout.saveHierarchyState(container);

        final SetupWizardLayout layout2 = new SetupWizardLayout(mContext);
        // noinspection ResourceType
        layout2.setId(1234);
        layout2.restoreHierarchyState(container);

        assertFalse("Progress bar should not be shown", layout2.isProgressBarShown());
    }

    @SmallTest
    public void testOnRestoreFromInstanceStateProgressBarShown() {
        final SetupWizardLayout layout = new SetupWizardLayout(mContext);
        // noinspection ResourceType
        layout.setId(1234);

        layout.setProgressBarShown(true);

        SparseArray<Parcelable> container = new SparseArray<>();
        layout.saveHierarchyState(container);

        final SetupWizardLayout layout2 = new SetupWizardLayout(mContext);
        // noinspection ResourceType
        layout2.setId(1234);
        layout2.restoreHierarchyState(container);

        assertTrue("Progress bar should be shown", layout2.isProgressBarShown());
    }

    @SmallTest
    public void testOnRestoreFromIncompatibleInstanceState() {
        final SetupWizardLayout layout = new SetupWizardLayout(mContext);
        // noinspection ResourceType
        layout.setId(1234);

        SparseArray<Parcelable> container = new SparseArray<>();
        container.put(1234, AbsSavedState.EMPTY_STATE);
        layout.restoreHierarchyState(container);

        // SetupWizardLayout shouldn't crash with incompatible Parcelable

        assertFalse("Progress bar should not be shown", layout.isProgressBarShown());
    }

    private void assertDefaultTemplateInflated(SetupWizardLayout layout) {
        View decorView = layout.findViewById(R.id.suw_layout_decor);
        View navbar = layout.findViewById(R.id.suw_layout_navigation_bar);
        View title = layout.findViewById(R.id.suw_layout_title);
        assertNotNull("@id/suw_layout_decor_view should not be null", decorView);
        assertTrue("@id/suw_layout_navigation_bar should be an instance of NavigationBar",
                navbar instanceof NavigationBar);
        assertNotNull("@id/suw_layout_title should not be null", title);
    }
}
