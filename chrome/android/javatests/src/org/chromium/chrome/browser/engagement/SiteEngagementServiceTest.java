// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.engagement;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Test for the Site Engagement Service Java binding.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class SiteEngagementServiceTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    /**
     * Verify that setting the engagement score for a URL and reading it back it works.
     */
    @Test
    @SmallTest
    @Feature({"Engagement"})
    public void testSettingAndRetrievingScore() throws Throwable {
        mActivityTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final String url = "https://www.example.com";
                SiteEngagementService service = SiteEngagementService.getForProfile(
                        mActivityTestRule.getActivity().getActivityTab().getProfile());

                Assert.assertEquals(0.0, service.getScore(url), 0);
                service.resetBaseScoreForUrl(url, 5.0);
                Assert.assertEquals(5.0, service.getScore(url), 0);

                service.resetBaseScoreForUrl(url, 2.0);
                Assert.assertEquals(2.0, service.getScore(url), 0);
            }
        });
    }

    /**
     * Verify that repeatedly fetching and throwing away the SiteEngagementService works.
     */
    @Test
    @SmallTest
    @Feature({"Engagement"})
    public void testRepeatedlyGettingService() throws Throwable {
        mActivityTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final String url = "https://www.example.com";
                Profile profile = mActivityTestRule.getActivity().getActivityTab().getProfile();

                Assert.assertEquals(
                        0.0, SiteEngagementService.getForProfile(profile).getScore(url), 0);
                SiteEngagementService.getForProfile(profile).resetBaseScoreForUrl(url, 5.0);
                Assert.assertEquals(
                        5.0, SiteEngagementService.getForProfile(profile).getScore(url), 0);

                SiteEngagementService.getForProfile(profile).resetBaseScoreForUrl(url, 2.0);
                Assert.assertEquals(
                        2.0, SiteEngagementService.getForProfile(profile).getScore(url), 0);
            }
        });
    }

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
    }
}
