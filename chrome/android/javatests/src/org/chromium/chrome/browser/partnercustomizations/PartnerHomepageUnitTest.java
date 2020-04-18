// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.partnercustomizations;

import static org.chromium.chrome.browser.partnercustomizations.BasePartnerBrowserCustomizationUnitTestRule.DEFAULT_TIMEOUT_MS;
import static org.chromium.chrome.browser.partnercustomizations.BasePartnerBrowserCustomizationUnitTestRule.PARTNER_BROWSER_CUSTOMIZATIONS_DELAYED_PROVIDER;
import static org.chromium.chrome.browser.partnercustomizations.BasePartnerBrowserCustomizationUnitTestRule.PARTNER_BROWSER_CUSTOMIZATIONS_NO_PROVIDER;
import static org.chromium.chrome.browser.partnercustomizations.BasePartnerBrowserCustomizationUnitTestRule.PARTNER_BROWSER_CUSTOMIZATIONS_PROVIDER;

import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.partnercustomizations.TestPartnerBrowserCustomizationsDelayedProvider;
import org.chromium.chrome.test.partnercustomizations.TestPartnerBrowserCustomizationsProvider;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;

/**
 * Unit test suite for partner homepage.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@DisableFeatures(ChromeFeatureList.HOME_PAGE_BUTTON_FORCE_ENABLED)
public class PartnerHomepageUnitTest {
    @Rule
    public TestRule mFeaturesProcesser = new Features.JUnitProcessor();

    @Rule
    public BasePartnerBrowserCustomizationUnitTestRule mTestRule =
            new BasePartnerBrowserCustomizationUnitTestRule();

    public static final String TAG = "PartnerHomepageUnitTest";

    private static final String TEST_CUSTOM_HOMEPAGE_URI = "http://chrome.com";

    private HomepageManager mHomepageManager;

    @Before
    public void setUp() throws Exception {
        mHomepageManager = HomepageManager.getInstance();
        RecordHistogram.setDisabledForTests(true);
        Assert.assertNotNull(mHomepageManager);

        Assert.assertNotSame(
                TestPartnerBrowserCustomizationsProvider.HOMEPAGE_URI, TEST_CUSTOM_HOMEPAGE_URI);
        PartnerBrowserCustomizations.ignoreBrowserProviderSystemPackageCheckForTests(true);
    }

    @After
    public void tearDown() throws Exception {
        RecordHistogram.setDisabledForTests(false);
    }

    @Test
    @SmallTest
    @Feature({"Homepage"})
    public void testHomepageFeatureFlag() throws InterruptedException {
        // Checks that #isHomepageProviderAvailableAndEnabled returned false
        Assert.assertNull(PartnerBrowserCustomizations.getHomePageUrl());

        FeatureUtilities.resetHomePageButtonForceEnabledForTests();
        ChromePreferenceManager.getInstance().setHomePageButtonForceEnabled(true);
        Assert.assertTrue(HomepageManager.isHomepageEnabled());
        Assert.assertEquals(UrlConstants.NTP_NON_NATIVE_URL, HomepageManager.getHomepageUri());

        mHomepageManager.setPrefHomepageEnabled(false);
        Assert.assertFalse(HomepageManager.isHomepageEnabled());

        FeatureUtilities.resetHomePageButtonForceEnabledForTests();
        ChromePreferenceManager.getInstance().setHomePageButtonForceEnabled(false);
        mHomepageManager.setPrefHomepageEnabled(true);
        Assert.assertFalse(HomepageManager.isHomepageEnabled());

        // Test that a cached value (homepage enabled = false) is being read.
        ChromePreferenceManager.getInstance().setHomePageButtonForceEnabled(true);
        mHomepageManager.setPrefHomepageEnabled(true);
        Assert.assertFalse(HomepageManager.isHomepageEnabled());

        FeatureUtilities.resetHomePageButtonForceEnabledForTests();
    }

    /**
     * Everything is enabled for using partner homepage, except that there is no flag file.
     * Flaky: crbug.com/836700
     */
    @Test
    @SmallTest
    @Feature({"Homepage"})
    @DisabledTest(message = "crbug.com/836700")
    public void testProviderNotFromSystemPackage() throws InterruptedException {
        mHomepageManager.setPrefHomepageEnabled(true);
        mHomepageManager.setPrefHomepageUseDefaultUri(true);
        mHomepageManager.setPrefHomepageCustomUri(TEST_CUSTOM_HOMEPAGE_URI);

        // Note that unlike other tests in this file, we test if Chrome ignores a customizations
        // provider that is not from a system package.
        PartnerBrowserCustomizations.ignoreBrowserProviderSystemPackageCheckForTests(false);
        PartnerBrowserCustomizations.setProviderAuthorityForTests(
                PARTNER_BROWSER_CUSTOMIZATIONS_PROVIDER);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PartnerBrowserCustomizations.initializeAsync(
                        mTestRule.getContextWrapper(), DEFAULT_TIMEOUT_MS);
            }
        });
        PartnerBrowserCustomizations.setOnInitializeAsyncFinished(
                mTestRule.getCallback(), DEFAULT_TIMEOUT_MS);

        mTestRule.getCallbackLock().acquire();

        Assert.assertTrue(PartnerBrowserCustomizations.isInitialized());
        Assert.assertFalse(PartnerBrowserCustomizations.isHomepageProviderAvailableAndEnabled());
        Assert.assertNull(PartnerBrowserCustomizations.getHomePageUrl());
        Assert.assertFalse(HomepageManager.isHomepageEnabled());
        Assert.assertFalse(HomepageManager.shouldShowHomepageSetting());
        Assert.assertNull(HomepageManager.getHomepageUri());
    }

    /**
     * Everything is enabled for using partner homepage, except that there is no actual provider.
     * Flaky : http://crbug.com/836110
     */
    @Test
    @SmallTest
    @Feature({"Homepage"})
    @DisabledTest(message = "crbug.com/836110")
    public void testNoProvider() throws InterruptedException {
        mHomepageManager.setPrefHomepageEnabled(true);
        mHomepageManager.setPrefHomepageUseDefaultUri(true);
        mHomepageManager.setPrefHomepageCustomUri(TEST_CUSTOM_HOMEPAGE_URI);

        PartnerBrowserCustomizations.setProviderAuthorityForTests(
                PARTNER_BROWSER_CUSTOMIZATIONS_NO_PROVIDER);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PartnerBrowserCustomizations.initializeAsync(
                        mTestRule.getContextWrapper(), DEFAULT_TIMEOUT_MS);
            }
        });
        PartnerBrowserCustomizations.setOnInitializeAsyncFinished(
                mTestRule.getCallback(), DEFAULT_TIMEOUT_MS);
        mTestRule.getCallbackLock().acquire();

        Assert.assertTrue(PartnerBrowserCustomizations.isInitialized());
        Assert.assertFalse(PartnerBrowserCustomizations.isHomepageProviderAvailableAndEnabled());
        Assert.assertNull(PartnerBrowserCustomizations.getHomePageUrl());
        Assert.assertFalse(HomepageManager.isHomepageEnabled());
        Assert.assertFalse(HomepageManager.shouldShowHomepageSetting());
        Assert.assertNull(HomepageManager.getHomepageUri());
    }

    /**
     * Everything is enabled for using partner homepage, except that the homepage preference is
     * disabled.
     */
    @Test
    @SmallTest
    @Feature({"Homepage"})
    @RetryOnFailure
    public void testHomepageDisabled() throws InterruptedException {
        mHomepageManager.setPrefHomepageEnabled(false);
        mHomepageManager.setPrefHomepageUseDefaultUri(true);
        mHomepageManager.setPrefHomepageCustomUri(TEST_CUSTOM_HOMEPAGE_URI);

        PartnerBrowserCustomizations.setProviderAuthorityForTests(
                PARTNER_BROWSER_CUSTOMIZATIONS_PROVIDER);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PartnerBrowserCustomizations.initializeAsync(
                        mTestRule.getContextWrapper(), DEFAULT_TIMEOUT_MS);
            }
        });
        PartnerBrowserCustomizations.setOnInitializeAsyncFinished(
                mTestRule.getCallback(), DEFAULT_TIMEOUT_MS);

        mTestRule.getCallbackLock().acquire();

        Assert.assertTrue(PartnerBrowserCustomizations.isInitialized());
        Assert.assertTrue(PartnerBrowserCustomizations.isHomepageProviderAvailableAndEnabled());
        Assert.assertEquals(TestPartnerBrowserCustomizationsProvider.HOMEPAGE_URI,
                PartnerBrowserCustomizations.getHomePageUrl());
        Assert.assertFalse(HomepageManager.isHomepageEnabled());
        Assert.assertTrue(HomepageManager.shouldShowHomepageSetting());
        Assert.assertNull(HomepageManager.getHomepageUri());
    }

    /**
     * Everything is enabled for using partner homepage, except that the preference is set to use
     * custom user-specified homepage.
     */
    @Test
    @SmallTest
    @Feature({"Homepage"})
    @RetryOnFailure
    public void testCustomHomepage() throws InterruptedException {
        mHomepageManager.setPrefHomepageEnabled(true);
        mHomepageManager.setPrefHomepageUseDefaultUri(false);
        mHomepageManager.setPrefHomepageCustomUri(TEST_CUSTOM_HOMEPAGE_URI);

        PartnerBrowserCustomizations.setProviderAuthorityForTests(
                PARTNER_BROWSER_CUSTOMIZATIONS_PROVIDER);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PartnerBrowserCustomizations.initializeAsync(
                        mTestRule.getContextWrapper(), DEFAULT_TIMEOUT_MS);
            }
        });
        PartnerBrowserCustomizations.setOnInitializeAsyncFinished(
                mTestRule.getCallback(), DEFAULT_TIMEOUT_MS);

        mTestRule.getCallbackLock().acquire();

        Assert.assertTrue(PartnerBrowserCustomizations.isInitialized());
        Assert.assertTrue(PartnerBrowserCustomizations.isHomepageProviderAvailableAndEnabled());
        Assert.assertEquals(TestPartnerBrowserCustomizationsProvider.HOMEPAGE_URI,
                PartnerBrowserCustomizations.getHomePageUrl());
        Assert.assertTrue(HomepageManager.isHomepageEnabled());
        Assert.assertTrue(HomepageManager.shouldShowHomepageSetting());
        Assert.assertEquals(TEST_CUSTOM_HOMEPAGE_URI, HomepageManager.getHomepageUri());
    }

    /**
     * Everything is enabled for using partner homepage, but the homepage provider query takes
     * longer than the timeout we specify.
     */
    @Test
    @SmallTest
    @DisabledTest(message = "crbug.com/837311")
    @Feature({"Homepage"})
    public void testHomepageProviderTimeout() throws InterruptedException {
        mHomepageManager.setPrefHomepageEnabled(true);
        mHomepageManager.setPrefHomepageUseDefaultUri(true);
        mHomepageManager.setPrefHomepageCustomUri(TEST_CUSTOM_HOMEPAGE_URI);

        PartnerBrowserCustomizations.setProviderAuthorityForTests(
                PARTNER_BROWSER_CUSTOMIZATIONS_DELAYED_PROVIDER);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PartnerBrowserCustomizations.initializeAsync(mTestRule.getContextWrapper(), 500);
            }
        });
        PartnerBrowserCustomizations.setOnInitializeAsyncFinished(mTestRule.getCallback(), 300);

        mTestRule.getCallbackLock().acquire();

        Assert.assertFalse(PartnerBrowserCustomizations.isInitialized());
        Assert.assertFalse(PartnerBrowserCustomizations.isHomepageProviderAvailableAndEnabled());
        Assert.assertNull(PartnerBrowserCustomizations.getHomePageUrl());
        Assert.assertFalse(HomepageManager.isHomepageEnabled());
        Assert.assertFalse(HomepageManager.shouldShowHomepageSetting());
        Assert.assertNull(HomepageManager.getHomepageUri());

        PartnerBrowserCustomizations.setOnInitializeAsyncFinished(mTestRule.getCallback(), 2000);

        mTestRule.getCallbackLock().acquire();

        Assert.assertTrue(PartnerBrowserCustomizations.isInitialized());
        Assert.assertFalse(PartnerBrowserCustomizations.isHomepageProviderAvailableAndEnabled());
        Assert.assertNull(PartnerBrowserCustomizations.getHomePageUrl());
        Assert.assertFalse(HomepageManager.isHomepageEnabled());
        Assert.assertFalse(HomepageManager.shouldShowHomepageSetting());
        Assert.assertNull(HomepageManager.getHomepageUri());
    }

    /**
     * Everything is enabled for using partner homepage. The homepage provider query does not take
     * longer than the timeout we specify, but longer than the first async task wait timeout. This
     * scenario covers that the homepage provider is not ready at the cold startup initial homepage
     * tab, but be ready later than that.
     */
    @Test
    @SmallTest
    @Feature({"Homepage"})
    @DisabledTest(message = "crbug.com/837130")
    public void testHomepageProviderDelayed() throws InterruptedException {
        mHomepageManager.setPrefHomepageEnabled(true);
        mHomepageManager.setPrefHomepageUseDefaultUri(true);
        mHomepageManager.setPrefHomepageCustomUri(TEST_CUSTOM_HOMEPAGE_URI);

        PartnerBrowserCustomizations.setProviderAuthorityForTests(
                PARTNER_BROWSER_CUSTOMIZATIONS_DELAYED_PROVIDER);
        mTestRule.setDelayProviderUriPathForDelay(
                PartnerBrowserCustomizations.PARTNER_HOMEPAGE_PATH);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PartnerBrowserCustomizations.initializeAsync(mTestRule.getContextWrapper(), 2000);
            }
        });
        PartnerBrowserCustomizations.setOnInitializeAsyncFinished(mTestRule.getCallback(), 300);

        mTestRule.getCallbackLock().acquire();

        Assert.assertFalse(PartnerBrowserCustomizations.isInitialized());
        Assert.assertFalse(PartnerBrowserCustomizations.isHomepageProviderAvailableAndEnabled());
        Assert.assertNull(PartnerBrowserCustomizations.getHomePageUrl());
        Assert.assertFalse(HomepageManager.isHomepageEnabled());
        Assert.assertFalse(HomepageManager.shouldShowHomepageSetting());
        Assert.assertNull(HomepageManager.getHomepageUri());

        PartnerBrowserCustomizations.setOnInitializeAsyncFinished(mTestRule.getCallback(), 3000);

        mTestRule.getCallbackLock().acquire();

        Assert.assertTrue(PartnerBrowserCustomizations.isInitialized());
        Assert.assertTrue(PartnerBrowserCustomizations.isHomepageProviderAvailableAndEnabled());
        Assert.assertEquals(TestPartnerBrowserCustomizationsDelayedProvider.HOMEPAGE_URI,
                PartnerBrowserCustomizations.getHomePageUrl());
        Assert.assertTrue(HomepageManager.isHomepageEnabled());
        Assert.assertTrue(HomepageManager.shouldShowHomepageSetting());
        Assert.assertEquals(TestPartnerBrowserCustomizationsDelayedProvider.HOMEPAGE_URI,
                HomepageManager.getHomepageUri());
    }

    /**
     * Everything is enabled for using partner homepage. It should be able to successfully retrieve
     * homepage URI from the provider.
     */
    @Test
    @SmallTest
    @Feature({"Homepage"})
    @RetryOnFailure
    public void testReadHomepageProvider() throws InterruptedException {
        mHomepageManager.setPrefHomepageEnabled(true);
        mHomepageManager.setPrefHomepageUseDefaultUri(true);
        mHomepageManager.setPrefHomepageCustomUri(TEST_CUSTOM_HOMEPAGE_URI);

        PartnerBrowserCustomizations.setProviderAuthorityForTests(
                PARTNER_BROWSER_CUSTOMIZATIONS_PROVIDER);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PartnerBrowserCustomizations.initializeAsync(
                        mTestRule.getContextWrapper(), DEFAULT_TIMEOUT_MS);
            }
        });
        PartnerBrowserCustomizations.setOnInitializeAsyncFinished(
                mTestRule.getCallback(), DEFAULT_TIMEOUT_MS);

        mTestRule.getCallbackLock().acquire();

        Assert.assertTrue(PartnerBrowserCustomizations.isInitialized());
        Assert.assertTrue(PartnerBrowserCustomizations.isHomepageProviderAvailableAndEnabled());
        Assert.assertEquals(TestPartnerBrowserCustomizationsProvider.HOMEPAGE_URI,
                PartnerBrowserCustomizations.getHomePageUrl());
        Assert.assertTrue(HomepageManager.isHomepageEnabled());
        Assert.assertTrue(HomepageManager.shouldShowHomepageSetting());
        Assert.assertEquals(TestPartnerBrowserCustomizationsProvider.HOMEPAGE_URI,
                HomepageManager.getHomepageUri());
    }
}
