// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.params.ParameterAnnotations;
import org.chromium.base.test.params.ParameterSet;
import org.chromium.base.test.params.ParameterizedRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SnippetsBridge;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4RunnerDelegate;
import org.chromium.chrome.test.util.NewTabPageTestUtils;
import org.chromium.chrome.test.util.browser.Features;

import java.util.Arrays;
import java.util.List;

/**
 * Misc. Content Suggestions instrumentation tests.
 */
@RunWith(ParameterizedRunner.class)
@ParameterAnnotations.UseRunnerDelegate(ChromeJUnit4RunnerDelegate.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class ContentSuggestionsTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @ParameterAnnotations.ClassParameter
    private static List<ParameterSet> sClassParams =
            Arrays.asList(new ParameterSet().value(false).name("DisableExpandableHeader"),
                    new ParameterSet().value(true).name("EnableExpandableHeader"));

    private final boolean mEnableExpandableHeader;

    public ContentSuggestionsTest(boolean enableExpandableHeader) {
        mEnableExpandableHeader = enableExpandableHeader;
    }

    @Before
    public void setUp() throws InterruptedException {
        if (mEnableExpandableHeader) {
            Features.getInstance().enable(
                    ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER);
        } else {
            Features.getInstance().disable(
                    ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER);
        }
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @Test
    @SmallTest
    @Feature("Suggestions")
    @CommandLineFlags.Add("enable-features=ContentSuggestionsSettings")
    public void testRemoteSuggestionsEnabled() throws InterruptedException {
        NewTabPage ntp = loadNTPWithSearchSuggestState(true);
        SuggestionsUiDelegate uiDelegate = ntp.getManagerForTesting();
        Assert.assertTrue(
                isCategoryEnabled(uiDelegate.getSuggestionsSource(), KnownCategories.ARTICLES));
    }

    @Test
    @SmallTest
    @Feature("Suggestions")
    @CommandLineFlags.Add("enable-features=ContentSuggestionsSettings")
    public void testRemoteSuggestionsDisabled() throws InterruptedException {
        NewTabPage ntp = loadNTPWithSearchSuggestState(false);
        SuggestionsUiDelegate uiDelegate = ntp.getManagerForTesting();
        // If header is expandable, category should still be enabled.
        Assert.assertEquals(mEnableExpandableHeader,
                isCategoryEnabled(uiDelegate.getSuggestionsSource(), KnownCategories.ARTICLES));
    }

    private NewTabPage loadNTPWithSearchSuggestState(final boolean enabled)
            throws InterruptedException {
        Tab tab = mActivityTestRule.getActivity().getActivityTab();
        ThreadUtils.runOnUiThreadBlocking(
                () -> PrefServiceBridge.getInstance().setSearchSuggestEnabled(enabled));

        mActivityTestRule.loadUrl(UrlConstants.NTP_URL);
        NewTabPageTestUtils.waitForNtpLoaded(tab);

        Assert.assertTrue(tab.getNativePage() instanceof NewTabPage);
        return (NewTabPage) tab.getNativePage();
    }

    private boolean isCategoryEnabled(SuggestionsSource source, @CategoryInt int category) {
        int[] categories = source.getCategories();
        for (int cat : categories) {
            if (cat != category) continue;
            if (SnippetsBridge.isCategoryEnabled(source.getCategoryStatus(category))) return true;
        }

        return false;
    }
}
