// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.assertion.ViewAssertions.matches;
import static android.support.test.espresso.matcher.ViewMatchers.isDisplayed;
import static android.support.test.espresso.matcher.ViewMatchers.withText;

import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertNull;

import static org.junit.Assert.assertTrue;

import android.graphics.drawable.Drawable;
import android.support.annotation.LayoutRes;
import android.support.test.filters.MediumTest;
import android.support.v4.view.ViewPager;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.modelutil.LazyViewBinderAdapter;
import org.chromium.chrome.browser.modelutil.PropertyModelChangeProcessor;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * View tests for the keyboard accessory sheet component.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class AccessorySheetViewTest {
    private AccessorySheetModel mModel;
    private LazyViewBinderAdapter.StubHolder<ViewPager> mStubHolder;

    @Rule
    public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeTabbedActivity.class);

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
        mStubHolder = new LazyViewBinderAdapter.StubHolder<>(
                mActivityTestRule.getActivity().findViewById(R.id.keyboard_accessory_sheet_stub));
        mModel = new AccessorySheetModel();
        mModel.addObserver(new PropertyModelChangeProcessor<>(
                mModel, mStubHolder, new LazyViewBinderAdapter<>(new AccessorySheetViewBinder())));
    }

    @Test
    @MediumTest
    public void testAccessoryVisibilityChangedByModel() {
        // Initially, there shouldn't be a view yet.
        assertNull(mStubHolder.getView());

        // After setting the visibility to true, the view should exist and be visible.
        ThreadUtils.runOnUiThreadBlocking(() -> mModel.setVisible(true));
        assertNotNull(mStubHolder.getView());
        assertTrue(mStubHolder.getView().getVisibility() == View.VISIBLE);

        // After hiding the view, the view should still exist but be invisible.
        ThreadUtils.runOnUiThreadBlocking(() -> mModel.setVisible(false));
        assertNotNull(mStubHolder.getView());
        assertTrue(mStubHolder.getView().getVisibility() != View.VISIBLE);
    }

    @Test
    @MediumTest
    public void testAddingTabToModelRendersTabsView() {
        final String kSampleAction = "Some Action";
        mModel.getTabList().add(createTestTab(view -> {
            assertNotNull("The tab must have been created!", view);
            assertTrue("Empty tab is a layout.", view instanceof LinearLayout);
            LinearLayout baseLayout = (LinearLayout) view;
            TextView sampleTextView = new TextView(mActivityTestRule.getActivity());
            sampleTextView.setText(kSampleAction);
            baseLayout.addView(sampleTextView);
        }));
        mModel.setActiveTabIndex(0);
        // Shouldn't cause the view to be inflated.
        assertNull(mStubHolder.getView());

        // Setting visibility should cause the Tab to be rendered.
        ThreadUtils.runOnUiThreadBlocking(() -> mModel.setVisible(true));
        assertNotNull(mStubHolder.getView());

        onView(withText(kSampleAction)).check(matches(isDisplayed()));
    }

    private KeyboardAccessoryData.Tab createTestTab(KeyboardAccessoryData.Tab.Listener listener) {
        return new KeyboardAccessoryData.Tab() {
            @Override
            public Drawable getIcon() {
                return null; // Unused.
            }

            @Override
            public String getContentDescription() {
                return null; // Unused.
            }

            @Override
            public @LayoutRes int getTabLayout() {
                return R.layout.empty_accessory_sheet;
            }

            @Override
            public Listener getListener() {
                return listener;
            }
        };
    }
}