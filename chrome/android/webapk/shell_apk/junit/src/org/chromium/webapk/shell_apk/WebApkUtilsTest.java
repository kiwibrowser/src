// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.ResolveInfo;
import android.os.Bundle;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowPackageManager;

import org.chromium.testing.local.LocalRobolectricTestRunner;
import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;
import org.chromium.webapk.test.WebApkTestHelper;

/** Tests for WebApkUtils. */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE, packageName = WebApkUtilsTest.WEBAPK_PACKAGE_NAME)
public class WebApkUtilsTest {
    protected static final String WEBAPK_PACKAGE_NAME = "org.chromium.webapk.test_package";
    private static final String BROWSER_INSTALLED_SUPPORTING_WEBAPKS = "com.chrome.canary";
    private static final String BROWSER_UNINSTALLED_SUPPORTING_WEBAPKS = "com.chrome.dev";
    private static final String BROWSER_INSTALLED_NOT_SUPPORTING_WEBAPKS =
            "browser.installed.not.supporting.webapks";
    private static final String ANOTHER_BROWSER_INSTALLED_SUPPORTING_WEBAPKS = "com.chrome.beta";

    private static final String[] sInstalledBrowsers = {BROWSER_INSTALLED_NOT_SUPPORTING_WEBAPKS,
            BROWSER_INSTALLED_SUPPORTING_WEBAPKS, ANOTHER_BROWSER_INSTALLED_SUPPORTING_WEBAPKS};

    private Context mContext;
    private ShadowPackageManager mPackageManager;

    @Before
    public void setUp() {
        mContext = RuntimeEnvironment.application;

        mPackageManager = Shadows.shadowOf(mContext.getPackageManager());

        WebApkUtils.resetCachedHostPackageForTesting();
    }

    /**
     * Tests that null will be returned if there isn't any browser installed on the device.
     */
    @Test
    public void testReturnsNullWhenNoBrowserInstalled() {
        String hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertNull(hostBrowser);
    }

    /**
     * Tests that the package name of the host browser in the SharedPreference will be returned if
     * it is installed, even if a host browser is specified in the AndroidManifest.xml.
     */
    @Test
    public void testReturnsHostBrowserInSharedPreferenceIfInstalled() {
        String expectedHostBrowser = BROWSER_INSTALLED_SUPPORTING_WEBAPKS;
        mockInstallBrowsers(sInstalledBrowsers, ANOTHER_BROWSER_INSTALLED_SUPPORTING_WEBAPKS);
        setHostBrowserInMetadata(ANOTHER_BROWSER_INSTALLED_SUPPORTING_WEBAPKS);
        setHostBrowserInSharedPreferences(expectedHostBrowser);

        String hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertEquals(expectedHostBrowser, hostBrowser);
    }

    /**
     * Test that MainActivity appends the start URL as a paramater if |loggedIntentUrlParam| in
     * WebAPK metadata is set and {@link intentStartUrl} is outside of the scope specified in the
     * manifest meta data.
     */
    @Test
    public void testLoggedIntentUrlParamWhenRewriteOutOfScope() {
        final String intentStartUrl = "https://maps.google.com/page?a=A";
        final String manifestStartUrl = "https://www.google.com/maps";
        final String manifestScope = "https://www.google.com";
        final String expectedRewrittenStartUrl =
                "https://www.google.com/maps?originalUrl=https%3A%2F%2Fmaps.google.com%2Fpage%3Fa%3DA";
        final String browserPackageName = "browser.support.webapks";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        bundle.putString(WebApkMetaDataKeys.LOGGED_INTENT_URL_PARAM, "originalUrl");

        Assert.assertEquals(expectedRewrittenStartUrl,
                WebApkUtils.rewriteIntentUrlIfNecessary(intentStartUrl, bundle));
    }

    /**
     * Test that MainActivity appends the start URL as a paramater if |loggedIntentUrlParam| in
     * WebAPK metadata is set and {@link intentStartUrl} is in the scope specified in the manifest
     * meta data.
     */
    @Test
    public void testLoggedIntentUrlParamWhenRewriteInScope() {
        final String intentStartUrl = "https://www.google.com/maps/search/A";
        final String manifestStartUrl = "https://www.google.com/maps?force=qVTs2FOxxTmHHo79-pwa";
        final String manifestScope = "https://www.google.com";
        final String expectedRewrittenStartUrl =
                "https://www.google.com/maps?force=qVTs2FOxxTmHHo79-pwa&intent="
                + "https%3A%2F%2Fwww.google.com%2Fmaps%2Fsearch%2FA";
        final String browserPackageName = "browser.support.webapks";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        bundle.putString(WebApkMetaDataKeys.LOGGED_INTENT_URL_PARAM, "intent");

        Assert.assertEquals(expectedRewrittenStartUrl,
                WebApkUtils.rewriteIntentUrlIfNecessary(intentStartUrl, bundle));
    }

    /**
     * This is a test for the WebAPK WITH a runtime host specified in its AndroidManifest.xml.
     * Tests that the package name of the host browser specified in the AndroidManifest.xml will be
     * returned if:
     * 1. there isn't a host browser specified in the SharedPreference or the specified one is
     *    uninstalled.
     * And
     * 2. the host browser stored in the AndroidManifest is still installed.
     */
    @Test
    public void testReturnsHostBrowserInManifestIfInstalled() {
        String expectedHostBrowser = BROWSER_INSTALLED_SUPPORTING_WEBAPKS;
        mockInstallBrowsers(sInstalledBrowsers, ANOTHER_BROWSER_INSTALLED_SUPPORTING_WEBAPKS);
        setHostBrowserInMetadata(expectedHostBrowser);
        // Simulates there isn't any host browser stored in the SharedPreference.
        setHostBrowserInSharedPreferences(null);

        String hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertEquals(hostBrowser, expectedHostBrowser);

        WebApkUtils.resetCachedHostPackageForTesting();
        // Simulates there is a host browser stored in the SharedPreference but uninstalled.
        setHostBrowserInSharedPreferences(BROWSER_UNINSTALLED_SUPPORTING_WEBAPKS);
        hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertEquals(expectedHostBrowser, hostBrowser);
    }

    /**
     * This is a test for the WebAPK WITH a runtime host specified in its AndroidManifest.xml.
     * Tests that we will return NULL if the runtime host is not installed, even if we have only one
     * other browser that supports WebAPK. This test is to ensure we don't fall into auto-selecting
     * browser for users, if browser is specified in WebAPK AndroidManifest.xml.
     */
    @Test
    public void testReturnsNullIfHostBrowserInManifestNotFoundAndAnotherBrowserSupportingWebApk() {
        mockInstallBrowsers(new String[] {ANOTHER_BROWSER_INSTALLED_SUPPORTING_WEBAPKS}, null);
        setHostBrowserInMetadata(BROWSER_UNINSTALLED_SUPPORTING_WEBAPKS);
        setHostBrowserInSharedPreferences(null);

        String hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertNull(hostBrowser);
    }

    /**
     * This is a test for the WebAPK WITHOUT any runtime host specified in its AndroidManifest.xml.
     * Tests that it will return package name of browser which supports WebAPKs if:
     * 1. there isn't any host browser stored in the SharedPreference, or the specified one has
     *    been uninstalled.
     * 2. the default browser does not support WebAPKs.
     * 3. only one of the installed browsers supports WebAPKs.
     * In this test, we only simulate the the first part of the condition 1.
     */
    @Test
    public void testReturnsOtherBrowserIfDefaultBrowserNotSupportingWebApk() {
        String defaultBrowser = BROWSER_INSTALLED_NOT_SUPPORTING_WEBAPKS;
        mockInstallBrowsers(new String[] {ANOTHER_BROWSER_INSTALLED_SUPPORTING_WEBAPKS,
                BROWSER_INSTALLED_NOT_SUPPORTING_WEBAPKS},
                defaultBrowser);
        setHostBrowserInMetadata(null);
        setHostBrowserInSharedPreferences(null);

        String hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertEquals(ANOTHER_BROWSER_INSTALLED_SUPPORTING_WEBAPKS, hostBrowser);
    }

    /**
     * This is a test for the WebAPK WITHOUT any runtime host specified in its AndroidManifest.xml.
     * Tests that the default browser package name will be returned if:
     * 1. there isn't any host browser stored in the SharedPreference, or the specified one has
     *    been uninstalled.
     * And
     * 2. the default browser supports WebAPKs.
     * In this test, we only simulate the the first part of the condition 1.
     */
    @Test
    public void testReturnsDefaultBrowser() {
        String defaultBrowser = BROWSER_INSTALLED_SUPPORTING_WEBAPKS;
        mockInstallBrowsers(sInstalledBrowsers, defaultBrowser);
        setHostBrowserInMetadata(null);
        // Simulates that there isn't any host browser stored in the SharedPreference.
        setHostBrowserInSharedPreferences(null);

        String hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertEquals(defaultBrowser, hostBrowser);
    }

    /**
     * This is a test for the WebAPK WITHOUT any runtime host specified in its AndroidManifest.xml.
     * Tests that null will be returned if:
     * 1. there isn't any host browser stored in the SharedPreference, or the specified one has
     *    been uninstalled.
     * And
     * 2. the default browser doesn't support WebAPKs.
     * In this test, we only simulate the the first part of the condition 1.
     */
    @Test
    public void testReturnsNullWhenDefaultBrowserDoesNotSupportWebApks() {
        mockInstallBrowsers(sInstalledBrowsers, BROWSER_INSTALLED_NOT_SUPPORTING_WEBAPKS);
        setHostBrowserInMetadata(null);
        setHostBrowserInSharedPreferences(null);

        String hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertNull(hostBrowser);
    }

    /**
     * Tests that {@link WebApkUtils#getHostBrowserPackageName(Context)} doesn't return the current
     * host browser which is cached in the {@link WebApkUtils#sHostPackage} and uninstalled.
     */
    @Test
    public void testDoesNotReturnTheCurrentHostBrowserAfterUninstall() {
        String currentHostBrowser = BROWSER_INSTALLED_SUPPORTING_WEBAPKS;
        mockInstallBrowsers(sInstalledBrowsers, ANOTHER_BROWSER_INSTALLED_SUPPORTING_WEBAPKS);
        setHostBrowserInMetadata(null);
        setHostBrowserInSharedPreferences(currentHostBrowser);

        String hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertEquals(currentHostBrowser, hostBrowser);

        uninstallBrowser(currentHostBrowser);
        hostBrowser = WebApkUtils.getHostBrowserPackageName(mContext);
        Assert.assertNotEquals(currentHostBrowser, hostBrowser);
    }

    /**
     * Tests that a WebAPK should be launched as a tab if Chrome's version number is lower than
     * {@link WebApkUtils#MINIMUM_REQUIRED_CHROME_VERSION}.
     */
    @Test
    public void testShouldLaunchInTabWhenChromeVersionIsTooLow() {
        String versionName = "56.0.0000.0";
        Assert.assertTrue(WebApkUtils.shouldLaunchInTab(versionName));
    }

    /**
     * Tests that a WebAPK should not be launched as a tab if Chrome's version is higher or equal to
     * {@link WebApkUtils#MINIMUM_REQUIRED_CHROME_VERSION}.
     */
    @Test
    public void testShouldNotLaunchInTabWithNewVersionOfChrome() {
        String versionName = "57.0.0000.0";
        Assert.assertFalse(WebApkUtils.shouldLaunchInTab(versionName));
    }

    /** Tests that a WebAPK should not be launched as a tab in a developer build of Chrome. */
    @Test
    public void testShouldNotLaunchInTabWithDevBuild() {
        String versionName = "Developer Build";
        Assert.assertFalse(WebApkUtils.shouldLaunchInTab(versionName));
    }

    /**
     * Tests that {@link WebApkUtils#isInstalled} returns false for an installed but disabled app.
     */
    @Test
    public void testReturnFalseForInstalledButDisabledApp() {
        String packageName = BROWSER_INSTALLED_SUPPORTING_WEBAPKS;
        PackageInfo info = new PackageInfo();
        info.packageName = packageName;
        info.applicationInfo = new ApplicationInfo();
        info.applicationInfo.enabled = false;
        mPackageManager.addPackage(info);

        Assert.assertFalse(WebApkUtils.isInstalled(mContext.getPackageManager(), packageName));
    }

    /**
     * Uninstall a browser. Note: this function only works for uninstalling the non default browser.
     */
    private void uninstallBrowser(String packageName) {
        Intent intent = null;
        try {
            intent = Intent.parseUri("http://", Intent.URI_INTENT_SCHEME);
        } catch (Exception e) {
            Assert.fail();
            return;
        }
        mPackageManager.removeResolveInfosForIntent(intent, packageName);
        mPackageManager.removePackage(packageName);
    }

    private static ResolveInfo newResolveInfo(String packageName) {
        ActivityInfo activityInfo = new ActivityInfo();
        activityInfo.packageName = packageName;
        activityInfo.applicationInfo = new ApplicationInfo();
        activityInfo.applicationInfo.enabled = true;
        ResolveInfo resolveInfo = new ResolveInfo();
        resolveInfo.activityInfo = activityInfo;
        return resolveInfo;
    }

    private void mockInstallBrowsers(String[] browsersToInstall, String defaultBrowser) {
        Intent intent = null;
        try {
            intent = Intent.parseUri("http://", Intent.URI_INTENT_SCHEME);
        } catch (Exception e) {
            Assert.fail();
            return;
        }

        ResolveInfo defaultBrowserInfo = null;
        if (defaultBrowser != null) {
            defaultBrowserInfo = newResolveInfo(defaultBrowser);
            mPackageManager.addResolveInfoForIntent(intent, defaultBrowserInfo);
        }

        for (String name : browsersToInstall) {
            mPackageManager.addResolveInfoForIntent(intent, newResolveInfo(name));
        }
    }

    private void setHostBrowserInSharedPreferences(String hostBrowserPackage) {
        SharedPreferences sharedPref =
                mContext.getSharedPreferences(WebApkConstants.PREF_PACKAGE, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString(WebApkUtils.SHARED_PREF_RUNTIME_HOST, hostBrowserPackage);
        editor.apply();
    }

    private void setHostBrowserInMetadata(String hostBrowserPackage) {
        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, hostBrowserPackage);
        WebApkTestHelper.registerWebApkWithMetaData(WEBAPK_PACKAGE_NAME, bundle);
    }
}
