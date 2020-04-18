// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.app.Activity;
import android.content.DialogInterface;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.widget.PromoDialog.DialogParams;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

import java.util.concurrent.Callable;

/**
 * Tests for the PromoDialog and PromoDialogLayout.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PromoDialogTest {
    // TODO(tedchoc): Find a way to introduce a lightweight activity that doesn't spin up the world.
    //                crbug.com/728297
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    /**
     * Creates a PromoDialog.  Doesn't call {@link PromoDialog#show} because there is no Window to
     * attach them to, but it does create them and inflate the layouts.
     */
    private static class PromoDialogWrapper {
        public final CallbackHelper primaryCallback = new CallbackHelper();
        public final CallbackHelper secondaryCallback = new CallbackHelper();
        public final PromoDialog dialog;
        public final PromoDialogLayout dialogLayout;

        private final DialogParams mDialogParams;

        PromoDialogWrapper(final Activity activity, final DialogParams dialogParams)
                throws Exception {
            mDialogParams = dialogParams;
            dialog = ThreadUtils.runOnUiThreadBlocking(new Callable<PromoDialog>() {
                @Override
                public PromoDialog call() throws Exception {
                    PromoDialog dialog = new PromoDialog(activity) {
                        @Override
                        public DialogParams getDialogParams() {
                            return mDialogParams;
                        }

                        @Override
                        public void onDismiss(DialogInterface dialog) {}

                        @Override
                        public void onClick(View view) {
                            if (view.getId() == R.id.button_primary) {
                                primaryCallback.notifyCalled();
                            } else if (view.getId() == R.id.button_secondary) {
                                secondaryCallback.notifyCalled();
                            }
                        }
                    };
                    dialog.onCreate(null);
                    return dialog;
                }
            });
            dialogLayout = ThreadUtils.runOnUiThreadBlocking(new Callable<PromoDialogLayout>() {
                @Override
                public PromoDialogLayout call() throws Exception {
                    PromoDialogLayout promoDialogLayout =
                            (PromoDialogLayout) dialog.getWindow().getDecorView().findViewById(
                                    R.id.promo_dialog_layout);
                    return promoDialogLayout;
                }
            });
            // Measure the PromoDialogLayout so that the controls have some size.
            triggerDialogLayoutMeasure(500, 1000);
        }

        /**
         * Trigger a {@link View#measure(int, int)} on the promo dialog layout.
         */
        public void triggerDialogLayoutMeasure(final int width, final int height) {
            ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    int widthMeasureSpec = MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY);
                    int heightMeasureSpec =
                            MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY);
                    dialogLayout.measure(widthMeasureSpec, heightMeasureSpec);
                }
            });
        }
    }

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @Test
    @SmallTest
    public void testBasic_Visibility() throws Exception {
        // Create a full dialog.
        DialogParams dialogParams = new DialogParams();
        dialogParams.drawableResource = R.drawable.data_reduction_illustration;
        dialogParams.headerStringResource = R.string.data_reduction_promo_title;
        dialogParams.subheaderStringResource = R.string.data_reduction_promo_summary;
        dialogParams.primaryButtonStringResource = R.string.ok;
        dialogParams.secondaryButtonStringResource = R.string.cancel;
        dialogParams.footerStringResource = R.string.learn_more;
        checkDialogControlVisibility(dialogParams);

        // Create a minimal dialog.
        dialogParams = new DialogParams();
        dialogParams.headerStringResource = R.string.data_reduction_promo_title;
        dialogParams.primaryButtonStringResource = R.string.ok;
        checkDialogControlVisibility(dialogParams);
    }

    /** Confirm that PromoDialogs are constructed with all the elements expected. */
    private void checkDialogControlVisibility(final DialogParams dialogParams) throws Exception {
        PromoDialogWrapper wrapper =
                new PromoDialogWrapper(mActivityTestRule.getActivity(), dialogParams);
        PromoDialogLayout promoDialogLayout = wrapper.dialogLayout;

        View illustration = promoDialogLayout.findViewById(R.id.illustration);
        View header = promoDialogLayout.findViewById(R.id.header);
        View subheader = promoDialogLayout.findViewById(R.id.subheader);
        View primary = promoDialogLayout.findViewById(R.id.button_primary);
        View secondary = promoDialogLayout.findViewById(R.id.button_secondary);
        View footer = promoDialogLayout.findViewById(R.id.footer);

        // Any controls not specified by the DialogParams won't exist.
        checkControlVisibility(illustration, dialogParams.drawableResource != 0);
        checkControlVisibility(header, dialogParams.headerStringResource != 0);
        checkControlVisibility(subheader, dialogParams.subheaderStringResource != 0);
        checkControlVisibility(primary, dialogParams.primaryButtonStringResource != 0);
        checkControlVisibility(secondary, dialogParams.secondaryButtonStringResource != 0);
        checkControlVisibility(footer, dialogParams.footerStringResource != 0);
    }

    /** Check if a control should be visible. */
    private void checkControlVisibility(View view, boolean shouldBeVisible) {
        Assert.assertEquals(shouldBeVisible, view != null);
        if (view != null) {
            Assert.assertTrue(view.getMeasuredWidth() > 0);
            Assert.assertTrue(view.getMeasuredHeight() > 0);
        }
    }

    @Test
    @SmallTest
    public void testBasic_Orientation() throws Exception {
        DialogParams dialogParams = new DialogParams();
        dialogParams.drawableResource = R.drawable.data_reduction_illustration;
        dialogParams.headerStringResource = R.string.data_reduction_promo_title;
        dialogParams.subheaderStringResource = R.string.data_reduction_promo_summary;
        dialogParams.primaryButtonStringResource = R.string.ok;
        dialogParams.secondaryButtonStringResource = R.string.cancel;
        dialogParams.footerStringResource = R.string.learn_more;

        PromoDialogWrapper wrapper =
                new PromoDialogWrapper(mActivityTestRule.getActivity(), dialogParams);
        final PromoDialogLayout promoDialogLayout = wrapper.dialogLayout;
        LinearLayout flippableLayout =
                (LinearLayout) promoDialogLayout.findViewById(R.id.full_promo_content);

        // Tall screen should keep the illustration above everything else.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                int widthMeasureSpec = MeasureSpec.makeMeasureSpec(500, MeasureSpec.EXACTLY);
                int heightMeasureSpec = MeasureSpec.makeMeasureSpec(1000, MeasureSpec.EXACTLY);
                promoDialogLayout.measure(widthMeasureSpec, heightMeasureSpec);
            }
        });
        Assert.assertEquals(LinearLayout.VERTICAL, flippableLayout.getOrientation());

        // Wide screen should move the image left.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                int widthMeasureSpec = MeasureSpec.makeMeasureSpec(1000, MeasureSpec.EXACTLY);
                int heightMeasureSpec = MeasureSpec.makeMeasureSpec(500, MeasureSpec.EXACTLY);
                promoDialogLayout.measure(widthMeasureSpec, heightMeasureSpec);
            }
        });
        Assert.assertEquals(LinearLayout.HORIZONTAL, flippableLayout.getOrientation());
    }

    @Test
    @SmallTest
    public void testBasic_ButtonClicks() throws Exception {
        DialogParams dialogParams = new DialogParams();
        dialogParams.headerStringResource = R.string.search_with_sogou;
        dialogParams.primaryButtonStringResource = R.string.ok;
        dialogParams.secondaryButtonStringResource = R.string.cancel;

        PromoDialogWrapper wrapper =
                new PromoDialogWrapper(mActivityTestRule.getActivity(), dialogParams);
        final PromoDialogLayout promoDialogLayout = wrapper.dialogLayout;

        // Nothing should have been clicked yet.
        Assert.assertEquals(0, wrapper.primaryCallback.getCallCount());
        Assert.assertEquals(0, wrapper.secondaryCallback.getCallCount());

        // Only the primary button should register a click.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                promoDialogLayout.findViewById(R.id.button_primary).performClick();
            }
        });
        Assert.assertEquals(1, wrapper.primaryCallback.getCallCount());
        Assert.assertEquals(0, wrapper.secondaryCallback.getCallCount());

        // Only the secondary button should register a click.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                promoDialogLayout.findViewById(R.id.button_secondary).performClick();
            }
        });
        Assert.assertEquals(1, wrapper.primaryCallback.getCallCount());
        Assert.assertEquals(1, wrapper.secondaryCallback.getCallCount());
    }

    @Test
    @SmallTest
    public void testBasic_HeaderBehavior_WithIllustration() throws Exception {
        // With an illustration, the header View is part of the scrollable content.
        DialogParams dialogParams = new DialogParams();
        dialogParams.drawableResource = R.drawable.data_reduction_illustration;
        dialogParams.headerStringResource = R.string.data_reduction_promo_title;
        dialogParams.primaryButtonStringResource = R.string.data_reduction_enable_button;

        PromoDialogWrapper wrapper =
                new PromoDialogWrapper(mActivityTestRule.getActivity(), dialogParams);
        PromoDialogLayout promoDialogLayout = wrapper.dialogLayout;
        ViewGroup scrollableLayout =
                (ViewGroup) promoDialogLayout.findViewById(R.id.scrollable_promo_content);

        View header = promoDialogLayout.findViewById(R.id.header);
        Assert.assertEquals(scrollableLayout.getChildAt(0), header);
        assertHasStartAndEndPadding(header, false);
    }

    @Test
    @SmallTest
    public void testBasic_HeaderBehavior_WithVectorIllustration() throws Exception {
        // With a vector illustration, the header View is part of the scrollable content.
        DialogParams dialogParams = new DialogParams();
        dialogParams.vectorDrawableResource = R.drawable.search_sogou;
        dialogParams.headerStringResource = R.string.search_with_sogou;
        dialogParams.primaryButtonStringResource = R.string.ok;

        PromoDialogWrapper wrapper =
                new PromoDialogWrapper(mActivityTestRule.getActivity(), dialogParams);
        PromoDialogLayout promoDialogLayout = wrapper.dialogLayout;
        ViewGroup scrollableLayout =
                (ViewGroup) promoDialogLayout.findViewById(R.id.scrollable_promo_content);

        View header = promoDialogLayout.findViewById(R.id.header);
        Assert.assertEquals(scrollableLayout.getChildAt(0), header);
        assertHasStartAndEndPadding(header, false);
    }

    @Test
    @SmallTest
    public void testBasic_HeaderBehavior_NoIllustration() throws Exception {
        // Without an illustration, the header View becomes locked to the top of the layout if
        // there is enough height.
        DialogParams dialogParams = new DialogParams();
        dialogParams.headerStringResource = R.string.search_with_sogou;
        dialogParams.primaryButtonStringResource = R.string.ok;

        PromoDialogWrapper wrapper =
                new PromoDialogWrapper(mActivityTestRule.getActivity(), dialogParams);
        PromoDialogLayout promoDialogLayout = wrapper.dialogLayout;

        // Add a dummy control view to ensure the scrolling container has some content.
        View view = new View(InstrumentationRegistry.getTargetContext());
        view.setMinimumHeight(2000);
        promoDialogLayout.addControl(view);

        View header = promoDialogLayout.findViewById(R.id.header);
        ViewGroup scrollableLayout =
                (ViewGroup) promoDialogLayout.findViewById(R.id.scrollable_promo_content);

        wrapper.triggerDialogLayoutMeasure(400, 1000);
        Assert.assertEquals(promoDialogLayout.getChildAt(0), header);
        assertHasStartAndEndPadding(header, true);

        // Decrease the size and see the header is moved into the scrollable content.
        wrapper.triggerDialogLayoutMeasure(400, 100);
        Assert.assertEquals(scrollableLayout.getChildAt(0), header);
        assertHasStartAndEndPadding(header, false);

        // Increase again and ensure the header is moved back to the top of the layout.
        wrapper.triggerDialogLayoutMeasure(400, 1000);
        Assert.assertEquals(promoDialogLayout.getChildAt(0), header);
        assertHasStartAndEndPadding(header, true);
    }

    private static void assertHasStartAndEndPadding(View view, boolean shouldHavePadding) {
        if (shouldHavePadding) {
            Assert.assertNotEquals(0, ApiCompatibilityUtils.getPaddingStart(view));
            Assert.assertNotEquals(0, ApiCompatibilityUtils.getPaddingEnd(view));
        } else {
            Assert.assertEquals(0, ApiCompatibilityUtils.getPaddingStart(view));
            Assert.assertEquals(0, ApiCompatibilityUtils.getPaddingEnd(view));
        }
    }
}
