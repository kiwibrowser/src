// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.input;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.WebContentsFactory;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.content_view.ContentView;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.ContentViewCore;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

/**
 * Test the select popup and how it interacts with another ContentViewCore.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class SelectPopupOtherContentViewTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static final String SELECT_URL = UrlUtils.encodeHtmlDataUri(
            "<html><body>"
            + "Which animal is the strongest:<br/>"
            + "<select id=\"select\">"
            + "<option>Black bear</option>"
            + "<option>Polar bear</option>"
            + "<option>Grizzly</option>"
            + "<option>Tiger</option>"
            + "<option>Lion</option>"
            + "<option>Gorilla</option>"
            + "<option>Chipmunk</option>"
            + "</select>"
            + "</body></html>");

    private class PopupShowingCriteria extends Criteria {
        public PopupShowingCriteria() {
            super("The select popup did not show up on click.");
        }

        @Override
        public boolean isSatisfied() {
            return mActivityTestRule.getActivity()
                    .getActivityTab()
                    .getWebContents()
                    .isSelectPopupVisibleForTesting();
        }
    }

    /**
     * Tests that the showing select popup does not get closed because an unrelated ContentView
     * gets destroyed.
     *
     */
    @Test
    @LargeTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testPopupNotClosedByOtherContentView()
            throws InterruptedException, Exception, Throwable {
        // Load the test page.
        mActivityTestRule.startMainActivityWithURL(SELECT_URL);

        // Once clicked, the popup should show up.
        DOMUtils.clickNode(mActivityTestRule.getActivity().getCurrentWebContents(), "select");
        CriteriaHelper.pollInstrumentationThread(new PopupShowingCriteria());

        // Now create and destroy a different ContentView.
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                WebContents webContents = WebContentsFactory.createWebContents(false, false);
                ChromeActivity activity = mActivityTestRule.getActivity();
                WindowAndroid windowAndroid = new ActivityWindowAndroid(activity);

                ContentView cv = ContentView.createContentView(activity, webContents);
                ContentViewCore contentViewCore = ContentViewCore.create(activity, "", webContents,
                        ViewAndroidDelegate.createBasicDelegate(cv), cv, windowAndroid);
                contentViewCore.destroy();
            }
        });

        // Process some more events to give a chance to the dialog to hide if it were to.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        // The popup should still be shown.
        ContentViewCore viewCore =
                mActivityTestRule.getActivity().getActivityTab().getContentViewCore();

        Assert.assertTrue("The select popup got hidden by destroying of unrelated ContentViewCore.",
                mActivityTestRule.getActivity()
                        .getActivityTab()
                        .getWebContents()
                        .isSelectPopupVisibleForTesting());
    }
}
