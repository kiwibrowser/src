// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.support.test.filters.MediumTest;
import android.view.ViewGroup;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.WebContents;

import java.util.concurrent.Callable;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;

/**
 * End-to-end tests for the CompositorViewHolder's behavior while in VR.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class VrShellCompositorViewHolderTest {
    // We explicitly instantiate a rule here instead of using parameterization since this class
    // only ever runs in ChromeTabbedActivity.
    @Rule
    public ChromeTabbedActivityVrTestRule mVrTestRule = new ChromeTabbedActivityVrTestRule();

    /**
     * Verify that resizing the CompositorViewHolder does not cause the current tab to resize while
     * the CompositorViewHolder is detached from the TabModelSelector. See crbug.com/680240.
     * @throws InterruptedException
     * @throws TimeoutException
     */
    @Test
    @MediumTest
    @RetryOnFailure
    public void testResizeWithCompositorViewHolderDetached()
            throws InterruptedException, TimeoutException {
        final AtomicReference<Integer> oldWidth = new AtomicReference<>();
        final AtomicReference<Integer> oldHeight = new AtomicReference<>();
        final int testWidth = 123;
        final int testHeight = 456;
        final WebContents webContents = mVrTestRule.getWebContents();

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                CompositorViewHolder compositorViewHolder =
                        (CompositorViewHolder) mVrTestRule.getActivity().findViewById(
                                R.id.compositor_view_holder);
                compositorViewHolder.onEnterVr();

                oldWidth.set(webContents.getWidth());
                oldHeight.set(webContents.getHeight());

                ViewGroup.LayoutParams layoutParams = compositorViewHolder.getLayoutParams();
                layoutParams.width = testWidth;
                layoutParams.height = testHeight;
                compositorViewHolder.requestLayout();
            }
        });
        CriteriaHelper.pollUiThread(Criteria.equals(testWidth, new Callable<Integer>() {
            @Override
            public Integer call() {
                return mVrTestRule.getActivity()
                        .findViewById(R.id.compositor_view_holder)
                        .getMeasuredWidth();
            }
        }));

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Assert.assertEquals(
                        "Viewport width should not have changed when resizing a detached "
                                + "CompositorViewHolder",
                        webContents.getWidth(), oldWidth.get().intValue());
                Assert.assertEquals(
                        "Viewport width should not have changed when resizing a detached "
                                + "CompositorViewHolder",
                        webContents.getHeight(), oldHeight.get().intValue());

                CompositorViewHolder compositorViewHolder =
                        (CompositorViewHolder) mVrTestRule.getActivity().findViewById(
                                R.id.compositor_view_holder);
                compositorViewHolder.onExitVr();
                Assert.assertNotEquals("Viewport width should have changed after the "
                                + "CompositorViewHolder was re-attached",
                        webContents.getHeight(), oldHeight.get().intValue());
                Assert.assertNotEquals("Viewport width should have changed after the "
                                + "CompositorViewHolder was re-attached",
                        webContents.getWidth(), oldWidth.get().intValue());
            }
        });
    }
}
