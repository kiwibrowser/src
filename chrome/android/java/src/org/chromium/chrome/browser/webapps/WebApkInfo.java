// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.text.TextUtils;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.blink_public.platform.WebDisplayMode;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.ShortcutSource;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.content_public.common.ScreenOrientationValues;
import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;

import java.util.HashMap;
import java.util.Map;

/**
 * Stores info for WebAPK.
 */
public class WebApkInfo extends WebappInfo {
    public static final String RESOURCE_NAME = "name";
    public static final String RESOURCE_SHORT_NAME = "short_name";
    public static final String RESOURCE_STRING_TYPE = "string";

    private static final String TAG = "WebApkInfo";

    private Icon mBadgeIcon;
    private String mApkPackageName;
    private int mShellApkVersion;
    private String mManifestUrl;
    private String mManifestStartUrl;
    private Map<String, String> mIconUrlToMurmur2HashMap;

    public static WebApkInfo createEmpty() {
        return new WebApkInfo();
    }

    /**
     * Constructs a WebApkInfo from the passed in Intent and <meta-data> in the WebAPK's Android
     * manifest.
     * @param intent Intent containing info about the app.
     */
    public static WebApkInfo create(Intent intent) {
        String webApkPackageName =
                IntentUtils.safeGetStringExtra(intent, WebApkConstants.EXTRA_WEBAPK_PACKAGE_NAME);
        if (TextUtils.isEmpty(webApkPackageName)) {
            return null;
        }

        String url = urlFromIntent(intent);
        int source = sourceFromIntent(intent);

        if (source == ShortcutSource.EXTERNAL_INTENT) {
            if (IntentHandler.determineExternalIntentSource(intent)
                    == IntentHandler.ExternalAppId.CHROME) {
                source = ShortcutSource.EXTERNAL_INTENT_FROM_CHROME;
            }
        }

        // Force navigation if the extra is not specified to avoid breaking deep linking for old
        // WebAPKs which don't specify the {@link ShortcutHelper#EXTRA_FORCE_NAVIGATION} intent
        // extra.
        boolean forceNavigation = IntentUtils.safeGetBooleanExtra(
                intent, ShortcutHelper.EXTRA_FORCE_NAVIGATION, true);

        return create(webApkPackageName, url, source, forceNavigation);
    }

    /**
     * Constructs a WebApkInfo from the passed in parameters and <meta-data> in the WebAPK's Android
     * manifest.
     *
     * @param webApkPackageName The package name of the WebAPK.
     * @param url Url that the WebAPK should navigate to when launched.
     * @param source Source that the WebAPK was launched from.
     * @param forceNavigation Whether the WebAPK should navigate to {@link url} if it is already
     *     running.
     */
    public static WebApkInfo create(
            String webApkPackageName, String url, int source, boolean forceNavigation) {
        // Unlike non-WebAPK web apps, WebAPK ids are predictable. A malicious actor may send an
        // intent with a valid start URL and arbitrary other data. Only use the start URL, the
        // package name and the ShortcutSource from the launch intent and extract the remaining data
        // from the <meta-data> in the WebAPK's Android manifest.

        Bundle bundle = extractWebApkMetaData(webApkPackageName);
        if (bundle == null) {
            return null;
        }

        Resources res = null;
        try {
            res = ContextUtils.getApplicationContext()
                          .getPackageManager()
                          .getResourcesForApplication(webApkPackageName);
        } catch (PackageManager.NameNotFoundException e) {
            return null;
        }

        int nameId = res.getIdentifier(RESOURCE_NAME, RESOURCE_STRING_TYPE, webApkPackageName);
        int shortNameId =
                res.getIdentifier(RESOURCE_SHORT_NAME, RESOURCE_STRING_TYPE, webApkPackageName);
        String name = nameId != 0 ? res.getString(nameId)
                                  : IntentUtils.safeGetString(bundle, WebApkMetaDataKeys.NAME);
        String shortName = shortNameId != 0
                ? res.getString(shortNameId)
                : IntentUtils.safeGetString(bundle, WebApkMetaDataKeys.SHORT_NAME);

        String scope = IntentUtils.safeGetString(bundle, WebApkMetaDataKeys.SCOPE);

        @WebDisplayMode
        int displayMode = displayModeFromString(
                IntentUtils.safeGetString(bundle, WebApkMetaDataKeys.DISPLAY_MODE));
        int orientation = orientationFromString(
                IntentUtils.safeGetString(bundle, WebApkMetaDataKeys.ORIENTATION));
        long themeColor = getLongFromMetaData(bundle, WebApkMetaDataKeys.THEME_COLOR,
                ShortcutHelper.MANIFEST_COLOR_INVALID_OR_MISSING);
        long backgroundColor = getLongFromMetaData(bundle, WebApkMetaDataKeys.BACKGROUND_COLOR,
                ShortcutHelper.MANIFEST_COLOR_INVALID_OR_MISSING);

        int shellApkVersion =
                IntentUtils.safeGetInt(bundle, WebApkMetaDataKeys.SHELL_APK_VERSION, 0);

        String manifestUrl = IntentUtils.safeGetString(bundle, WebApkMetaDataKeys.WEB_MANIFEST_URL);
        String manifestStartUrl = IntentUtils.safeGetString(bundle, WebApkMetaDataKeys.START_URL);
        Map<String, String> iconUrlToMurmur2HashMap = getIconUrlAndIconMurmur2HashMap(bundle);

        int primaryIconId = IntentUtils.safeGetInt(bundle, WebApkMetaDataKeys.ICON_ID, 0);
        Bitmap primaryIcon = decodeImageResource(res, primaryIconId);

        int badgeIconId = IntentUtils.safeGetInt(bundle, WebApkMetaDataKeys.BADGE_ICON_ID, 0);
        Bitmap badgeIcon = decodeImageResource(res, badgeIconId);

        return create(WebApkConstants.WEBAPK_ID_PREFIX + webApkPackageName, url, scope,
                new Icon(primaryIcon), new Icon(badgeIcon), name, shortName, displayMode,
                orientation, source, themeColor, backgroundColor, webApkPackageName,
                shellApkVersion, manifestUrl, manifestStartUrl, iconUrlToMurmur2HashMap,
                forceNavigation);
    }

    /**
     * Construct a {@link WebApkInfo} instance.
     *
     * @param id                      ID for the WebAPK.
     * @param url                     URL that the WebAPK should navigate to when launched.
     * @param scope                   Scope for the WebAPK.
     * @param primaryIcon             Primary icon to show for the WebAPK.
     * @param badgeIcon               Badge icon to use for notifications.
     * @param name                    Name of the WebAPK.
     * @param shortName               The short name of the WebAPK.
     * @param displayMode             Display mode of the WebAPK.
     * @param orientation             Orientation of the WebAPK.
     * @param source                  Source that the WebAPK was launched from.
     * @param themeColor              The theme color of the WebAPK.
     * @param backgroundColor         The background color of the WebAPK.
     * @param webApkPackageName       The package of the WebAPK.
     * @param shellApkVersion         Version of the code in //chrome/android/webapk/shell_apk.
     * @param manifestUrl             URL of the Web Manifest.
     * @param manifestStartUrl        URL that the WebAPK should navigate to when launched from the
     *                                homescreen. Different from the {@link url} parameter if the
     *                                WebAPK is launched from a deep link.
     * @param iconUrlToMurmur2HashMap Map of the WebAPK's icon URLs to Murmur2 hashes of the
     *                                icon untransformed bytes.
     * @param forceNavigation         Whether the WebAPK should navigate to {@link url} if the
     *                                WebAPK is already open.
     */
    public static WebApkInfo create(String id, String url, String scope, Icon primaryIcon,
            Icon badgeIcon, String name, String shortName, @WebDisplayMode int displayMode,
            int orientation, int source, long themeColor, long backgroundColor,
            String webApkPackageName, int shellApkVersion, String manifestUrl,
            String manifestStartUrl, Map<String, String> iconUrlToMurmur2HashMap,
            boolean forceNavigation) {
        if (id == null || url == null || manifestStartUrl == null || webApkPackageName == null) {
            Log.e(TAG,
                    "Incomplete data provided: " + id + ", " + url + ", " + manifestStartUrl + ", "
                            + webApkPackageName);
            return null;
        }

        // The default scope should be computed from the Web Manifest start URL. If the WebAPK was
        // launched from a deep link {@link startUrl} may be different from the Web Manifest start
        // URL.
        if (TextUtils.isEmpty(scope)) {
            scope = ShortcutHelper.getScopeFromUrl(manifestStartUrl);
        }

        return new WebApkInfo(id, url, scope, primaryIcon, badgeIcon, name, shortName, displayMode,
                orientation, source, themeColor, backgroundColor, webApkPackageName,
                shellApkVersion, manifestUrl, manifestStartUrl, iconUrlToMurmur2HashMap,
                forceNavigation);
    }

    protected WebApkInfo(String id, String url, String scope, Icon primaryIcon, Icon badgeIcon,
            String name, String shortName, @WebDisplayMode int displayMode, int orientation,
            int source, long themeColor, long backgroundColor, String webApkPackageName,
            int shellApkVersion, String manifestUrl, String manifestStartUrl,
            Map<String, String> iconUrlToMurmur2HashMap, boolean forceNavigation) {
        super(id, url, scope, primaryIcon, name, shortName, displayMode, orientation, source,
                themeColor, backgroundColor, null /* splash_screen_url */,
                false /* isIconGenerated */, forceNavigation);
        mBadgeIcon = badgeIcon;
        mApkPackageName = webApkPackageName;
        mShellApkVersion = shellApkVersion;
        mManifestUrl = manifestUrl;
        mManifestStartUrl = manifestStartUrl;
        mIconUrlToMurmur2HashMap = iconUrlToMurmur2HashMap;
    }

    protected WebApkInfo() {}

    /**
     * Returns the badge icon in Bitmap form.
     */
    public Bitmap badgeIcon() {
        return (mBadgeIcon == null) ? null : mBadgeIcon.decoded();
    }

    @Override
    public String apkPackageName() {
        return mApkPackageName;
    }

    public int shellApkVersion() {
        return mShellApkVersion;
    }

    public String manifestUrl() {
        return mManifestUrl;
    }

    public String manifestStartUrl() {
        return mManifestStartUrl;
    }

    public Map<String, String> iconUrlToMurmur2HashMap() {
        return mIconUrlToMurmur2HashMap;
    }

    @Override
    public void setWebappIntentExtras(Intent intent) {
        // For launching a {@link WebApkActivity}.
        intent.putExtra(ShortcutHelper.EXTRA_ID, id());
        intent.putExtra(ShortcutHelper.EXTRA_URL, uri().toString());
        intent.putExtra(ShortcutHelper.EXTRA_SOURCE, source());
        intent.putExtra(WebApkConstants.EXTRA_WEBAPK_PACKAGE_NAME, apkPackageName());
        intent.putExtra(ShortcutHelper.EXTRA_FORCE_NAVIGATION, shouldForceNavigation());
    }

    /**
     * Extracts meta data from a WebAPK's Android Manifest.
     * @param webApkPackageName WebAPK's package name.
     * @return Bundle with the extracted meta data.
     */
    private static Bundle extractWebApkMetaData(String webApkPackageName) {
        PackageManager packageManager = ContextUtils.getApplicationContext().getPackageManager();
        try {
            ApplicationInfo appInfo = packageManager.getApplicationInfo(
                    webApkPackageName, PackageManager.GET_META_DATA);
            return appInfo.metaData;
        } catch (PackageManager.NameNotFoundException e) {
            return null;
        }
    }

    /**
     * Decodes bitmap from WebAPK's resources.
     */
    private static Bitmap decodeImageResource(Resources webApkResources, int resourceId) {
        return BitmapFactory.decodeResource(webApkResources, resourceId);
    }

    /**
     * Extracts long value from the WebAPK's meta data.
     * @param metaData WebAPK meta data to extract the long from.
     * @param name Name of the <meta-data> tag to extract the value from.
     * @param defaultValue Value to return if long value could not be extracted.
     * @return long value.
     */
    private static long getLongFromMetaData(Bundle metaData, String name, long defaultValue) {
        String value = metaData.getString(name);

        // The value should be terminated with 'L' to force the value to be a string. According to
        // https://developer.android.com/guide/topics/manifest/meta-data-element.html numeric
        // meta data values can only be retrieved via {@link Bundle#getInt()} and
        // {@link Bundle#getFloat()}. We cannot use {@link Bundle#getFloat()} due to loss of
        // precision.
        if (value == null || !value.endsWith("L")) {
            return defaultValue;
        }
        try {
            return Long.parseLong(value.substring(0, value.length() - 1));
        } catch (NumberFormatException e) {
        }
        return defaultValue;
    }

    /**
     * Extract the icon URLs and icon hashes from the WebAPK's meta data, and returns a map of these
     * {URL, hash} pairs. The icon URLs/icon hashes are stored in a single meta data tag in the
     * WebAPK's AndroidManifest.xml as following:
     * "URL1 hash1 URL2 hash2 URL3 hash3..."
     */
    private static Map<String, String> getIconUrlAndIconMurmur2HashMap(Bundle metaData) {
        Map<String, String> iconUrlAndIconMurmur2HashMap = new HashMap<String, String>();
        String iconUrlsAndIconMurmur2Hashes = metaData.getString(
                WebApkMetaDataKeys.ICON_URLS_AND_ICON_MURMUR2_HASHES);
        if (TextUtils.isEmpty(iconUrlsAndIconMurmur2Hashes)) return iconUrlAndIconMurmur2HashMap;

        // Parse the metadata tag which contains "URL1 hash1 URL2 hash2 URL3 hash3..." pairs and
        // create a hash map.
        // TODO(hanxi): crbug.com/666349. Add a test to verify that the icon URLs in WebAPKs'
        // AndroidManifest.xml don't contain space.
        String[] urlsAndHashes = iconUrlsAndIconMurmur2Hashes.split("[ ]+");
        if (urlsAndHashes.length % 2 != 0) {
            Log.e(TAG, "The icon URLs and icon murmur2 hashes don't come in pairs.");
            return iconUrlAndIconMurmur2HashMap;
        }
        for (int i = 0; i < urlsAndHashes.length; i += 2) {
            iconUrlAndIconMurmur2HashMap.put(urlsAndHashes[i], urlsAndHashes[i + 1]);
        }
        return iconUrlAndIconMurmur2HashMap;
    }

    /**
     * Returns the WebDisplayMode which matches {@link displayMode}.
     * @param displayMode One of https://www.w3.org/TR/appmanifest/#dfn-display-modes-values
     * @return The matching WebDisplayMode. {@link WebDisplayMode#Undefined} if there is no match.
     */
    private static @WebDisplayMode int displayModeFromString(String displayMode) {
        if (displayMode == null) {
            return WebDisplayMode.UNDEFINED;
        }

        if (displayMode.equals("fullscreen")) {
            return WebDisplayMode.FULLSCREEN;
        } else if (displayMode.equals("standalone")) {
            return WebDisplayMode.STANDALONE;
        } else if (displayMode.equals("minimal-ui")) {
            return WebDisplayMode.MINIMAL_UI;
        } else if (displayMode.equals("browser")) {
            return WebDisplayMode.BROWSER;
        } else {
            return WebDisplayMode.UNDEFINED;
        }
    }

    /**
     * Returns the ScreenOrientationValue which matches {@link orientation}.
     * @param orientation One of https://w3c.github.io/screen-orientation/#orientationlocktype-enum
     * @return The matching ScreenOrientationValue. {@link ScreenOrientationValues#DEFAULT} if there
     * is no match.
     */
    private static int orientationFromString(String orientation) {
        if (orientation == null) {
            return ScreenOrientationValues.DEFAULT;
        }

        if (orientation.equals("any")) {
            return ScreenOrientationValues.ANY;
        } else if (orientation.equals("natural")) {
            return ScreenOrientationValues.NATURAL;
        } else if (orientation.equals("landscape")) {
            return ScreenOrientationValues.LANDSCAPE;
        } else if (orientation.equals("landscape-primary")) {
            return ScreenOrientationValues.LANDSCAPE_PRIMARY;
        } else if (orientation.equals("landscape-secondary")) {
            return ScreenOrientationValues.LANDSCAPE_SECONDARY;
        } else if (orientation.equals("portrait")) {
            return ScreenOrientationValues.PORTRAIT;
        } else if (orientation.equals("portrait-primary")) {
            return ScreenOrientationValues.PORTRAIT_PRIMARY;
        } else if (orientation.equals("portrait-secondary")) {
            return ScreenOrientationValues.PORTRAIT_SECONDARY;
        } else {
            return ScreenOrientationValues.DEFAULT;
        }
    }
}
