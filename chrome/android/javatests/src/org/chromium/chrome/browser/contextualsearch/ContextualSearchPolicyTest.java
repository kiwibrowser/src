// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

import java.util.ArrayList;

/**
 * Tests for the ContextualSearchPolicy class.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class ContextualSearchPolicyTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    ContextualSearchPolicy mPolicy;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                mPolicy = new ContextualSearchPolicy(null, null);
            }
        });
    }

    @Test
    @SmallTest
    @Feature({"ContextualSearch"})
    @RetryOnFailure
    public void testBestTargetLanguageFromMultiple() {
        ArrayList<String> list = new ArrayList<String>();
        list.add("br");
        list.add("de");
        Assert.assertEquals("br", mPolicy.bestTargetLanguage(list));
    }

    @Test
    @SmallTest
    @Feature({"ContextualSearch"})
    @RetryOnFailure
    public void testBestTargetLanguageSkipsEnglish() {
        String countryOfUx = "";
        ArrayList<String> list = new ArrayList<String>();
        list.add("en");
        list.add("id");
        Assert.assertEquals("id", mPolicy.bestTargetLanguage(list, countryOfUx));
    }

    @Test
    @SmallTest
    @Feature({"ContextualSearch"})
    @RetryOnFailure
    public void testBestTargetLanguageReturnsEnglishWhenInUS() {
        String countryOfUx = "US";
        ArrayList<String> list = new ArrayList<String>();
        list.add("en");
        list.add("id");
        Assert.assertEquals("en", mPolicy.bestTargetLanguage(list, countryOfUx));
    }

    @Test
    @SmallTest
    @Feature({"ContextualSearch"})
    @RetryOnFailure
    public void testBestTargetLanguageUsesEnglishWhenOnlyChoice() {
        ArrayList<String> list = new ArrayList<String>();
        list.add("en");
        Assert.assertEquals("en", mPolicy.bestTargetLanguage(list));
    }

    @Test
    @SmallTest
    @Feature({"ContextualSearch"})
    public void testBestTargetLanguageReturnsEmptyWhenNoChoice() {
        ArrayList<String> list = new ArrayList<String>();
        Assert.assertEquals("", mPolicy.bestTargetLanguage(list));
    }

    // TODO(donnd): This set of tests is not complete, add more tests.
}
