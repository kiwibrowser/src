// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import static org.hamcrest.Matchers.instanceOf;
import static org.hamcrest.Matchers.is;
import static org.junit.Assert.assertThat;

import android.graphics.drawable.Drawable;
import android.support.annotation.LayoutRes;
import android.support.test.filters.MediumTest;
import android.support.v7.widget.RecyclerView;
import android.widget.TextView;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.atomic.AtomicReference;

/**
 * View tests for the password accessory sheet.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PasswordAccessorySheetViewTest {
    private PasswordAccessorySheetModel mModel;
    private AtomicReference<RecyclerView> mView = new AtomicReference<>();

    @Rule
    public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeTabbedActivity.class);

    /**
     * This helper method inflates the accessory sheet and loads the given layout as minimalistic
     * Tab. The passed callback then allows access to the inflated layout.
     * @param layout The layout to be inflated.
     * @param listener Is called with the inflated layout when the Accessory Sheet initializes it.
     */
    private void openLayoutInAccessorySheet(
            @LayoutRes int layout, KeyboardAccessoryData.Tab.Listener listener) {
        mActivityTestRule.getActivity()
                .getManualFillingController()
                .getAccessorySheetForTesting()
                .addTab(new KeyboardAccessoryData.Tab() {
                    @Override
                    public Drawable getIcon() {
                        return null;
                    }

                    @Override
                    public String getContentDescription() {
                        return null;
                    }

                    @Override
                    public @LayoutRes int getTabLayout() {
                        return layout;
                    }

                    @Override
                    public Listener getListener() {
                        return listener;
                    }
                });
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mActivityTestRule.getActivity()
                    .getManualFillingController()
                    .getAccessorySheetForTesting()
                    .getMediatorForTesting()
                    .show();
        });
    }

    @Before
    public void setUp() throws InterruptedException {
        mModel = new PasswordAccessorySheetModel();
        mActivityTestRule.startMainActivityOnBlankPage();
        openLayoutInAccessorySheet(R.layout.password_accessory_sheet, view -> {
            mView.set((RecyclerView) view);
            final PasswordAccessorySheetViewBinder binder = new PasswordAccessorySheetViewBinder();
            // Reuse coordinator code to create and wire the adapter. No mediator involved.
            binder.initializeView(
                    mView.get(), PasswordAccessorySheetCoordinator.createAdapter(mModel, binder));
        });
        CriteriaHelper.pollUiThread(Criteria.equals(true, () -> mView.get() != null));
    }

    @After
    public void tearDown() {
        mView.set(null);
    }

    @Test
    @MediumTest
    public void testAddingCaptionsToTheModelRendersThem() {
        assertThat(mView.get().getChildCount(), is(0));

        ThreadUtils.runOnUiThreadBlocking(() -> mModel.addLabel("Passwords"));

        CriteriaHelper.pollUiThread(Criteria.equals(1, () -> mView.get().getChildCount()));
        assertThat(mView.get().getChildAt(0), instanceOf(TextView.class));
        assertThat(((TextView) mView.get().getChildAt(0)).getText(), is("Passwords"));
    }

    @Test
    @MediumTest
    public void testAddingSuggestionsToTheModelRendersClickableActions() throws ExecutionException {
        final AtomicReference<Boolean> clicked = new AtomicReference<>(false);
        final KeyboardAccessoryData.Action testSuggestion = new KeyboardAccessoryData.Action() {
            @Override
            public String getCaption() {
                return "Password Suggestion";
            }

            @Override
            public Delegate getDelegate() {
                return action -> clicked.set(true);
            }
        };
        assertThat(mView.get().getChildCount(), is(0));

        ThreadUtils.runOnUiThreadBlocking(() -> mModel.addSuggestion(testSuggestion));

        CriteriaHelper.pollUiThread(Criteria.equals(1, () -> mView.get().getChildCount()));
        assertThat(mView.get().getChildAt(0), instanceOf(TextView.class));

        TextView suggestion = (TextView) mView.get().getChildAt(0);
        assertThat(suggestion.getText(), is("Password Suggestion"));

        ThreadUtils.runOnUiThreadBlocking(suggestion::performClick);
        assertThat(clicked.get(), is(true));
    }
}