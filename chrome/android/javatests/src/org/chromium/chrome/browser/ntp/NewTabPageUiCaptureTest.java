// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.view.View;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.params.ParameterAnnotations.ClassParameter;
import org.chromium.base.test.params.ParameterAnnotations.UseRunnerDelegate;
import org.chromium.base.test.params.ParameterSet;
import org.chromium.base.test.params.ParameterizedRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.ntp.cards.NewTabPageRecyclerView;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.test.ScreenShooter;
import org.chromium.chrome.test.ChromeJUnit4RunnerDelegate;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.NewTabPageTestUtils;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.RecyclerViewTestUtils;
import org.chromium.chrome.test.util.browser.compositor.layouts.DisableChromeAnimations;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;

import java.util.Arrays;
import java.util.List;

/**
 * Capture the New Tab Page UI for UX review.
 */
@RunWith(ParameterizedRunner.class)
@UseRunnerDelegate(ChromeJUnit4RunnerDelegate.class)
@CommandLineFlags.Add({
        ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
})
public class NewTabPageUiCaptureTest {
    @ClassParameter
    private static List<ParameterSet> sClassParams =
            Arrays.asList(new ParameterSet().value(false).name("DisableNTPModernLayout"),
                    new ParameterSet().value(true).name("EnableNTPModernLayout"));
    @Rule
    public TestRule mFeaturesProcessor = new Features.InstrumentationProcessor();
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    @Rule
    public ScreenShooter mScreenShooter = new ScreenShooter();
    @Rule
    public TestRule mCreateSuggestions =
            new SuggestionsDependenciesRule(NtpUiCaptureTestData.createFactory());
    @Rule
    public TestRule mDisableChromeAnimations = new DisableChromeAnimations();

    private NewTabPage mNtp;

    private final boolean mEnableNTPModernLayout;

    public NewTabPageUiCaptureTest(boolean enableNTPModernLayout) {
        mEnableNTPModernLayout = enableNTPModernLayout;
    }

    @Before
    public void setUp() throws Exception {
        if (mEnableNTPModernLayout) {
            Features.getInstance().enable(ChromeFeatureList.NTP_MODERN_LAYOUT);
        } else {
            Features.getInstance().disable(ChromeFeatureList.NTP_MODERN_LAYOUT);
        }
        mActivityTestRule.startMainActivityWithURL(UrlConstants.NTP_URL);
        // TODO(aberent): this sequence or similar is used in a number of tests, extract to common
        // test method?
        Tab tab = mActivityTestRule.getActivity().getActivityTab();
        NewTabPageTestUtils.waitForNtpLoaded(tab);
        Assert.assertTrue(tab.getNativePage() instanceof NewTabPage);
        mNtp = (NewTabPage) tab.getNativePage();

        // When scrolling to a View, we wait until the View is no longer updating - when it is no
        // longer dirty. If scroll to load is triggered, the animated progress spinner will keep
        // the RecyclerView dirty as it is constantly updating.
        //
        // We do not want to disable the Scroll to Load feature entirely because its presence
        // effects other elements of the UI - it moves the Learn More link into the Context Menu.
        // Removing the ScrollToLoad listener from the RecyclerView allows us to prevent scroll to
        // load triggering while maintaining the UI otherwise.
        ThreadUtils.runOnUiThreadBlocking(
                () -> mNtp.getNewTabPageView().getRecyclerView().clearScrollToLoadListener());
    }

    @Test
    @MediumTest
    @Feature({"NewTabPage", "UiCatalogue"})
    public void testCaptureNewTabPage() {
        assertThat(ChromeFeatureList.isEnabled(ChromeFeatureList.NTP_MODERN_LAYOUT),
                is(mEnableNTPModernLayout));
        shoot("New Tab Page");

        // Scroll to search bar
        final NewTabPageRecyclerView recyclerView = mNtp.getNewTabPageView().getRecyclerView();

        final View fakebox = mNtp.getView().findViewById(org.chromium.chrome.R.id.search_box);
        final int firstScrollHeight = fakebox.getTop() + fakebox.getPaddingTop();
        final int subsequentScrollHeight = mNtp.getView().getHeight()
                - mActivityTestRule.getActivity().getToolbarManager().getToolbar().getHeight();

        ThreadUtils.runOnUiThreadBlocking(() -> { recyclerView.scrollBy(0, firstScrollHeight); });
        RecyclerViewTestUtils.waitForStableRecyclerView(recyclerView);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        shoot("New Tab Page scrolled");

        ThreadUtils.runOnUiThreadBlocking(
                () -> { recyclerView.scrollBy(0, subsequentScrollHeight); });
        RecyclerViewTestUtils.waitForStableRecyclerView(recyclerView);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        shoot("New Tab Page scrolled twice");

        ThreadUtils.runOnUiThreadBlocking(
                () -> { recyclerView.scrollBy(0, subsequentScrollHeight); });
        RecyclerViewTestUtils.waitForStableRecyclerView(recyclerView);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        shoot("New Tab Page scrolled thrice");
    }

    /**
     * Takes a screenshot with the given name. Applies a suffix to the name to differentiate
     * parameterized features.
     * @param shotName The shot name.
     */
    private void shoot(String shotName) {
        mScreenShooter.shoot(shotName + (mEnableNTPModernLayout ? "_modern" : ""));
    }
}
