// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.website;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.support.v7.app.AlertDialog;
import android.text.format.Formatter;
import android.widget.ListAdapter;
import android.widget.ListView;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.notifications.channels.SiteChannelsManager;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferenceUtils;
import org.chromium.components.url_formatter.UrlFormatter;

import java.net.URI;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

/**
 * Shows the permissions and other settings for a particular website.
 */
public class SingleWebsitePreferences extends PreferenceFragment
        implements OnPreferenceChangeListener, OnPreferenceClickListener {
    // SingleWebsitePreferences expects either EXTRA_SITE (a Website) or
    // EXTRA_SITE_ADDRESS (a WebsiteAddress) to be present (but not both). If
    // EXTRA_SITE is present, the fragment will display the permissions in that
    // Website object. If EXTRA_SITE_ADDRESS is present, the fragment will find all
    // permissions for that website address and display those.
    public static final String EXTRA_SITE = "org.chromium.chrome.preferences.site";
    public static final String EXTRA_SITE_ADDRESS = "org.chromium.chrome.preferences.site_address";
    public static final String EXTRA_OBJECT_INFO = "org.chromium.chrome.preferences.object_info";

    // Preference keys, see single_website_preferences.xml
    // Headings:
    public static final String PREF_SITE_TITLE = "site_title";
    public static final String PREF_USAGE = "site_usage";
    public static final String PREF_PERMISSIONS = "site_permissions";
    public static final String PREF_OS_PERMISSIONS_WARNING = "os_permissions_warning";
    public static final String PREF_OS_PERMISSIONS_WARNING_EXTRA = "os_permissions_warning_extra";
    public static final String PREF_OS_PERMISSIONS_WARNING_DIVIDER =
            "os_permissions_warning_divider";
    public static final String PREF_INTRUSIVE_ADS_INFO = "intrusive_ads_info";
    public static final String PREF_INTRUSIVE_ADS_INFO_DIVIDER = "intrusive_ads_info_divider";
    // Actions at the top (if adding new, see hasUsagePreferences below):
    public static final String PREF_CLEAR_DATA = "clear_data";
    // Buttons:
    public static final String PREF_RESET_SITE = "reset_site_button";
    // Website permissions (if adding new, see hasPermissionsPreferences and resetSite below):
    public static final String PREF_ADS_PERMISSION = "ads_permission_list";
    public static final String PREF_AUTOPLAY_PERMISSION = "autoplay_permission_list";
    public static final String PREF_BACKGROUND_SYNC_PERMISSION = "background_sync_permission_list";
    public static final String PREF_CAMERA_CAPTURE_PERMISSION = "camera_permission_list";
    public static final String PREF_CLIPBOARD_PERMISSION = "clipboard_permission_list";
    public static final String PREF_COOKIES_PERMISSION = "cookies_permission_list";
    public static final String PREF_JAVASCRIPT_PERMISSION = "javascript_permission_list";
    public static final String PREF_LOCATION_ACCESS = "location_access_list";
    public static final String PREF_MIC_CAPTURE_PERMISSION = "microphone_permission_list";
    public static final String PREF_MIDI_SYSEX_PERMISSION = "midi_sysex_permission_list";
    public static final String PREF_NOTIFICATIONS_PERMISSION = "push_notifications_list";
    public static final String PREF_POPUP_PERMISSION = "popup_permission_list";
    public static final String PREF_PROTECTED_MEDIA_IDENTIFIER_PERMISSION =
            "protected_media_identifier_permission_list";
    public static final String PREF_SOUND_PERMISSION = "sound_permission_list";

    // All permissions from the permissions preference category must be listed here.
    // TODO(mvanouwerkerk): Use this array in more places to reduce verbosity.
    private static final String[] PERMISSION_PREFERENCE_KEYS = {
            PREF_AUTOPLAY_PERMISSION,
            PREF_BACKGROUND_SYNC_PERMISSION,
            PREF_CAMERA_CAPTURE_PERMISSION,
            PREF_CLIPBOARD_PERMISSION,
            PREF_COOKIES_PERMISSION,
            PREF_JAVASCRIPT_PERMISSION,
            PREF_LOCATION_ACCESS,
            PREF_MIC_CAPTURE_PERMISSION,
            PREF_MIDI_SYSEX_PERMISSION,
            PREF_NOTIFICATIONS_PERMISSION,
            PREF_POPUP_PERMISSION,
            PREF_PROTECTED_MEDIA_IDENTIFIER_PERMISSION,
            PREF_ADS_PERMISSION,
            PREF_SOUND_PERMISSION,
    };

    private static final int REQUEST_CODE_NOTIFICATION_CHANNEL_SETTINGS = 1;

    // The website this page is displaying details about.
    private Website mSite;

    // The number of chosen object permissions displayed.
    private int mObjectPermissionCount;

    private class SingleWebsitePermissionsPopulator
            implements WebsitePermissionsFetcher.WebsitePermissionsCallback {
        private final WebsiteAddress mSiteAddress;

        public SingleWebsitePermissionsPopulator(WebsiteAddress siteAddress) {
            mSiteAddress = siteAddress;
        }

        @Override
        public void onWebsitePermissionsAvailable(Collection<Website> sites) {
            // This method may be called after the activity has been destroyed.
            // In that case, bail out.
            if (getActivity() == null) return;

            // TODO(mvanouwerkerk): Avoid modifying the outer class from this inner class.
            mSite = mergePermissionInfoForTopLevelOrigin(mSiteAddress, sites);

            displaySitePermissions();
        }
    }

    /**
     * Creates a Bundle with the correct arguments for opening this fragment for
     * the website with the given url.
     *
     * @param url The URL to open the fragment with. This is a complete url including scheme,
     *            domain, port,  path, etc.
     * @return The bundle to attach to the preferences intent.
     */
    public static Bundle createFragmentArgsForSite(String url) {
        Bundle fragmentArgs = new Bundle();
        // TODO(mvanouwerkerk): Define a pure getOrigin method in UrlUtilities that is the
        // equivalent of the call below, because this is perfectly fine for non-display purposes.
        String origin =
                UrlFormatter.formatUrlForSecurityDisplay(URI.create(url), true /* showScheme */);
        fragmentArgs.putSerializable(EXTRA_SITE_ADDRESS, WebsiteAddress.create(origin));
        return fragmentArgs;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        getActivity().setTitle(R.string.prefs_site_settings);
        ListView listView = (ListView) getView().findViewById(android.R.id.list);
        listView.setDivider(null);

        Object extraSite = getArguments().getSerializable(EXTRA_SITE);
        Object extraSiteAddress = getArguments().getSerializable(EXTRA_SITE_ADDRESS);

        if (extraSite != null && extraSiteAddress == null) {
            mSite = (Website) extraSite;
            displaySitePermissions();
        } else if (extraSiteAddress != null && extraSite == null) {
            WebsitePermissionsFetcher fetcher;
            fetcher = new WebsitePermissionsFetcher(
                    new SingleWebsitePermissionsPopulator((WebsiteAddress) extraSiteAddress));
            fetcher.fetchAllPreferences();
        } else {
            assert false : "Exactly one of EXTRA_SITE or EXTRA_SITE_ADDRESS must be provided.";
        }

        super.onActivityCreated(savedInstanceState);
    }

    /**
     * Given an address and a list of sets of websites, returns a new site with the same origin
     * as |address| which has merged into it the permissions of the matching input sites. If a
     * permission is found more than once, the one found first is used and the latter are ignored.
     * This should not drop any relevant data as there should not be duplicates like that in the
     * first place.
     *
     * @param address The address to search for.
     * @param websites The websites to search in.
     * @return The merged website.
     */
    private static Website mergePermissionInfoForTopLevelOrigin(
            WebsiteAddress address, Collection<Website> websites) {
        String origin = address.getOrigin();
        String host = Uri.parse(origin).getHost();
        Website merged = new Website(address, null);
        // This loop looks expensive, but the amount of data is likely to be relatively small
        // because most sites have very few permissions.
        for (Website other : websites) {
            if (merged.getAdsException() == null && other.getAdsException() != null
                    && other.compareByAddressTo(merged) == 0) {
                merged.setAdsException(other.getAdsException());
            }
            if (merged.getClipboardInfo() == null && other.getClipboardInfo() != null
                    && permissionInfoIsForTopLevelOrigin(other.getClipboardInfo(), origin)) {
                merged.setClipboardInfo(other.getClipboardInfo());
            }
            if (merged.getGeolocationInfo() == null && other.getGeolocationInfo() != null
                    && permissionInfoIsForTopLevelOrigin(other.getGeolocationInfo(), origin)) {
                merged.setGeolocationInfo(other.getGeolocationInfo());
            }
            if (merged.getMidiInfo() == null && other.getMidiInfo() != null
                    && permissionInfoIsForTopLevelOrigin(other.getMidiInfo(), origin)) {
                merged.setMidiInfo(other.getMidiInfo());
            }
            if (merged.getProtectedMediaIdentifierInfo() == null
                    && other.getProtectedMediaIdentifierInfo() != null
                    && permissionInfoIsForTopLevelOrigin(
                               other.getProtectedMediaIdentifierInfo(), origin)) {
                merged.setProtectedMediaIdentifierInfo(other.getProtectedMediaIdentifierInfo());
            }
            if (merged.getNotificationInfo() == null
                    && other.getNotificationInfo() != null
                    && permissionInfoIsForTopLevelOrigin(
                               other.getNotificationInfo(), origin)) {
                merged.setNotificationInfo(other.getNotificationInfo());
            }
            if (merged.getCameraInfo() == null && other.getCameraInfo() != null
                    && permissionInfoIsForTopLevelOrigin(
                               other.getCameraInfo(), origin)) {
                merged.setCameraInfo(other.getCameraInfo());
            }
            if (merged.getMicrophoneInfo() == null && other.getMicrophoneInfo() != null
                    && permissionInfoIsForTopLevelOrigin(other.getMicrophoneInfo(), origin)) {
                merged.setMicrophoneInfo(other.getMicrophoneInfo());
            }
            if (merged.getLocalStorageInfo() == null
                    && other.getLocalStorageInfo() != null
                    && origin.equals(other.getLocalStorageInfo().getOrigin())) {
                merged.setLocalStorageInfo(other.getLocalStorageInfo());
            }
            for (StorageInfo storageInfo : other.getStorageInfo()) {
                if (host.equals(storageInfo.getHost())) {
                    merged.addStorageInfo(storageInfo);
                }
            }
            for (ChosenObjectInfo objectInfo : other.getChosenObjectInfo()) {
                if (origin.equals(objectInfo.getOrigin())
                        && (objectInfo.getEmbedder() == null
                                   || objectInfo.getEmbedder().equals("*"))) {
                    merged.addChosenObjectInfo(objectInfo);
                }
            }
            if (host.equals(other.getAddress().getHost())) {
                if (merged.getJavaScriptException() == null
                        && other.getJavaScriptException() != null) {
                    merged.setJavaScriptException(other.getJavaScriptException());
                }
                if (merged.getSoundException() == null && other.getSoundException() != null) {
                    merged.setSoundException(other.getSoundException());
                }
                if (merged.getAutoplayException() == null && other.getAutoplayException() != null) {
                    merged.setAutoplayException(other.getAutoplayException());
                }
                if (merged.getBackgroundSyncException() == null
                        && other.getBackgroundSyncException() != null) {
                    merged.setBackgroundSyncException(other.getBackgroundSyncException());
                }
                if (merged.getPopupException() == null && other.getPopupException() != null) {
                    merged.setPopupException(other.getPopupException());
                }
            }

            // TODO(crbug.com/763982): Deal with this TODO colony.
            // TODO(mvanouwerkerk): Make the various info types share a common interface that
            // supports reading the origin or host.
            // TODO(lshang): Merge in CookieException? It will use patterns.
        }
        return merged;
    }

    private static boolean permissionInfoIsForTopLevelOrigin(PermissionInfo info, String origin) {
        // TODO(mvanouwerkerk): Find a more generic place for this method.
        return origin.equals(info.getOrigin())
                && (origin.equals(info.getEmbedderSafe()) || "*".equals(info.getEmbedderSafe()));
    }

    /**
     * Updates the permissions displayed in the UI by fetching them from mSite.
     * Must only be called once mSite is set.
     */
    private void displaySitePermissions() {
        PreferenceUtils.addPreferencesFromResource(this, R.xml.single_website_preferences);

        Set<String> permissionPreferenceKeys =
                new HashSet<>(Arrays.asList(PERMISSION_PREFERENCE_KEYS));
        int maxPermissionOrder = 0;
        PreferenceScreen preferenceScreen = getPreferenceScreen();
        ListAdapter preferences = preferenceScreen.getRootAdapter();
        for (int i = 0; i < preferences.getCount(); ++i) {
            Preference preference = (Preference) preferences.getItem(i);
            setUpPreference(preference);
            // Keep track of the maximum 'order' value of permission preferences, to allow correct
            // positioning of subsequent permission preferences.
            if (permissionPreferenceKeys.contains(preference.getKey())) {
                maxPermissionOrder = Math.max(maxPermissionOrder, preference.getOrder());
            }
        }
        setUpChosenObjectPreferences(maxPermissionOrder);
        setUpOsWarningPreferences();

        setUpAdsInformationalBanner();

        // Remove categories if no sub-items.
        if (!hasUsagePreferences()) {
            Preference heading = preferenceScreen.findPreference(PREF_USAGE);
            preferenceScreen.removePreference(heading);
        }
        if (!hasPermissionsPreferences()) {
            Preference heading = preferenceScreen.findPreference(PREF_PERMISSIONS);
            preferenceScreen.removePreference(heading);
        }
    }

    private void setUpPreference(Preference preference) {
        String key = preference.getKey();
        if (PREF_SITE_TITLE.equals(key)) {
            preference.setTitle(mSite.getTitle());
        } else if (PREF_CLEAR_DATA.equals(key)) {
            setUpClearDataPreference(preference);
        } else if (PREF_RESET_SITE.equals(key)) {
            preference.setOnPreferenceClickListener(this);
        } else if (PREF_ADS_PERMISSION.equals(key)) {
            setUpAdsPreference(preference);
        } else if (PREF_AUTOPLAY_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getAutoplayPermission());
        } else if (PREF_BACKGROUND_SYNC_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getBackgroundSyncPermission());
        } else if (PREF_CAMERA_CAPTURE_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getCameraPermission());
        } else if (PREF_CLIPBOARD_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getClipboardPermission());
        } else if (PREF_COOKIES_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getCookiePermission());
        } else if (PREF_JAVASCRIPT_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getJavaScriptPermission());
        } else if (PREF_LOCATION_ACCESS.equals(key)) {
            setUpLocationPreference(preference);
        } else if (PREF_MIC_CAPTURE_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getMicrophonePermission());
        } else if (PREF_MIDI_SYSEX_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getMidiPermission());
        } else if (PREF_NOTIFICATIONS_PERMISSION.equals(key)) {
            setUpNotificationsPreference(preference);
        } else if (PREF_POPUP_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getPopupPermission());
        } else if (PREF_PROTECTED_MEDIA_IDENTIFIER_PERMISSION.equals(key)) {
            setUpListPreference(preference, mSite.getProtectedMediaIdentifierPermission());
        } else if (PREF_SOUND_PERMISSION.equals(key)) {
            setUpSoundPreference(preference);
        }
    }

    private void setUpClearDataPreference(Preference preference) {
        long usage = mSite.getTotalUsage();
        if (usage > 0) {
            Context context = preference.getContext();
            preference.setTitle(
                    String.format(context.getString(R.string.origin_settings_storage_usage_brief),
                            Formatter.formatShortFileSize(context, usage)));
            ((ClearWebsiteStorage) preference)
                    .setConfirmationListener(new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            clearStoredData();
                        }
                    });
        } else {
            getPreferenceScreen().removePreference(preference);
        }
    }

    private void setUpNotificationsPreference(Preference preference) {
        final ContentSetting value = mSite.getNotificationPermission();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                && ChromeFeatureList.isEnabled(ChromeFeatureList.SITE_NOTIFICATION_CHANNELS)) {
            if (!(value == ContentSetting.ALLOW || value == ContentSetting.BLOCK)) {
                // TODO(crbug.com/735110): Figure out if this is the correct thing to do, for values
                // that are non-null, but not ALLOW or BLOCK either. (In setupListPreference we
                // treat non-ALLOW settings as BLOCK, but here we are simply removing them.)
                getPreferenceScreen().removePreference(preference);
                return;
            }
            // On Android O this preference is read-only, so we replace the existing pref with a
            // regular Preference that takes users to OS settings on click.
            Preference newPreference = new Preference(preference.getContext());
            newPreference.setKey(preference.getKey());
            setUpPreferenceCommon(newPreference);

            if (isPermissionControlledByDSE(
                        ContentSettingsType.CONTENT_SETTINGS_TYPE_NOTIFICATIONS)) {
                newPreference.setSummary(getResources().getString(value == ContentSetting.ALLOW
                                ? R.string.website_settings_permissions_allow_dse
                                : R.string.website_settings_permissions_block_dse));
            } else {
                newPreference.setSummary(
                        getResources().getString(ContentSettingsResources.getSiteSummary(value)));
            }

            newPreference.setDefaultValue(value);

            // This preference is read-only so should not attempt to persist to shared prefs.
            newPreference.setPersistent(false);

            newPreference.setOnPreferenceClickListener(new OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    // There is no guarantee that a channel has been initialized yet for sites
                    // that were granted permission before the channel-initialization-on-grant
                    // code was in place. However, getChannelIdForOrigin will fall back to the
                    // generic Sites channel if no specific channel has been created for the given
                    // origin, so it is safe to open the channel settings for whatever channel ID
                    // it returns.
                    String channelId = SiteChannelsManager.getInstance().getChannelIdForOrigin(
                            mSite.getAddress().getOrigin());
                    launchOsChannelSettings(preference.getContext(), channelId);
                    return true;
                }
            });
            newPreference.setOrder(preference.getOrder());
            getPreferenceScreen().removePreference(preference);
            getPreferenceScreen().addPreference(newPreference);
        } else {
            setUpListPreference(preference, value);
            if (isPermissionControlledByDSE(ContentSettingsType.CONTENT_SETTINGS_TYPE_NOTIFICATIONS)
                    && value != null) {
                updatePreferenceForDSESetting(preference);
            }
        }
    }

    private void launchOsChannelSettings(Context context, String channelId) {
        Intent intent = new Intent(Settings.ACTION_CHANNEL_NOTIFICATION_SETTINGS);
        intent.putExtra(Settings.EXTRA_CHANNEL_ID, channelId);
        intent.putExtra(Settings.EXTRA_APP_PACKAGE, context.getPackageName());
        startActivityForResult(intent, REQUEST_CODE_NOTIFICATION_CHANNEL_SETTINGS);
    }

    /**
     * If we are returning to Site Settings from another activity, the preferences displayed may be
     * out of date. Here we refresh any we suspect may have changed.
     */
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        // The preference screen and mSite may be null if this activity was killed in the
        // background, and the tasks scheduled from onActivityCreated haven't completed yet. Those
        // tasks will take care of reinitializing everything afresh so there is no work to do here.
        if (getPreferenceScreen() == null || mSite == null) {
            return;
        }
        if (requestCode == REQUEST_CODE_NOTIFICATION_CHANNEL_SETTINGS) {
            // User has navigated back from system channel settings on O+. Ensure notification
            // preference is up to date, since they might have toggled it from channel settings.
            Preference notificationsPreference =
                    getPreferenceScreen().findPreference(PREF_NOTIFICATIONS_PERMISSION);
            if (notificationsPreference != null) {
                setUpNotificationsPreference(notificationsPreference);
            }
        }
    }

    private void setUpChosenObjectPreferences(int maxPermissionOrder) {
        for (ChosenObjectInfo info : mSite.getChosenObjectInfo()) {
            Preference preference = new Preference(getActivity());
            preference.getExtras().putSerializable(EXTRA_OBJECT_INFO, info);
            preference.setIcon(ContentSettingsResources.getIcon(info.getContentSettingsType()));
            preference.setOnPreferenceClickListener(this);
            preference.setOrder(maxPermissionOrder);
            preference.setTitle(info.getName());
            preference.setWidgetLayoutResource(R.layout.object_permission);
            getPreferenceScreen().addPreference(preference);
            mObjectPermissionCount++;
        }
    }

    private void setUpOsWarningPreferences() {
        PreferenceScreen preferenceScreen = getPreferenceScreen();
        SiteSettingsCategory categoryWithWarning = getWarningCategory();
        // Remove the 'permission is off in Android' message if not needed.
        if (categoryWithWarning == null) {
            preferenceScreen.removePreference(
                    preferenceScreen.findPreference(PREF_OS_PERMISSIONS_WARNING));
            preferenceScreen.removePreference(
                    preferenceScreen.findPreference(PREF_OS_PERMISSIONS_WARNING_EXTRA));
            preferenceScreen.removePreference(
                    preferenceScreen.findPreference(PREF_OS_PERMISSIONS_WARNING_DIVIDER));
        } else {
            Preference osWarning = preferenceScreen.findPreference(PREF_OS_PERMISSIONS_WARNING);
            Preference osWarningExtra =
                    preferenceScreen.findPreference(PREF_OS_PERMISSIONS_WARNING_EXTRA);
            categoryWithWarning.configurePermissionIsOffPreferences(
                    osWarning, osWarningExtra, getActivity(), false);
            if (osWarning.getTitle() == null) {
                preferenceScreen.removePreference(
                        preferenceScreen.findPreference(PREF_OS_PERMISSIONS_WARNING));
            } else if (osWarningExtra.getTitle() == null) {
                preferenceScreen.removePreference(
                        preferenceScreen.findPreference(PREF_OS_PERMISSIONS_WARNING_EXTRA));
            }
        }
    }

    private void setUpAdsInformationalBanner() {
        // Add the informational banner which shows at the top of the UI if ad blocking is
        // activated on this site.
        PreferenceScreen preferenceScreen = getPreferenceScreen();
        boolean adBlockingActivated = SiteSettingsCategory.adsCategoryEnabled()
                && WebsitePreferenceBridge.getAdBlockingActivated(mSite.getAddress().getOrigin())
                && preferenceScreen.findPreference(PREF_ADS_PERMISSION) != null;

        if (!adBlockingActivated) {
            Preference intrusiveAdsInfo = preferenceScreen.findPreference(PREF_INTRUSIVE_ADS_INFO);
            Preference intrusiveAdsInfoDivider =
                    preferenceScreen.findPreference(PREF_INTRUSIVE_ADS_INFO_DIVIDER);

            preferenceScreen.removePreference(intrusiveAdsInfo);
            preferenceScreen.removePreference(intrusiveAdsInfoDivider);
        }
    }

    private SiteSettingsCategory getWarningCategory() {
        // If more than one per-app permission is disabled in Android, we can pick any category to
        // show the warning, because they will all show the same warning and all take the user to
        // the user to the same location. It is preferrable, however, that we give Geolocation some
        // priority because that category is the only one that potentially shows an additional
        // warning (when Location is turned off globally).
        if (showWarningFor(ContentSettingsType.CONTENT_SETTINGS_TYPE_GEOLOCATION)) {
            return SiteSettingsCategory.fromContentSettingsType(
                    ContentSettingsType.CONTENT_SETTINGS_TYPE_GEOLOCATION);
        }
        if (showWarningFor(ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA)) {
            return SiteSettingsCategory.fromContentSettingsType(
                    ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA);
        }
        if (showWarningFor(ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC)) {
            return SiteSettingsCategory.fromContentSettingsType(
                    ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC);
        }
        return null;
    }

    private boolean showWarningFor(int type) {
        ContentSetting setting = null;
        if (type == ContentSettingsType.CONTENT_SETTINGS_TYPE_GEOLOCATION) {
            setting = mSite.getGeolocationPermission();
        } else if (type == ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA) {
            setting = mSite.getCameraPermission();
        } else if (type == ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC) {
            setting = mSite.getMicrophonePermission();
        }

        if (setting == null) {
            return false;
        }

        SiteSettingsCategory category = SiteSettingsCategory.fromContentSettingsType(type);
        return category.showPermissionBlockedMessage(getActivity());
    }

    private boolean hasUsagePreferences() {
        // New actions under the Usage preference category must be listed here so that the category
        // heading can be removed when no actions are shown.
        return getPreferenceScreen().findPreference(PREF_CLEAR_DATA) != null;
    }

    private boolean hasPermissionsPreferences() {
        if (mObjectPermissionCount > 0) return true;
        PreferenceScreen screen = getPreferenceScreen();
        for (String key : PERMISSION_PREFERENCE_KEYS) {
            if (screen.findPreference(key) != null) return true;
        }
        return false;
    }

    /**
     * Initialize a ListPreference with a certain value.
     * @param preference The ListPreference to initialize.
     * @param value The value to initialize it to.
     */
    private void setUpListPreference(Preference preference, ContentSetting value) {
        if (value == null) {
            getPreferenceScreen().removePreference(preference);
            return;
        }
        setUpPreferenceCommon(preference);
        ListPreference listPreference = (ListPreference) preference;

        CharSequence[] keys = new String[2];
        CharSequence[] descriptions = new String[2];
        keys[0] = ContentSetting.ALLOW.toString();
        keys[1] = ContentSetting.BLOCK.toString();
        descriptions[0] = getResources().getString(
                ContentSettingsResources.getSiteSummary(ContentSetting.ALLOW));
        descriptions[1] = getResources().getString(
                ContentSettingsResources.getSiteSummary(ContentSetting.BLOCK));
        listPreference.setEntryValues(keys);
        listPreference.setEntries(descriptions);
        // TODO(crbug.com/735110): Figure out if this is the correct thing to do - here we are
        // effectively treating non-ALLOW values as BLOCK.
        int index = (value == ContentSetting.ALLOW ? 0 : 1);
        listPreference.setValueIndex(index);
        listPreference.setOnPreferenceChangeListener(this);
        listPreference.setSummary("%s");
    }

    /**
     * Sets some properties that apply to both regular Preferences and ListPreferences, i.e.
     * preference title, enabled-state, and icon, based on the preference's key.
     */
    private void setUpPreferenceCommon(Preference preference) {
        int contentType = getContentSettingsTypeFromPreferenceKey(preference.getKey());
        int explanationResourceId = ContentSettingsResources.getExplanation(contentType);
        if (explanationResourceId != 0) {
            preference.setTitle(explanationResourceId);
        }
        if (preference.isEnabled()) {
            SiteSettingsCategory category =
                    SiteSettingsCategory.fromContentSettingsType(contentType);
            if (category != null && !category.enabledInAndroid(getActivity())) {
                preference.setIcon(category.getDisabledInAndroidIcon(getActivity()));
                preference.setEnabled(false);
            } else {
                preference.setIcon(
                        ContentSettingsResources.getTintedIcon(contentType, getResources()));
            }
        } else {
            preference.setIcon(
                    ContentSettingsResources.getDisabledIcon(contentType, getResources()));
        }
    }

    private void setUpLocationPreference(Preference preference) {
        ContentSetting permission = mSite.getGeolocationPermission();
        setUpListPreference(preference, permission);
        if (isPermissionControlledByDSE(ContentSettingsType.CONTENT_SETTINGS_TYPE_GEOLOCATION)
                && permission != null) {
            updatePreferenceForDSESetting(preference);
        }
    }

    private void setUpSoundPreference(Preference preference) {
        ContentSetting currentValue = mSite.getSoundPermission();
        // In order to always show the sound permission, set it up with the default value if it
        // doesn't have a current value.
        if (currentValue == null) {
            currentValue = PrefServiceBridge.getInstance().isSoundEnabled() ? ContentSetting.ALLOW
                                                                            : ContentSetting.BLOCK;
        }
        setUpListPreference(preference, currentValue);
    }

    /**
     * Updates the ads list preference based on whether the site is a candidate for blocking. This
     * has some custom behavior.
     * 1. If the site is a candidate and has activation, the permission should show up even if it
     *    is set as the default (e.g. |preference| is null).
     * 2. The BLOCK string is custom.
     */
    private void setUpAdsPreference(Preference preference) {
        // Do not show the setting if the category is not enabled.
        if (!SiteSettingsCategory.adsCategoryEnabled()) {
            setUpListPreference(preference, null);
            return;
        }
        // If the ad blocker is activated, then this site will have ads blocked unless there is an
        // explicit permission disallowing the blocking.
        boolean activated =
                WebsitePreferenceBridge.getAdBlockingActivated(mSite.getAddress().getOrigin());
        ContentSetting permission = mSite.getAdsPermission();

        // If |permission| is null, there is no explicit (non-default) permission set for this site.
        // If the site is not considered a candidate for blocking, do the standard thing and remove
        // the preference.
        if (permission == null && !activated) {
            setUpListPreference(preference, null);
            return;
        }

        // However, if the blocking is activated, we still want to show the permission, even if it
        // is in the default state.
        if (permission == null) {
            ContentSetting defaultPermission = PrefServiceBridge.getInstance().adsEnabled()
                    ? ContentSetting.ALLOW
                    : ContentSetting.BLOCK;
            permission = defaultPermission;
        }
        setUpListPreference(preference, permission);

        // The subresource filter permission has a custom BLOCK string.
        ListPreference listPreference = (ListPreference) preference;
        Resources res = getResources();
        listPreference.setEntries(
                new String[] {res.getString(R.string.website_settings_permissions_allow),
                        res.getString(R.string.website_settings_permissions_ads_block)});
        listPreference.setValueIndex(permission == ContentSetting.ALLOW ? 0 : 1);
    }

    /**
     * Returns true if the DSE (default search engine) geolocation and notifications permissions
     * are configured for the DSE.
     */
    private boolean isPermissionControlledByDSE(@ContentSettingsType int contentSettingsType) {
        return WebsitePreferenceBridge.isPermissionControlledByDSE(
                contentSettingsType, mSite.getAddress().getOrigin(), false);
    }

    /**
     * Updates the location preference to indicate that the site has access to location (via X-Geo)
     * for searches that happen from the omnibox.
     * @param preference The Location preference to modify.
     */
    private void updatePreferenceForDSESetting(Preference preference) {
        ListPreference listPreference = (ListPreference) preference;
        Resources res = getResources();
        listPreference.setEntries(new String[] {
                res.getString(R.string.website_settings_permissions_allow_dse),
                res.getString(R.string.website_settings_permissions_block_dse),
        });
    }

    private int getContentSettingsTypeFromPreferenceKey(String preferenceKey) {
        switch (preferenceKey) {
            case PREF_ADS_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_ADS;
            case PREF_AUTOPLAY_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_AUTOPLAY;
            case PREF_BACKGROUND_SYNC_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_BACKGROUND_SYNC;
            case PREF_CAMERA_CAPTURE_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA;
            case PREF_CLIPBOARD_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_CLIPBOARD_READ;
            case PREF_COOKIES_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_COOKIES;
            case PREF_JAVASCRIPT_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_JAVASCRIPT;
            case PREF_LOCATION_ACCESS:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_GEOLOCATION;
            case PREF_MIC_CAPTURE_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC;
            case PREF_MIDI_SYSEX_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_MIDI_SYSEX;
            case PREF_NOTIFICATIONS_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_NOTIFICATIONS;
            case PREF_POPUP_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_POPUPS;
            case PREF_PROTECTED_MEDIA_IDENTIFIER_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER;
            case PREF_SOUND_PERMISSION:
                return ContentSettingsType.CONTENT_SETTINGS_TYPE_SOUND;
            default:
                return 0;
        }
    }

    private void clearStoredData() {
        mSite.clearAllStoredData(
                new Website.StoredDataClearedCallback() {
                    @Override
                    public void onStoredDataCleared() {
                        PreferenceScreen preferenceScreen = getPreferenceScreen();
                        preferenceScreen.removePreference(
                                preferenceScreen.findPreference(PREF_CLEAR_DATA));
                        if (!hasUsagePreferences()) {
                            preferenceScreen.removePreference(
                                    preferenceScreen.findPreference(PREF_USAGE));
                        }
                        popBackIfNoSettings();
                    }
                });
    }

    private void popBackIfNoSettings() {
        if (!hasPermissionsPreferences() && !hasUsagePreferences()) {
            if (getActivity() != null) {
                getActivity().finish();
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        ContentSetting permission = ContentSetting.fromString((String) newValue);
        if (PREF_ADS_PERMISSION.equals(preference.getKey())) {
            mSite.setAdsPermission(permission);
        } else if (PREF_AUTOPLAY_PERMISSION.equals(preference.getKey())) {
            mSite.setAutoplayPermission(permission);
        } else if (PREF_BACKGROUND_SYNC_PERMISSION.equals(preference.getKey())) {
            mSite.setBackgroundSyncPermission(permission);
        } else if (PREF_CAMERA_CAPTURE_PERMISSION.equals(preference.getKey())) {
            mSite.setCameraPermission(permission);
        } else if (PREF_CLIPBOARD_PERMISSION.equals(preference.getKey())) {
            mSite.setClipboardPermission(permission);
        } else if (PREF_COOKIES_PERMISSION.equals(preference.getKey())) {
            mSite.setCookiePermission(permission);
        } else if (PREF_JAVASCRIPT_PERMISSION.equals(preference.getKey())) {
            mSite.setJavaScriptPermission(permission);
        } else if (PREF_LOCATION_ACCESS.equals(preference.getKey())) {
            mSite.setGeolocationPermission(permission);
        } else if (PREF_MIC_CAPTURE_PERMISSION.equals(preference.getKey())) {
            mSite.setMicrophonePermission(permission);
        } else if (PREF_MIDI_SYSEX_PERMISSION.equals(preference.getKey())) {
            mSite.setMidiPermission(permission);
        } else if (PREF_NOTIFICATIONS_PERMISSION.equals(preference.getKey())) {
            mSite.setNotificationPermission(permission);
        } else if (PREF_POPUP_PERMISSION.equals(preference.getKey())) {
            mSite.setPopupPermission(permission);
        } else if (PREF_PROTECTED_MEDIA_IDENTIFIER_PERMISSION.equals(preference.getKey())) {
            mSite.setProtectedMediaIdentifierPermission(permission);
        } else if (PREF_SOUND_PERMISSION.equals(preference.getKey())) {
            mSite.setSoundPermission(permission);
        }

        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        Bundle extras = preference.peekExtras();
        if (extras != null) {
            ChosenObjectInfo objectInfo =
                    (ChosenObjectInfo) extras.getSerializable(EXTRA_OBJECT_INFO);
            if (objectInfo != null) {
                objectInfo.revoke();

                PreferenceScreen preferenceScreen = getPreferenceScreen();
                preferenceScreen.removePreference(preference);
                mObjectPermissionCount--;
                if (!hasPermissionsPreferences()) {
                    Preference heading = preferenceScreen.findPreference(PREF_PERMISSIONS);
                    preferenceScreen.removePreference(heading);
                }
                return true;
            }
        }

        // Handle the Clear & Reset preference click by showing a confirmation.
        new AlertDialog.Builder(getActivity(), R.style.AlertDialogTheme)
                .setTitle(R.string.website_reset)
                .setMessage(R.string.website_reset_confirmation)
                .setPositiveButton(R.string.website_reset, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        resetSite();
                    }
                })
                .setNegativeButton(R.string.cancel, null)
                .show();
        return true;
    }

    /**
     * Resets the current site, clearing all permissions and storage used (inc. cookies).
     */
    @VisibleForTesting
    protected void resetSite() {
        if (getActivity() == null) return;
        // Clear the screen.
        // TODO(mvanouwerkerk): Refactor this class so that it does not depend on the screen state
        // for its logic. This class should maintain its own data model, and only update the screen
        // after a change is made.
        PreferenceScreen screen = getPreferenceScreen();
        for (String key : PERMISSION_PREFERENCE_KEYS) {
            Preference preference = screen.findPreference(key);
            if (preference != null) screen.removePreference(preference);
        }

        String origin = mSite.getAddress().getOrigin();
        WebsitePreferenceBridge.nativeClearCookieData(origin);
        WebsitePreferenceBridge.nativeClearBannerData(origin);

        // Clear the permissions.
        mSite.setAdsPermission(ContentSetting.DEFAULT);
        mSite.setAutoplayPermission(ContentSetting.DEFAULT);
        mSite.setBackgroundSyncPermission(ContentSetting.DEFAULT);
        mSite.setCameraPermission(ContentSetting.DEFAULT);
        mSite.setClipboardPermission(ContentSetting.DEFAULT);
        mSite.setCookiePermission(ContentSetting.DEFAULT);
        mSite.setGeolocationPermission(ContentSetting.DEFAULT);
        mSite.setJavaScriptPermission(ContentSetting.DEFAULT);
        mSite.setMicrophonePermission(ContentSetting.DEFAULT);
        mSite.setMidiPermission(ContentSetting.DEFAULT);
        mSite.setNotificationPermission(ContentSetting.DEFAULT);
        mSite.setPopupPermission(ContentSetting.DEFAULT);
        mSite.setProtectedMediaIdentifierPermission(ContentSetting.DEFAULT);
        mSite.setSoundPermission(ContentSetting.DEFAULT);

        for (ChosenObjectInfo info : mSite.getChosenObjectInfo()) info.revoke();
        mObjectPermissionCount = 0;

        // Clear the storage and finish the activity if necessary.
        if (mSite.getTotalUsage() > 0) {
            clearStoredData();
        } else {
            // Clearing stored data implies popping back to parent menu if there
            // is nothing left to show. Therefore, we only need to explicitly
            // close the activity if there's no stored data to begin with.
            getActivity().finish();
        }
    }
}
