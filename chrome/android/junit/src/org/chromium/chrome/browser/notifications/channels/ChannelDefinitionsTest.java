// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications.channels;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.everyItem;
import static org.hamcrest.Matchers.isIn;
import static org.hamcrest.Matchers.not;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;

/**
 * Java unit tests for ChannelDefinitions.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class ChannelDefinitionsTest {
    @Rule
    public TestRule processor = new Features.JUnitProcessor();

    @Test
    @EnableFeatures(ChromeFeatureList.SITE_NOTIFICATION_CHANNELS)
    public void testNoOverlapBetweenStartupAndLegacyChannelIds() throws Exception {
        assertThat(ChannelDefinitions.getStartupChannelIds(),
                everyItem(not(isIn(ChannelDefinitions.getLegacyChannelIds()))));
    }
}