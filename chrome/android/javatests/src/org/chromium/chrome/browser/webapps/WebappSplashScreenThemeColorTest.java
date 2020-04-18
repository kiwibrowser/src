// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.annotation.TargetApi;
import android.graphics.Color;
import android.os.Build;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.metrics.WebappUma;
import org.chromium.chrome.browser.tab.TabTestUtils;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.concurrent.Callable;

/**
 * Tests for splash screens with EXTRA_THEME_COLOR specified in the Intent.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class WebappSplashScreenThemeColorTest {
    @Rule
    public final WebappActivityTestRule mActivityTestRule = new WebappActivityTestRule();

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startWebappActivityAndWaitForSplashScreen(
                mActivityTestRule
                        .createIntent()
                        // This is setting Color.Magenta with 50% opacity.
                        .putExtra(ShortcutHelper.EXTRA_THEME_COLOR, 0x80FF00FFL));
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void testThemeColorWhenSpecified() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return;

        Assert.assertEquals(ColorUtils.getDarkenedColorForStatusBar(Color.MAGENTA),
                mActivityTestRule.getActivity().getWindow().getStatusBarColor());
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void testThemeColorNotUsedIfPagesHasOne() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return;

        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                TabTestUtils.simulateChangeThemeColor(
                        mActivityTestRule.getActivity().getActivityTab(), Color.GREEN);
            }
        });

        // Waits for theme-color to change so the test doesn't rely on system timing.
        CriteriaHelper.pollInstrumentationThread(Criteria.equals(
                ColorUtils.getDarkenedColorForStatusBar(Color.GREEN), new Callable<Integer>() {
                    @Override
                    public Integer call() {
                        return mActivityTestRule.getActivity().getWindow().getStatusBarColor();
                    }
                }));
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testUmaThemeColorCustom() {
        Assert.assertEquals(1,
                RecordHistogram.getHistogramValueCountForTesting(
                        WebappUma.HISTOGRAM_SPLASHSCREEN_THEMECOLOR,
                        WebappUma.SPLASHSCREEN_COLOR_STATUS_CUSTOM));
    }
}
