// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_shell_apk.ContentShellActivityTestRule;

import java.util.concurrent.TimeoutException;

/**
 * Class which provides test coverage for Popup Zoomer.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@RetryOnFailure
public class ContentViewPopupZoomerTest {
    private static final String TARGET_NODE_ID = "target";

    @Rule
    public ContentShellActivityTestRule mActivityTestRule = new ContentShellActivityTestRule();

    private static PopupZoomer findPopupZoomer(ViewGroup view) {
        assert view != null;
        for (int i = 0; i < view.getChildCount(); i++) {
            View child = view.getChildAt(i);
            if (child instanceof PopupZoomer) return (PopupZoomer) child;
        }
        return null;
    }

    private static class PopupShowingCriteria extends Criteria {
        private final ViewGroup mView;
        private final boolean mShouldBeShown;
        public PopupShowingCriteria(ViewGroup view, boolean shouldBeShown) {
            super(shouldBeShown ? "Popup did not get shown." : "Popup shown incorrectly.");
            mView = view;
            mShouldBeShown = shouldBeShown;
        }
        @Override
        public boolean isSatisfied() {
            PopupZoomer popup = findPopupZoomer(mView);
            boolean isVisibilitySet = popup == null ? false : popup.getVisibility() == View.VISIBLE;
            return isVisibilitySet ? mShouldBeShown : !mShouldBeShown;
        }
    }

    private static class PopupHasNonZeroDimensionsCriteria extends Criteria {
        private final ViewGroup mView;
        public PopupHasNonZeroDimensionsCriteria(ViewGroup view) {
            super("The zoomer popup has zero dimensions.");
            mView = view;
        }
        @Override
        public boolean isSatisfied() {
            PopupZoomer popup = findPopupZoomer(mView);
            if (popup == null) return false;
            return popup.getWidth() != 0 && popup.getHeight() != 0;
        }
    }

    /**
     * Creates a webpage that has a couple links next to one another with a zero-width node between
     * them. Clicking on the zero-width node should trigger the popup zoomer to appear.
     */
    private String generateTestUrl() {
        final StringBuilder testUrl = new StringBuilder();
        testUrl.append("<html><body>");
        testUrl.append("<a href=\"javascript:void(0);\">A</a>");
        testUrl.append("<a id=\"" + TARGET_NODE_ID + "\"></a>");
        testUrl.append("<a href=\"javascript:void(0);\">Z</a>");
        testUrl.append("</body></html>");
        return UrlUtils.encodeHtmlDataUri(testUrl.toString());
    }

    public ContentViewPopupZoomerTest() {
    }

    /**
     * Tests that shows a zoomer popup and makes sure it has valid dimensions.
     */
    @Test
    @MediumTest
    @Feature({"Browser"})
    public void testPopupZoomerShowsUp() throws InterruptedException, TimeoutException {
        mActivityTestRule.launchContentShellWithUrl(generateTestUrl());
        mActivityTestRule.waitForActiveShellToBeDoneLoading();

        final WebContents webContents = mActivityTestRule.getWebContents();
        final ViewGroup view = webContents.getViewAndroidDelegate().getContainerView();

        // The popup should be hidden before the click.
        CriteriaHelper.pollInstrumentationThread(new PopupShowingCriteria(view, false));

        // Once clicked, the popup should show up.
        DOMUtils.clickNode(webContents, TARGET_NODE_ID);
        CriteriaHelper.pollInstrumentationThread(new PopupShowingCriteria(view, true));

        // The shown popup should have valid dimensions eventually.
        CriteriaHelper.pollInstrumentationThread(new PopupHasNonZeroDimensionsCriteria(view));
    }

    /**
     * Tests Popup zoomer hides when device back key is pressed.
     */
    @Test
    @MediumTest
    @Feature({"Browser"})
    @RetryOnFailure
    public void testBackKeyDismissesPopupZoomer() throws InterruptedException, TimeoutException {
        mActivityTestRule.launchContentShellWithUrl(generateTestUrl());
        mActivityTestRule.waitForActiveShellToBeDoneLoading();

        final WebContents webContents = mActivityTestRule.getWebContents();
        final ViewGroup view = webContents.getViewAndroidDelegate().getContainerView();

        CriteriaHelper.pollInstrumentationThread(new PopupShowingCriteria(view, false));
        DOMUtils.clickNode(webContents, TARGET_NODE_ID);
        CriteriaHelper.pollInstrumentationThread(new PopupShowingCriteria(view, true));
        InstrumentationRegistry.getInstrumentation().sendKeyDownUpSync(KeyEvent.KEYCODE_BACK);
        // When device key is pressed, popup zoomer should hide if already showing.
        CriteriaHelper.pollInstrumentationThread(new PopupShowingCriteria(view, false));
    }
}
