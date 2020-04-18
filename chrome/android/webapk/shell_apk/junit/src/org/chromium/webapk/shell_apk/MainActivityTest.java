// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplication;

import org.chromium.testing.local.LocalRobolectricTestRunner;
import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;
import org.chromium.webapk.test.WebApkTestHelper;

/** Unit tests for {@link MainActivity}.
 *
 * Note: In real word, |loggedIntentUrlParam| is set to be nonempty iff intent url is outside of the
 * scope specified in the Android manifest, so in the test we always have these two conditions
 * together.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE, packageName = WebApkUtilsTest.WEBAPK_PACKAGE_NAME)
public final class MainActivityTest {
    /**
     * Test that MainActivity uses the manifest start URL and appends the intent url as a paramater,
     * if intent URL scheme does not match the scope url.
     */
    @Test
    public void testIntentUrlOutOfScopeBecauseOfScheme() {
        final String intentStartUrl = "http://www.google.com/search_results?q=eh#cr=countryCA";
        final String manifestStartUrl = "https://www.google.com/index.html";
        final String manifestScope = "https://www.google.com/";
        final String expectedStartUrl =
                "https://www.google.com/index.html?originalUrl=http%3A%2F%2Fwww.google.com%2Fsearch_results%3Fq%3Deh%23cr%3DcountryCA";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        bundle.putString(WebApkMetaDataKeys.LOGGED_INTENT_URL_PARAM, "originalUrl");
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class, launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(
                HostBrowserLauncher.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(
                expectedStartUrl, startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    /**
     * Test that MainActivity uses the manifest start URL and appends the intent url as a paramater,
     * if intent URL path is outside of the scope specified in the Android Manifest.
     */
    @Test
    public void testIntentUrlOutOfScopeBecauseOfPath() {
        final String intentStartUrl = "https://www.google.com/maps/";
        final String manifestStartUrl = "https://www.google.com/maps/contrib/startUrl";
        final String manifestScope = "https://www.google.com/maps/contrib/";
        final String expectedStartUrl =
                "https://www.google.com/maps/contrib/startUrl?originalUrl=https%3A%2F%2Fwww.google.com%2Fmaps%2F";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        bundle.putString(WebApkMetaDataKeys.LOGGED_INTENT_URL_PARAM, "originalUrl");
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class, launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(
                HostBrowserLauncher.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(
                expectedStartUrl, startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    /**
     * Tests that the intent URL is rewritten if |LoggedIntentUrlParam| is set, even though the
     * intent URL is inside the scope specified in the Android Manifest.
     */
    @Test
    public void testRewriteStartUrlInsideScope() {
        final String intentStartUrl = "https://www.google.com/maps/address?A=a";
        final String manifestStartUrl = "https://www.google.com/maps/startUrl";
        final String manifestScope = "https://www.google.com/maps";
        final String expectedStartUrl =
                "https://www.google.com/maps/startUrl?originalUrl=https%3A%2F%2Fwww.google.com%2Fmaps%2Faddress%3FA%3Da";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        bundle.putString(WebApkMetaDataKeys.LOGGED_INTENT_URL_PARAM, "originalUrl");
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class, launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(
                HostBrowserLauncher.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(
                expectedStartUrl, startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    /**
     * Tests that the intent URL is not rewritten again if the query parameter to append is part of
     * the intent URL when |LoggedIntentUrlParam| is set.
     */
    @Test
    public void testNotRewriteStartUrlWhenContainsTheQueryParameterToAppend() {
        final String intentStartUrl =
                "https://www.google.com/maps/startUrl?originalUrl=https%3A%2F%2Fwww.google.com%2Fmaps%2Faddress%3FA%3Da";
        final String manifestStartUrl = "https://www.google.com/maps/startUrl";
        final String manifestScope = "https://www.google.com/maps";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, manifestScope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        bundle.putString(WebApkMetaDataKeys.LOGGED_INTENT_URL_PARAM, "originalUrl");
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class, launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(
                HostBrowserLauncher.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(
                intentStartUrl, startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    /**
     * Test that MainActivity uses the manifest start URL and appends the intent url as a paramater,
     * if intent URL host includes unicode characters, and the host name is different from the scope
     * url host specified in the Android Manifest. In particular, MainActivity should not escape
     * unicode characters.
     */
    @Test
    public void testRewriteUnicodeHost() {
        final String intentStartUrl = "https://www.google.com/";
        final String manifestStartUrl = "https://www.☺.com/";
        final String scope = "https://www.☺.com/";
        final String expectedStartUrl =
                "https://www.☺.com/?originalUrl=https%3A%2F%2Fwww.google.com%2F";
        final String browserPackageName = "com.android.chrome";

        Bundle bundle = new Bundle();
        bundle.putString(WebApkMetaDataKeys.START_URL, manifestStartUrl);
        bundle.putString(WebApkMetaDataKeys.SCOPE, scope);
        bundle.putString(WebApkMetaDataKeys.RUNTIME_HOST, browserPackageName);
        bundle.putString(WebApkMetaDataKeys.LOGGED_INTENT_URL_PARAM, "originalUrl");
        WebApkTestHelper.registerWebApkWithMetaData(WebApkUtilsTest.WEBAPK_PACKAGE_NAME, bundle);

        installBrowser(browserPackageName);

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(intentStartUrl));
        Robolectric.buildActivity(MainActivity.class, launchIntent).create();

        Intent startActivityIntent = ShadowApplication.getInstance().getNextStartedActivity();
        Assert.assertEquals(
                HostBrowserLauncher.ACTION_START_WEBAPK, startActivityIntent.getAction());
        Assert.assertEquals(
                expectedStartUrl, startActivityIntent.getStringExtra(WebApkConstants.EXTRA_URL));
    }

    private void installBrowser(String browserPackageName) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://"));
        Shadows.shadowOf(RuntimeEnvironment.application.getPackageManager())
                .addResolveInfoForIntent(intent, newResolveInfo(browserPackageName));
        Shadows.shadowOf(RuntimeEnvironment.application.getPackageManager())
                .addPackage(newPackageInfo(browserPackageName));
    }

    private static ResolveInfo newResolveInfo(String packageName) {
        ActivityInfo activityInfo = new ActivityInfo();
        activityInfo.packageName = packageName;
        ResolveInfo resolveInfo = new ResolveInfo();
        resolveInfo.activityInfo = activityInfo;
        return resolveInfo;
    }

    private static PackageInfo newPackageInfo(String packageName) {
        PackageInfo packageInfo = new PackageInfo();
        packageInfo.packageName = packageName;
        packageInfo.versionName = "Developer Build";
        packageInfo.applicationInfo = new ApplicationInfo();
        packageInfo.applicationInfo.enabled = true;
        return packageInfo;
    }
}
