// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.action.ViewActions.click;
import static android.support.test.espresso.assertion.ViewAssertions.doesNotExist;
import static android.support.test.espresso.assertion.ViewAssertions.matches;
import static android.support.test.espresso.matcher.ViewMatchers.isDisplayed;
import static android.support.test.espresso.matcher.ViewMatchers.isRoot;
import static android.support.test.espresso.matcher.ViewMatchers.withContentDescription;
import static android.support.test.espresso.matcher.ViewMatchers.withText;

import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertNull;

import static org.hamcrest.Matchers.instanceOf;
import static org.hamcrest.core.AllOf.allOf;
import static org.junit.Assert.assertTrue;

import static org.chromium.chrome.test.util.ViewUtils.VIEW_GONE;
import static org.chromium.chrome.test.util.ViewUtils.VIEW_INVISIBLE;
import static org.chromium.chrome.test.util.ViewUtils.VIEW_NULL;
import static org.chromium.chrome.test.util.ViewUtils.waitForView;

import android.graphics.drawable.Drawable;
import android.support.test.filters.MediumTest;
import android.support.v7.widget.AppCompatImageView;
import android.view.View;
import android.view.ViewGroup;

import org.hamcrest.Matcher;
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

import java.util.concurrent.atomic.AtomicReference;

/**
 * View tests for the keyboard accessory component.
 *
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class KeyboardAccessoryViewTest {
    private KeyboardAccessoryModel mModel;
    private LazyViewBinderAdapter.StubHolder<KeyboardAccessoryView> mViewHolder;

    @Rule
    public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeTabbedActivity.class);

    private static KeyboardAccessoryData.Action createTestAction(
            String caption, KeyboardAccessoryData.Action.Delegate delegate) {
        return new KeyboardAccessoryData.Action() {
            @Override
            public String getCaption() {
                return caption;
            }

            @Override
            public Delegate getDelegate() {
                return delegate;
            }
        };
    }

    private KeyboardAccessoryData.Tab createTestTab(String contentDescription) {
        return new KeyboardAccessoryData.Tab() {
            @Override
            public Drawable getIcon() {
                return mActivityTestRule.getActivity().getResources().getDrawable(
                        android.R.drawable.ic_lock_lock);
            }

            @Override
            public String getContentDescription() {
                return contentDescription;
            }

            @Override
            public int getTabLayout() {
                return R.layout.empty_accessory_sheet; // Unused.
            }

            @Override
            public Listener getListener() {
                return null; // Unused.
            }
        };
    }

    /**
     * Matches a tab with a given content description. Selecting the content description alone will
     * match all icons of the tabs as well.
     * @param description The description to look for.
     * @return Returns a matcher that can be used in |onView| or within other {@link Matcher}s.
     */
    private static Matcher<View> isTabWithDescription(String description) {
        return allOf(withContentDescription(description),
                instanceOf(AppCompatImageView.class)); // Match only the image.
    }

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
        mModel = new KeyboardAccessoryModel();
        mViewHolder = new LazyViewBinderAdapter.StubHolder<>(
                mActivityTestRule.getActivity().findViewById(R.id.keyboard_accessory_stub));

        mModel.addObserver(new PropertyModelChangeProcessor<>(mModel, mViewHolder,
                new LazyViewBinderAdapter<>(new KeyboardAccessoryViewBinder())));
    }

    @Test
    @MediumTest
    public void testAccessoryVisibilityChangedByModel() {
        // Initially, there shouldn't be a view yet.
        assertNull(mViewHolder.getView());

        // After setting the visibility to true, the view should exist and be visible.
        ThreadUtils.runOnUiThreadBlocking(() -> mModel.setVisible(true));
        assertNotNull(mViewHolder.getView());
        assertTrue(mViewHolder.getView().getVisibility() == View.VISIBLE);

        // After hiding the view, the view should still exist but be invisible.
        ThreadUtils.runOnUiThreadBlocking(() -> mModel.setVisible(false));
        assertNotNull(mViewHolder.getView());
        assertTrue(mViewHolder.getView().getVisibility() != View.VISIBLE);
    }

    @Test
    @MediumTest
    public void testClickableActionAddedWhenChangingModel() {
        final AtomicReference<Boolean> buttonClicked = new AtomicReference<>();
        final KeyboardAccessoryData.Action testAction =
                createTestAction("Test Button", action -> buttonClicked.set(true));

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mModel.setVisible(true);
            mModel.getActionList().add(testAction);
        });

        onView(isRoot()).check((root, e) -> waitForView((ViewGroup) root, withText("Test Button")));
        onView(withText("Test Button")).perform(click());

        assertTrue(buttonClicked.get());
    }

    @Test
    @MediumTest
    public void testCanAddSingleButtons() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mModel.setVisible(true);
            mModel.getActionList().set(
                    new KeyboardAccessoryData.Action[] {createTestAction("First", action -> {}),
                            createTestAction("Second", action -> {})});
        });

        onView(isRoot()).check((root, e) -> waitForView((ViewGroup) root, withText("First")));
        onView(withText("First")).check(matches(isDisplayed()));
        onView(withText("Second")).check(matches(isDisplayed()));

        ThreadUtils.runOnUiThreadBlocking(
                () -> mModel.getActionList().add(createTestAction("Third", action -> {})));

        onView(isRoot()).check((root, e) -> waitForView((ViewGroup) root, withText("Third")));
        onView(withText("First")).check(matches(isDisplayed()));
        onView(withText("Second")).check(matches(isDisplayed()));
        onView(withText("Third")).check(matches(isDisplayed()));
    }

    @Test
    @MediumTest
    public void testCanRemoveSingleButtons() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mModel.setVisible(true);
            mModel.getActionList().set(
                    new KeyboardAccessoryData.Action[] {createTestAction("First", action -> {}),
                            createTestAction("Second", action -> {}),
                            createTestAction("Third", action -> {})});
        });

        onView(isRoot()).check((root, e) -> waitForView((ViewGroup) root, withText("First")));
        onView(withText("First")).check(matches(isDisplayed()));
        onView(withText("Second")).check(matches(isDisplayed()));
        onView(withText("Third")).check(matches(isDisplayed()));

        ThreadUtils.runOnUiThreadBlocking(
                () -> mModel.getActionList().remove(mModel.getActionList().get(1)));

        onView(isRoot()).check((root, e)
                                       -> waitForView((ViewGroup) root, withText("Second"),
                                               VIEW_INVISIBLE | VIEW_GONE | VIEW_NULL));
        onView(withText("First")).check(matches(isDisplayed()));
        onView(withText("Second")).check(doesNotExist());
        onView(withText("Third")).check(matches(isDisplayed()));
    }

    @Test
    @MediumTest
    public void testRemovesTabs() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mModel.setVisible(true);
            mModel.getTabList().set(new KeyboardAccessoryData.Tab[] {createTestTab("FirstTab"),
                    createTestTab("SecondTab"), createTestTab("ThirdTab")});
        });

        onView(isRoot()).check(
                (root, e) -> waitForView((ViewGroup) root, isTabWithDescription("FirstTab")));
        onView(isTabWithDescription("FirstTab")).check(matches(isDisplayed()));
        onView(isTabWithDescription("SecondTab")).check(matches(isDisplayed()));
        onView(isTabWithDescription("ThirdTab")).check(matches(isDisplayed()));

        ThreadUtils.runOnUiThreadBlocking(
                () -> mModel.getTabList().remove(mModel.getTabList().get(1)));

        onView(isRoot()).check(
                (root, e)
                        -> waitForView((ViewGroup) root, isTabWithDescription("SecondTab"),
                                VIEW_INVISIBLE | VIEW_GONE | VIEW_NULL));
        onView(isTabWithDescription("FirstTab")).check(matches(isDisplayed()));
        onView(isTabWithDescription("SecondTab")).check(doesNotExist());
        onView(isTabWithDescription("ThirdTab")).check(matches(isDisplayed()));
    }

    @Test
    @MediumTest
    public void testAddsTabs() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mModel.setVisible(true);
            mModel.getTabList().set(new KeyboardAccessoryData.Tab[] {
                    createTestTab("FirstTab"), createTestTab("SecondTab")});
        });

        onView(isRoot()).check(
                (root, e) -> waitForView((ViewGroup) root, isTabWithDescription("FirstTab")));
        onView(isTabWithDescription("FirstTab")).check(matches(isDisplayed()));
        onView(isTabWithDescription("SecondTab")).check(matches(isDisplayed()));
        onView(isTabWithDescription("ThirdTab")).check(doesNotExist());

        ThreadUtils.runOnUiThreadBlocking(() -> mModel.getTabList().add(createTestTab("ThirdTab")));

        onView(isRoot()).check(
                (root, e) -> waitForView((ViewGroup) root, isTabWithDescription("ThirdTab")));
        onView(isTabWithDescription("FirstTab")).check(matches(isDisplayed()));
        onView(isTabWithDescription("SecondTab")).check(matches(isDisplayed()));
        onView(isTabWithDescription("ThirdTab")).check(matches(isDisplayed()));
    }
}