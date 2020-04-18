// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.metrics.WebappUma;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabTestUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Tests for splash screens.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class WebappSplashScreenTest {
    @Rule
    public final WebappActivityTestRule mActivityTestRule = new WebappActivityTestRule();

    private int getHistogramTotalCountFor(String histogram, int buckets) {
        int count = 0;

        for (int i = 0; i < buckets; ++i) {
            count += RecordHistogram.getHistogramValueCountForTesting(histogram, i);
        }

        return count;
    }

    private boolean hasHistogramEntry(String histogram, int maxValue) {
        for (int i = 0; i < maxValue; ++i) {
            if (RecordHistogram.getHistogramValueCountForTesting(histogram, i) > 0) {
                return true;
            }
        }
        return false;
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testDefaultBackgroundColor() throws Exception {
        ViewGroup splashScreen = mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        ColorDrawable background = (ColorDrawable) splashScreen.getBackground();

        Assert.assertEquals(
                ApiCompatibilityUtils.getColor(
                        mActivityTestRule.getActivity().getResources(), R.color.webapp_default_bg),
                background.getColor());
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void testThemeColorWhenNotSpecified() throws Exception {
        mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return;

        Assert.assertEquals(
                Color.BLACK, mActivityTestRule.getActivity().getWindow().getStatusBarColor());
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testHidesAfterFirstPaint() throws Exception {
        mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        Assert.assertTrue(mActivityTestRule.isSplashScreenVisible());

        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                TabTestUtils.simulateFirstVisuallyNonEmptyPaint(
                        mActivityTestRule.getActivity().getActivityTab());
            }
        });

        mActivityTestRule.waitUntilSplashscreenHides();
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testHidesAfterCrash() throws Throwable {
        mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        Assert.assertTrue(mActivityTestRule.isSplashScreenVisible());

        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                TabTestUtils.simulateCrash(mActivityTestRule.getActivity().getActivityTab(), true);
            }
        });

        mActivityTestRule.waitUntilSplashscreenHides();
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testHidesAfterLoadCompletes() throws Exception {
        mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        Assert.assertTrue(mActivityTestRule.isSplashScreenVisible());

        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                TabTestUtils.simulatePageLoadFinished(
                        mActivityTestRule.getActivity().getActivityTab());
            }
        });

        mActivityTestRule.waitUntilSplashscreenHides();
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testHidesAfterLoadFails() throws Exception {
        mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        Assert.assertTrue(mActivityTestRule.isSplashScreenVisible());

        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                TabTestUtils.simulatePageLoadFailed(
                        mActivityTestRule.getActivity().getActivityTab(), 0);
            }
        });

        mActivityTestRule.waitUntilSplashscreenHides();
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testHidesAfterMultipleEvents() throws Exception {
        mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        Assert.assertTrue(mActivityTestRule.isSplashScreenVisible());

        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Tab tab = mActivityTestRule.getActivity().getActivityTab();

                TabTestUtils.simulatePageLoadFinished(tab);
                TabTestUtils.simulatePageLoadFailed(tab, 0);
                TabTestUtils.simulateFirstVisuallyNonEmptyPaint(tab);
            }
        });

        mActivityTestRule.waitUntilSplashscreenHides();
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testUmaOnNativeLoad() throws Exception {
        mActivityTestRule.startWebappActivityAndWaitForSplashScreen();

        // Tests UMA values.
        Assert.assertEquals(1,
                RecordHistogram.getHistogramValueCountForTesting(
                        WebappUma.HISTOGRAM_SPLASHSCREEN_BACKGROUNDCOLOR,
                        WebappUma.SPLASHSCREEN_COLOR_STATUS_DEFAULT));
        Assert.assertEquals(1,
                RecordHistogram.getHistogramValueCountForTesting(
                        WebappUma.HISTOGRAM_SPLASHSCREEN_THEMECOLOR,
                        WebappUma.SPLASHSCREEN_COLOR_STATUS_DEFAULT));
        Assert.assertEquals(1,
                RecordHistogram.getHistogramValueCountForTesting(
                        WebappUma.HISTOGRAM_SPLASHSCREEN_ICON_TYPE,
                        WebappUma.SPLASHSCREEN_ICON_TYPE_NONE));

        // Tests UMA counts.
        Assert.assertEquals(1,
                getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_BACKGROUNDCOLOR,
                        WebappUma.SPLASHSCREEN_COLOR_STATUS_MAX));
        Assert.assertEquals(1,
                getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_THEMECOLOR,
                        WebappUma.SPLASHSCREEN_COLOR_STATUS_MAX));
        Assert.assertEquals(1,
                getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_ICON_TYPE,
                        WebappUma.SPLASHSCREEN_ICON_TYPE_MAX));

        // Given that there is no icon, the ICON_SIZE UMA should not be recorded.
        Assert.assertEquals(
                0, getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_ICON_SIZE, 50));

        // DURATION and HIDES UMA should not have been recorded yet.
        Assert.assertFalse(hasHistogramEntry(WebappUma.HISTOGRAM_SPLASHSCREEN_DURATION, 3000));
        Assert.assertEquals(0,
                getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_HIDES,
                        WebappUma.SPLASHSCREEN_HIDES_REASON_MAX));
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testUmaWhenSplashHides() throws Exception {
        mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                TabTestUtils.simulateFirstVisuallyNonEmptyPaint(
                        mActivityTestRule.getActivity().getActivityTab());
            }
        });

        mActivityTestRule.waitUntilSplashscreenHides();

        // DURATION and HIDES should now have a value.
        Assert.assertTrue(hasHistogramEntry(WebappUma.HISTOGRAM_SPLASHSCREEN_DURATION, 10000));
        Assert.assertEquals(1,
                getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_HIDES,
                        WebappUma.SPLASHSCREEN_HIDES_REASON_MAX));

        // The other UMA records should not have changed.
        Assert.assertEquals(1,
                getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_BACKGROUNDCOLOR,
                        WebappUma.SPLASHSCREEN_COLOR_STATUS_MAX));
        Assert.assertEquals(1,
                getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_THEMECOLOR,
                        WebappUma.SPLASHSCREEN_COLOR_STATUS_MAX));
        Assert.assertEquals(1,
                getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_ICON_TYPE,
                        WebappUma.SPLASHSCREEN_ICON_TYPE_MAX));
        Assert.assertEquals(
                0, getHistogramTotalCountFor(WebappUma.HISTOGRAM_SPLASHSCREEN_ICON_SIZE, 50));
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testRegularSplashScreenAppears() throws Exception {
        // Register a properly-sized icon for the splash screen.
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        int thresholdSize = context.getResources().getDimensionPixelSize(
                R.dimen.webapp_splash_image_size_threshold);
        int size = thresholdSize + 1;
        Bitmap splashBitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        String bitmapString = ShortcutHelper.encodeBitmapAsString(splashBitmap);

        TestFetchStorageCallback callback = new TestFetchStorageCallback();
        WebappRegistry.getInstance().register(WebappActivityTestRule.WEBAPP_ID, callback);
        callback.waitForCallback(0);
        callback.getStorage().updateSplashScreenImage(bitmapString);

        ViewGroup splashScreen = mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        Assert.assertTrue(mActivityTestRule.isSplashScreenVisible());

        ImageView splashImage =
                (ImageView) splashScreen.findViewById(R.id.webapp_splash_screen_icon);
        Assert.assertEquals(size, splashImage.getMeasuredWidth());
        Assert.assertEquals(size, splashImage.getMeasuredHeight());

        TextView splashText = (TextView) splashScreen.findViewById(R.id.webapp_splash_screen_name);
        int[] rules = ((RelativeLayout.LayoutParams) splashText.getLayoutParams()).getRules();
        Assert.assertEquals(RelativeLayout.TRUE, rules[RelativeLayout.ALIGN_PARENT_BOTTOM]);
        Assert.assertEquals(0, rules[RelativeLayout.BELOW]);
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testSmallSplashScreenAppears() throws Exception {
        // Register a smaller icon for the splash screen.
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        int thresholdSize = context.getResources().getDimensionPixelSize(
                R.dimen.webapp_splash_image_size_threshold);
        int size = context.getResources().getDimensionPixelSize(
                R.dimen.webapp_splash_image_size_minimum);
        Bitmap splashBitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        String bitmapString = ShortcutHelper.encodeBitmapAsString(splashBitmap);

        TestFetchStorageCallback callback = new TestFetchStorageCallback();
        WebappRegistry.getInstance().register(WebappActivityTestRule.WEBAPP_ID, callback);
        callback.waitForCallback(0);
        callback.getStorage().updateSplashScreenImage(bitmapString);

        ViewGroup splashScreen = mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        Assert.assertTrue(mActivityTestRule.isSplashScreenVisible());

        // The icon is centered within a fixed-size area on the splash screen.
        ImageView splashImage =
                (ImageView) splashScreen.findViewById(R.id.webapp_splash_screen_icon);
        Assert.assertEquals(thresholdSize, splashImage.getMeasuredWidth());
        Assert.assertEquals(thresholdSize, splashImage.getMeasuredHeight());

        // The web app name is anchored to the icon.
        TextView splashText = (TextView) splashScreen.findViewById(R.id.webapp_splash_screen_name);
        int[] rules = ((RelativeLayout.LayoutParams) splashText.getLayoutParams()).getRules();
        Assert.assertEquals(0, rules[RelativeLayout.ALIGN_PARENT_BOTTOM]);
        Assert.assertEquals(R.id.webapp_splash_screen_icon, rules[RelativeLayout.BELOW]);
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testSplashScreenWithoutImageAppears() throws Exception {
        // Register an image that's too small for the splash screen.
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        int size = context.getResources().getDimensionPixelSize(
                R.dimen.webapp_splash_image_size_minimum) - 1;
        Bitmap splashBitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        String bitmapString = ShortcutHelper.encodeBitmapAsString(splashBitmap);

        TestFetchStorageCallback callback = new TestFetchStorageCallback();
        WebappRegistry.getInstance().register(WebappActivityTestRule.WEBAPP_ID, callback);
        callback.waitForCallback(0);
        callback.getStorage().updateSplashScreenImage(bitmapString);

        ViewGroup splashScreen = mActivityTestRule.startWebappActivityAndWaitForSplashScreen(
                mActivityTestRule.createIntent().putExtra(
                        ShortcutHelper.EXTRA_IS_ICON_GENERATED, true));
        Assert.assertTrue(mActivityTestRule.isSplashScreenVisible());

        // There's no icon displayed.
        ImageView splashImage =
                (ImageView) splashScreen.findViewById(R.id.webapp_splash_screen_icon);
        Assert.assertNull(splashImage);

        View spacer = splashScreen.findViewById(R.id.webapp_splash_space);
        Assert.assertNotNull(spacer);

        // The web app name is anchored to the top of the spacer.
        TextView splashText = (TextView) splashScreen.findViewById(R.id.webapp_splash_screen_name);
        int[] rules = ((RelativeLayout.LayoutParams) splashText.getLayoutParams()).getRules();
        Assert.assertEquals(0, rules[RelativeLayout.ALIGN_PARENT_BOTTOM]);
        Assert.assertEquals(0, rules[RelativeLayout.BELOW]);
        Assert.assertEquals(R.id.webapp_splash_space, rules[RelativeLayout.ABOVE]);
    }

    @Test
    @SmallTest
    @Feature({"Webapps"})
    public void testSplashScreenAppearsWithoutRegisteredSplashImage() throws Exception {
        // Don't register anything for the web app, which represents apps that were added to the
        // home screen before splash screen images were downloaded.
        ViewGroup splashScreen = mActivityTestRule.startWebappActivityAndWaitForSplashScreen();
        Assert.assertTrue(mActivityTestRule.isSplashScreenVisible());

        // There's no icon displayed.
        ImageView splashImage =
                (ImageView) splashScreen.findViewById(R.id.webapp_splash_screen_icon);
        Assert.assertNull(splashImage);

        View spacer = splashScreen.findViewById(R.id.webapp_splash_space);
        Assert.assertNotNull(spacer);

        // The web app name is anchored to the top of the spacer.
        TextView splashText = (TextView) splashScreen.findViewById(R.id.webapp_splash_screen_name);
        int[] rules = ((RelativeLayout.LayoutParams) splashText.getLayoutParams()).getRules();
        Assert.assertEquals(0, rules[RelativeLayout.ALIGN_PARENT_BOTTOM]);
        Assert.assertEquals(0, rules[RelativeLayout.BELOW]);
        Assert.assertEquals(0, rules[RelativeLayout.CENTER_IN_PARENT]);
        Assert.assertEquals(R.id.webapp_splash_space, rules[RelativeLayout.ABOVE]);
    }
}
