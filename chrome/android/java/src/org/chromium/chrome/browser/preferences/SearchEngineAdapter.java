// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.IntDef;
import android.support.annotation.StringRes;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.AccessibilityDelegate;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.BaseAdapter;
import android.widget.RadioButton;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.preferences.website.ContentSetting;
import org.chromium.chrome.browser.preferences.website.GeolocationInfo;
import org.chromium.chrome.browser.preferences.website.NotificationInfo;
import org.chromium.chrome.browser.preferences.website.SingleWebsitePreferences;
import org.chromium.chrome.browser.preferences.website.WebsitePreferenceBridge;
import org.chromium.chrome.browser.search_engines.TemplateUrl;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.components.location.LocationUtils;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
* A custom adapter for listing search engines.
*/
public class SearchEngineAdapter extends BaseAdapter
        implements TemplateUrlService.LoadListener, TemplateUrlService.TemplateUrlServiceObserver,
                OnClickListener {
    private static final String TAG = "cr_SearchEngines";

    private static final int VIEW_TYPE_ITEM = 0;
    private static final int VIEW_TYPE_DIVIDER = 1;
    private static final int VIEW_TYPE_COUNT = 2;

    public static final int MAX_RECENT_ENGINE_NUM = 3;
    public static final long MAX_DISPLAY_TIME_SPAN_MS = TimeUnit.DAYS.toMillis(2);

    /**
     * Type for source of search engine. This is needed because if a custom search engine is set as
     * default, it will be moved to the prepopulated list.
     */
    @IntDef({TYPE_DEFAULT, TYPE_PREPOPULATED, TYPE_RECENT})
    @Retention(RetentionPolicy.SOURCE)
    public @interface TemplateUrlSourceType {}
    public static final int TYPE_DEFAULT = 0;
    public static final int TYPE_PREPOPULATED = 1;
    public static final int TYPE_RECENT = 2;

    /** The current context. */
    private Context mContext;

    /** The layout inflater to use for the custom views. */
    private LayoutInflater mLayoutInflater;

    /** The list of prepopluated and default search engines. */
    private List<TemplateUrl> mPrepopulatedSearchEngines = new ArrayList<>();

    /** The list of recently visited search engines. */
    private List<TemplateUrl> mRecentSearchEngines = new ArrayList<>();

    /**
     * The position (index into mPrepopulatedSearchEngines) of the currently selected search engine.
     * Can be -1 if current search engine is managed and set to something other than the
     * pre-populated values.
     */
    private int mSelectedSearchEnginePosition = -1;

    /** The position of the default search engine before user's action. */
    private int mInitialEnginePosition = -1;

    private boolean mHasLoadObserver;

    private boolean mIsLocationPermissionChanged;

    /**
     * Construct a SearchEngineAdapter.
     * @param context The current context.
     */
    public SearchEngineAdapter(Context context) {
        mContext = context;
        mLayoutInflater = (LayoutInflater) mContext.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
    }

    /**
     * Start the adapter to gather the available search engines and listen for updates.
     */
    public void start() {
        refreshData();
        TemplateUrlService.getInstance().addObserver(this);
    }

    /**
     * Stop the adapter from listening for future search engine updates.
     */
    public void stop() {
        if (mHasLoadObserver) {
            TemplateUrlService.getInstance().unregisterLoadListener(this);
            mHasLoadObserver = false;
        }
        TemplateUrlService.getInstance().removeObserver(this);
    }

    @VisibleForTesting
    String getValueForTesting() {
        return Integer.toString(mSelectedSearchEnginePosition);
    }

    @VisibleForTesting
    String setValueForTesting(String value) {
        return searchEngineSelected(Integer.parseInt(value));
    }

    @VisibleForTesting
    String getKeywordForTesting(int index) {
        return toKeyword(index);
    }

    /**
     * Initialize the search engine list.
     */
    private void refreshData() {
        TemplateUrlService templateUrlService = TemplateUrlService.getInstance();
        if (!templateUrlService.isLoaded()) {
            mHasLoadObserver = true;
            templateUrlService.registerLoadListener(this);
            templateUrlService.load();
            return;  // Flow continues in onTemplateUrlServiceLoaded below.
        }

        List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();
        TemplateUrl defaultSearchEngineTemplateUrl =
                templateUrlService.getDefaultSearchEngineTemplateUrl();
        sortAndFilterUnnecessaryTemplateUrl(templateUrls, defaultSearchEngineTemplateUrl);
        boolean forceRefresh = mIsLocationPermissionChanged;
        mIsLocationPermissionChanged = false;
        if (!didSearchEnginesChange(templateUrls)) {
            if (forceRefresh) notifyDataSetChanged();
            return;
        }

        mPrepopulatedSearchEngines = new ArrayList<>();
        mRecentSearchEngines = new ArrayList<>();

        for (int i = 0; i < templateUrls.size(); i++) {
            TemplateUrl templateUrl = templateUrls.get(i);
            if (getSearchEngineSourceType(templateUrl, defaultSearchEngineTemplateUrl)
                    == TYPE_RECENT) {
                mRecentSearchEngines.add(templateUrl);
            } else {
                mPrepopulatedSearchEngines.add(templateUrl);
            }
        }

        // Convert the TemplateUrl index into an index of mSearchEngines.
        mSelectedSearchEnginePosition = -1;
        for (int i = 0; i < mPrepopulatedSearchEngines.size(); ++i) {
            if (mPrepopulatedSearchEngines.get(i).equals(defaultSearchEngineTemplateUrl)) {
                mSelectedSearchEnginePosition = i;
            }
        }

        for (int i = 0; i < mRecentSearchEngines.size(); ++i) {
            if (mRecentSearchEngines.get(i).equals(defaultSearchEngineTemplateUrl)) {
                // Add one to offset the title for the recent search engine list.
                mSelectedSearchEnginePosition = i + computeStartIndexForRecentSearchEngines();
            }
        }

        if (mSelectedSearchEnginePosition == -1) {
            throw new IllegalStateException(
                    "Default search engine index did not match any available search engines.");
        }

        mInitialEnginePosition = mSelectedSearchEnginePosition;

        notifyDataSetChanged();
    }

    public static void sortAndFilterUnnecessaryTemplateUrl(
            List<TemplateUrl> templateUrls, TemplateUrl defaultSearchEngine) {
        Collections.sort(templateUrls, new Comparator<TemplateUrl>() {
            @Override
            public int compare(TemplateUrl templateUrl1, TemplateUrl templateUrl2) {
                if (templateUrl1.getIsPrepopulated() && templateUrl2.getIsPrepopulated()) {
                    return templateUrl1.getPrepopulatedId() - templateUrl2.getPrepopulatedId();
                } else if (templateUrl1.getIsPrepopulated()) {
                    return -1;
                } else if (templateUrl2.getIsPrepopulated()) {
                    return 1;
                } else if (templateUrl1.equals(templateUrl2)) {
                    return 0;
                } else if (templateUrl1.equals(defaultSearchEngine)) {
                    return -1;
                } else if (templateUrl2.equals(defaultSearchEngine)) {
                    return 1;
                } else {
                    return ApiCompatibilityUtils.compareLong(
                            templateUrl2.getLastVisitedTime(), templateUrl1.getLastVisitedTime());
                }
            }
        });
        int recentEngineNum = 0;
        long displayTime = System.currentTimeMillis() - MAX_DISPLAY_TIME_SPAN_MS;
        Iterator<TemplateUrl> iterator = templateUrls.iterator();
        while (iterator.hasNext()) {
            TemplateUrl templateUrl = iterator.next();
            if (getSearchEngineSourceType(templateUrl, defaultSearchEngine) != TYPE_RECENT) {
                continue;
            }
            if (recentEngineNum < MAX_RECENT_ENGINE_NUM
                    && templateUrl.getLastVisitedTime() > displayTime) {
                recentEngineNum++;
            } else {
                iterator.remove();
            }
        }
    }

    private static @TemplateUrlSourceType int getSearchEngineSourceType(
            TemplateUrl templateUrl, TemplateUrl defaultSearchEngine) {
        if (templateUrl.getIsPrepopulated()) {
            return TYPE_PREPOPULATED;
        } else if (templateUrl.equals(defaultSearchEngine)) {
            return TYPE_DEFAULT;
        } else {
            return TYPE_RECENT;
        }
    }

    private static boolean containsTemplateUrl(
            List<TemplateUrl> templateUrls, TemplateUrl targetTemplateUrl) {
        for (int i = 0; i < templateUrls.size(); i++) {
            TemplateUrl templateUrl = templateUrls.get(i);
            // Explicitly excluding TemplateUrlSourceType and Index as they might change if a search
            // engine is set as default.
            if (templateUrl.getIsPrepopulated() == targetTemplateUrl.getIsPrepopulated()
                    && TextUtils.equals(templateUrl.getKeyword(), targetTemplateUrl.getKeyword())
                    && TextUtils.equals(
                               templateUrl.getShortName(), targetTemplateUrl.getShortName())) {
                return true;
            }
        }
        return false;
    }

    private boolean didSearchEnginesChange(List<TemplateUrl> templateUrls) {
        if (templateUrls.size()
                != mPrepopulatedSearchEngines.size() + mRecentSearchEngines.size()) {
            return true;
        }
        for (int i = 0; i < templateUrls.size(); i++) {
            TemplateUrl templateUrl = templateUrls.get(i);
            if (!containsTemplateUrl(mPrepopulatedSearchEngines, templateUrl)
                    && !SearchEngineAdapter.containsTemplateUrl(
                               mRecentSearchEngines, templateUrl)) {
                return true;
            }
        }
        return false;
    }

    private String toKeyword(int position) {
        if (position < mPrepopulatedSearchEngines.size()) {
            return mPrepopulatedSearchEngines.get(position).getKeyword();
        } else {
            position -= computeStartIndexForRecentSearchEngines();
            return mRecentSearchEngines.get(position).getKeyword();
        }
    }

    // BaseAdapter:

    @Override
    public int getCount() {
        int size = 0;
        if (mPrepopulatedSearchEngines != null) {
            size += mPrepopulatedSearchEngines.size();
        }
        if (mRecentSearchEngines != null && mRecentSearchEngines.size() != 0) {
            // Account for the header by adding one to the size.
            size += mRecentSearchEngines.size() + 1;
        }
        return size;
    }

    @Override
    public int getViewTypeCount() {
        return VIEW_TYPE_COUNT;
    }

    @Override
    public Object getItem(int pos) {
        if (pos < mPrepopulatedSearchEngines.size()) {
            return mPrepopulatedSearchEngines.get(pos);
        } else if (pos > mPrepopulatedSearchEngines.size()) {
            pos -= computeStartIndexForRecentSearchEngines();
            return mRecentSearchEngines.get(pos);
        }
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public int getItemViewType(int position) {
        if (position == mPrepopulatedSearchEngines.size() && mRecentSearchEngines.size() != 0) {
            return VIEW_TYPE_DIVIDER;
        } else {
            return VIEW_TYPE_ITEM;
        }
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = convertView;
        int itemViewType = getItemViewType(position);
        if (convertView == null) {
            view = mLayoutInflater.inflate(
                    itemViewType == VIEW_TYPE_DIVIDER && mRecentSearchEngines.size() != 0
                            ? R.layout.search_engine_recent_title
                            : R.layout.search_engine,
                    null);
        }
        if (itemViewType == VIEW_TYPE_DIVIDER) {
            return view;
        }

        view.setOnClickListener(this);
        view.setTag(position);

        // TODO(finnur): There's a tinting bug in the AppCompat lib (see http://crbug.com/474695),
        // which causes the first radiobox to always appear selected, even if it is not. It is being
        // addressed, but in the meantime we should use the native RadioButton instead.
        RadioButton radioButton = (RadioButton) view.findViewById(R.id.radiobutton);
        // On Lollipop this removes the redundant animation ring on selection but on older versions
        // it would cause the radio button to disappear.
        // TODO(finnur): Remove the encompassing if statement once we go back to using the AppCompat
        // control.
        final boolean selected = position == mSelectedSearchEnginePosition;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            radioButton.setBackgroundResource(0);
        }
        radioButton.setChecked(selected);

        TextView description = (TextView) view.findViewById(R.id.name);
        Resources resources = mContext.getResources();

        TemplateUrl templateUrl = (TemplateUrl) getItem(position);
        description.setText(templateUrl.getShortName());

        TextView url = (TextView) view.findViewById(R.id.url);
        url.setText(templateUrl.getKeyword());
        if (TextUtils.isEmpty(templateUrl.getKeyword())) {
            url.setVisibility(View.GONE);
        }

        // To improve the explore-by-touch experience, the radio button is hidden from accessibility
        // and instead, "checked" or "not checked" is read along with the search engine's name, e.g.
        // "google.com checked" or "google.com not checked".
        radioButton.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        description.setAccessibilityDelegate(new AccessibilityDelegate() {
            @Override
            public void onInitializeAccessibilityEvent(View host, AccessibilityEvent event) {
                super.onInitializeAccessibilityEvent(host, event);
                event.setChecked(selected);
            }

            @Override
            public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {
                super.onInitializeAccessibilityNodeInfo(host, info);
                info.setCheckable(true);
                info.setChecked(selected);
            }
        });

        TextView link = (TextView) view.findViewById(R.id.location_permission);
        link.setVisibility(View.GONE);
        if (TemplateUrlService.getInstance().getSearchEngineUrlFromTemplateUrl(
                templateUrl.getKeyword()) == null) {
            Log.e(TAG, "Invalid template URL found: %s", templateUrl);
            assert false;
        } else if (selected) {
            setupPermissionsLink(templateUrl, link);
        }

        return view;
    }

    // TemplateUrlService.LoadListener

    @Override
    public void onTemplateUrlServiceLoaded() {
        TemplateUrlService.getInstance().unregisterLoadListener(this);
        mHasLoadObserver = false;
        refreshData();
    }

    @Override
    public void onTemplateURLServiceChanged() {
        refreshData();
    }

    // OnClickListener:

    @Override
    public void onClick(View view) {
        if (view.getTag() == null) {
            onPermissionsLinkClicked();
        } else {
            searchEngineSelected((int) view.getTag());
        }
    }

    private String searchEngineSelected(int position) {
        // Record the change in search engine.
        mSelectedSearchEnginePosition = position;

        String keyword = toKeyword(mSelectedSearchEnginePosition);
        TemplateUrlService.getInstance().setSearchEngine(keyword);

        // If the user has manually set the default search engine, disable auto switching.
        boolean manualSwitch = mSelectedSearchEnginePosition != mInitialEnginePosition;
        if (manualSwitch) {
            RecordUserAction.record("SearchEngine_ManualChange");
            LocaleManager.getInstance().setSearchEngineAutoSwitch(false);
        }
        notifyDataSetChanged();
        return keyword;
    }

    private void onPermissionsLinkClicked() {
        mIsLocationPermissionChanged = true;
        String url = TemplateUrlService.getInstance().getSearchEngineUrlFromTemplateUrl(
                toKeyword(mSelectedSearchEnginePosition));
        int linkBeingShown = getPermissionsLinkMessage(url);
        assert linkBeingShown != 0;
        // If notifications are off and location is on but system location is off, it's a special
        // case where we link directly to Android Settings.
        if (linkBeingShown == R.string.search_engine_system_location_disabled) {
            mContext.startActivity(LocationUtils.getInstance().getSystemLocationSettingsIntent());
        } else {
            Intent settingsIntent = PreferencesLauncher.createIntentForSettingsPage(
                    mContext, SingleWebsitePreferences.class.getName());
            Bundle fragmentArgs = SingleWebsitePreferences.createFragmentArgsForSite(url);
            settingsIntent.putExtra(Preferences.EXTRA_SHOW_FRAGMENT_ARGUMENTS, fragmentArgs);
            mContext.startActivity(settingsIntent);
        }
    }

    private String getSearchEngineUrl(TemplateUrl templateUrl) {
        if (templateUrl == null) {
            Log.e(TAG, "Invalid null template URL found");
            assert false;
            return "";
        }

        String url = TemplateUrlService.getInstance().getSearchEngineUrlFromTemplateUrl(
                templateUrl.getKeyword());
        if (url == null) {
            Log.e(TAG, "Invalid template URL found: %s", templateUrl);
            assert false;
            return "";
        }

        return url;
    }

    @StringRes
    private int getPermissionsLinkMessage(String url) {
        if (url == null) return 0;

        NotificationInfo notificationSettings = new NotificationInfo(url, null, false);
        boolean notificationsAllowed =
                notificationSettings.getContentSetting() == ContentSetting.ALLOW
                && WebsitePreferenceBridge.isPermissionControlledByDSE(
                           ContentSettingsType.CONTENT_SETTINGS_TYPE_NOTIFICATIONS, url, false);

        GeolocationInfo locationSettings = new GeolocationInfo(url, null, false);
        boolean locationAllowed = locationSettings.getContentSetting() == ContentSetting.ALLOW
                && WebsitePreferenceBridge.isPermissionControlledByDSE(
                           ContentSettingsType.CONTENT_SETTINGS_TYPE_GEOLOCATION, url, false);

        boolean systemLocationAllowed =
                LocationUtils.getInstance().isSystemLocationSettingEnabled();

        // Cases where location is fully enabled.
        if (locationAllowed && systemLocationAllowed) {
            if (notificationsAllowed) {
                return R.string.search_engine_location_and_notifications_allowed;
            } else {
                return R.string.search_engine_location_allowed;
            }
        }

        // Cases where the user has allowed location for the site but it's disabled at the system
        // level.
        if (locationAllowed) {
            if (notificationsAllowed) {
                return R.string.search_engine_notifications_allowed_system_location_disabled;
            } else {
                return R.string.search_engine_system_location_disabled;
            }
        }

        // Cases where the user has not allowed location.
        if (notificationsAllowed) return R.string.search_engine_notifications_allowed;

        return 0;
    }

    private void setupPermissionsLink(TemplateUrl templateUrl, TextView link) {
        int message = getPermissionsLinkMessage(getSearchEngineUrl(templateUrl));
        if (message == 0) return;

        ForegroundColorSpan linkSpan = new ForegroundColorSpan(
                ApiCompatibilityUtils.getColor(mContext.getResources(), R.color.google_blue_700));
        link.setVisibility(View.VISIBLE);
        link.setOnClickListener(this);

        // If notifications are off and location is on but system location is off, it's a special
        // case where we link directly to Android Settings. So we only highlight that part of the
        // link to make it clearer.
        if (message == R.string.search_engine_system_location_disabled) {
            link.setText(SpanApplier.applySpans(
                    mContext.getString(message), new SpanInfo("<link>", "</link>", linkSpan)));
        } else {
            SpannableString messageWithLink = new SpannableString(mContext.getString(message));
            messageWithLink.setSpan(linkSpan, 0, messageWithLink.length(), 0);
            link.setText(messageWithLink);
        }
    }

    private boolean locationEnabled(TemplateUrl templateUrl) {
        String url = getSearchEngineUrl(templateUrl);
        if (url.isEmpty()) return false;

        GeolocationInfo locationSettings = new GeolocationInfo(url, null, false);
        ContentSetting locationPermission = locationSettings.getContentSetting();
        return locationPermission == ContentSetting.ALLOW;
    }

    private int computeStartIndexForRecentSearchEngines() {
        // If there are custom search engines to show, add 1 for showing the  "Recently visited"
        // header.
        if (mRecentSearchEngines.size() > 0) {
            return mPrepopulatedSearchEngines.size() + 1;
        } else {
            return mPrepopulatedSearchEngines.size();
        }
    }
}
