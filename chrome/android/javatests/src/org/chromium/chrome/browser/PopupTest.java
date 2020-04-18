// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.text.TextUtils;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.FlakyTest;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.infobar.InfoBar;
import org.chromium.chrome.browser.infobar.InfoBarContainer;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.safe_browsing.SafeBrowsingApiBridge;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content.browser.test.util.TouchCommon;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.ArrayList;

/**
 * Tests whether popup windows appear.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@RetryOnFailure
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PopupTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static final String POPUP_HTML_PATH = "/chrome/test/data/android/popup_test.html";

    private static final String METADATA_FOR_ABUSIVE_ENFORCEMENT =
            "{\"matches\":[{\"threat_type\":\"13\",\"sf_absv\":\"\"}]}";

    private String mPopupHtmlUrl;
    private EmbeddedTestServer mTestServer;

    private int getNumInfobarsShowing() {
        return mActivityTestRule.getInfoBars().size();
    }

    @Before
    public void setUp() throws Exception {
        // Create a new temporary instance to ensure the Class is loaded. Otherwise we will get a
        // ClassNotFoundException when trying to instantiate during startup.
        SafeBrowsingApiBridge.setSafeBrowsingHandlerType(
                new MockSafeBrowsingApiHandler().getClass());
        mActivityTestRule.startMainActivityOnBlankPage();

        ThreadUtils.runOnUiThread(() -> Assert.assertTrue(getNumInfobarsShowing() == 0));

        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        mPopupHtmlUrl = mTestServer.getURL(POPUP_HTML_PATH);
    }

    @After
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
        MockSafeBrowsingApiHandler.clearMockResponses();
    }

    @Test
    @MediumTest
    @Feature({"Popup"})
    public void testPopupInfobarAppears() throws Exception {
        mActivityTestRule.loadUrl(mPopupHtmlUrl);
        CriteriaHelper.pollUiThread(Criteria.equals(1, () -> getNumInfobarsShowing()));
    }

    @Test
    @MediumTest
    @Feature({"Popup"})
    public void testSafeGestureTabNotBlocked() throws Exception {
        final TabModelSelector selector = mActivityTestRule.getActivity().getTabModelSelector();

        String url = mTestServer.getURL("/chrome/test/data/android/popup_on_click.html");

        mActivityTestRule.loadUrl(url);
        CriteriaHelper.pollUiThread(Criteria.equals(0, () -> getNumInfobarsShowing()));
        DOMUtils.clickNode(
                mActivityTestRule.getActivity().getActivityTab().getWebContents(), "link");
        CriteriaHelper.pollUiThread(Criteria.equals(0, () -> getNumInfobarsShowing()));
    }

    @Test
    @MediumTest
    @Feature({"Popup"})
    public void testAbusiveGesturePopupBlocked() throws Exception {
        final TabModelSelector selector = mActivityTestRule.getActivity().getTabModelSelector();

        String url = mTestServer.getURL("/chrome/test/data/android/popup_on_click.html");
        MockSafeBrowsingApiHandler.addMockResponse(url, METADATA_FOR_ABUSIVE_ENFORCEMENT);

        mActivityTestRule.loadUrl(url);
        CriteriaHelper.pollUiThread(Criteria.equals(0, () -> getNumInfobarsShowing()));
        DOMUtils.clickNode(
                mActivityTestRule.getActivity().getActivityTab().getWebContents(), "link");
        CriteriaHelper.pollUiThread(Criteria.equals(1, () -> getNumInfobarsShowing()));
        Assert.assertEquals(1, selector.getTotalTabCount());
    }

    @Test
    @MediumTest
    @Feature({"Popup"})
    @FlakyTest(message = "crbug.com/771103")
    public void testPopupWindowsAppearWhenAllowed() throws Exception {
        final TabModelSelector selector = mActivityTestRule.getActivity().getTabModelSelector();

        mActivityTestRule.loadUrl(mPopupHtmlUrl);
        CriteriaHelper.pollUiThread(Criteria.equals(1, () -> getNumInfobarsShowing()));
        Assert.assertEquals(1, selector.getTotalTabCount());
        final InfoBarContainer container = selector.getCurrentTab().getInfoBarContainer();
        ArrayList<InfoBar> infobars = container.getInfoBarsForTesting();
        Assert.assertEquals(1, infobars.size());

        // Wait until the animations are done, then click the "open popups" button.
        final InfoBar infobar = infobars.get(0);
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return !container.isAnimating();
            }
        });
        TouchCommon.singleClickView(infobar.getView().findViewById(R.id.button_primary));

        // Document mode popups appear slowly and sequentially to prevent Android from throwing them
        // away, so use a long timeout.  http://crbug.com/498920.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                if (getNumInfobarsShowing() != 0) return false;
                return TextUtils.equals("Two", selector.getCurrentTab().getTitle());
            }
        }, 7500, CriteriaHelper.DEFAULT_POLLING_INTERVAL);

        Assert.assertEquals(3, selector.getTotalTabCount());
        int currentTabId = selector.getCurrentTab().getId();

        // Test that revisiting the original page makes popup windows immediately.
        mActivityTestRule.loadUrl(mPopupHtmlUrl);
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                if (getNumInfobarsShowing() != 0) return false;
                if (selector.getTotalTabCount() != 5) return false;
                return TextUtils.equals("Two", selector.getCurrentTab().getTitle());
            }
        }, 7500, CriteriaHelper.DEFAULT_POLLING_INTERVAL);
        Assert.assertNotSame(currentTabId, selector.getCurrentTab().getId());
    }
}
