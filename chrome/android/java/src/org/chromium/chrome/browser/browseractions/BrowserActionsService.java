// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.app.Activity;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.text.TextUtils;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.notifications.ChromeNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.NotificationConstants;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorImpl;
import org.chromium.chrome.browser.tabmodel.TabPersistentStore.TabModelSelectorMetadata;
import org.chromium.chrome.browser.tabmodel.TabPersistentStore.TabPersistentStoreObserver;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.common.Referrer;
import org.chromium.ui.widget.Toast;

import java.lang.ref.WeakReference;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * The foreground service responsible for creating notifications for Browser Actions and keep the
 * process alive during tab creation.
 */
public class BrowserActionsService extends Service {
    public static final String ACTION_TAB_CREATION_START =
            "org.chromium.chrome.browser.browseractions.ACTION_TAB_CREATION_START";
    public static final String ACTION_TAB_CREATION_CHROME_DISPLAYED =
            "org.chromium.chrome.browser.browseractions.ACTION_TAB_CREATION_CHROME_DISPLAYED";
    public static final String EXTRA_TAB_ID = "org.chromium.chrome.browser.browseractions.TAB_ID";
    public static final String EXTRA_LINK_URL =
            "org.chromium.chrome.browser.browseractions.LINK_URL";
    public static final String EXTRA_SOURCE_PACKAGE_NAME =
            "org.chromium.chrome.browser.browseractions.SOURCE_PACKAGE_NAME";

    public static final String PREF_HAS_BROWSER_ACTIONS_NOTIFICATION =
            "org.chromium.chrome.browser.browseractions.HAS_BROWSER_ACTIONS_NOTIFICATION";

    public static final String PREF_NUM_TAB_CREATED_IN_BACKGROUND =
            "org.chromium.chrome.browser.browseractions.NUM_TAB_CREATED_IN_BACKGROUND";

    /**
     * Extra that indicates whether to show a Tab for single url or the tab switcher for
     * multiple urls.
     */
    public static final String EXTRA_IS_SINGLE_URL =
            "org.chromium.chrome.browser.browseractions.is_single_url";

    private static Intent sNotificationIntent;

    private static int sTitleResId;

    private static Set<Integer> sPendingTabIds = new HashSet<Integer>();
    private static int sPendingCreatedUrlNum;

    private BrowserActionsTabModelSelector mBrowserActionsSelector;
    private TabModelSelectorImpl mTabbedModeTabModelSelector;
    private TabPersistentStoreObserver mObserver;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @VisibleForTesting
    static Intent getNotificationIntent() {
        return sNotificationIntent;
    }

    @VisibleForTesting
    static int getTitleResId() {
        return sTitleResId;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (TextUtils.equals(intent.getAction(), ACTION_TAB_CREATION_START)) {
            createNotification(false, Tab.INVALID_TAB_ID);

            String url = IntentUtils.safeGetStringExtra(intent, EXTRA_LINK_URL);
            String sourcePackageName =
                    IntentUtils.safeGetStringExtra(intent, EXTRA_SOURCE_PACKAGE_NAME);
            openTabInBackground(url, sourcePackageName);
            Toast.makeText(this, R.string.browser_actions_open_in_background_toast_message,
                         Toast.LENGTH_SHORT)
                    .show();
            updateNumTabCreatedInBackground();
            NotificationUmaTracker.getInstance().onNotificationShown(
                    NotificationUmaTracker.BROWSER_ACTIONS, ChannelDefinitions.CHANNEL_ID_BROWSER);
        } else if (TextUtils.equals(intent.getAction(), ACTION_TAB_CREATION_CHROME_DISPLAYED)) {
            clearPendingStatus();
            removeObserver();
            stopForeground(true);
        }
        // The service will not be restarted if Chrome get killed.
        return START_NOT_STICKY;
    }

    private static void clearPendingStatus() {
        sPendingTabIds.clear();
        sPendingCreatedUrlNum = 0;
    }

    private void createNotification(boolean isUpdate, int tabId) {
        sendBrowserActionsNotification(isUpdate, tabId);
        if (isUpdate) {
            ContextUtils.getAppSharedPreferences()
                    .edit()
                    .putBoolean(PREF_HAS_BROWSER_ACTIONS_NOTIFICATION, true)
                    .apply();
        }
    }

    private void backgroundServiceIfNecessary() {
        if (sPendingTabIds.isEmpty() && sPendingCreatedUrlNum == 0) {
            stopForeground(false);
            removeObserver();
        }
    }

    private void removeObserver() {
        if (mObserver == null) return;
        if (mBrowserActionsSelector != null) {
            mBrowserActionsSelector.removeTabPersistentStoreObserver(mObserver);
        }
        if (mTabbedModeTabModelSelector != null) {
            mTabbedModeTabModelSelector.removeTabPersistentStoreObserver(mObserver);
        }
    }

    @VisibleForTesting
    static boolean isBackgroundService() {
        return sPendingTabIds.isEmpty() && sPendingCreatedUrlNum == 0;
    }

    private void openTabInBackground(String linkUrl, String sourcePackageName) {
        Referrer referrer = IntentHandler.constructValidReferrerForAuthority(sourcePackageName);
        LoadUrlParams loadUrlParams = new LoadUrlParams(linkUrl);
        loadUrlParams.setReferrer(referrer);
        Tab tab = launchTabInRunningTabbedActivity(loadUrlParams);
        if (tab != null) {
            int tabId = tab.getId();
            assert tabId != Tab.INVALID_TAB_ID;
            sPendingTabIds.add(tabId);
            createNotification(true, tabId);
            return;
        }
        launchTabInBrowserActionsModel(loadUrlParams);
    }

    private Tab launchTabInRunningTabbedActivity(LoadUrlParams loadUrlParams) {
        for (WeakReference<Activity> ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref.get() instanceof ChromeTabbedActivity)) continue;

            ChromeTabbedActivity activity = (ChromeTabbedActivity) ref.get();
            if (activity == null) continue;
            if (activity.getTabModelSelector() != null) {
                mTabbedModeTabModelSelector = (TabModelSelectorImpl) activity.getTabModelSelector();
                mTabbedModeTabModelSelector.addTabPersistentStoreObserver(
                        getTabPersistentStoreObserver());
                Tab tab = mTabbedModeTabModelSelector.openNewTab(
                        loadUrlParams, TabLaunchType.FROM_BROWSER_ACTIONS, null, false);
                assert tab != null;
                return tab;
            }
        }
        return null;
    }

    private TabPersistentStoreObserver getTabPersistentStoreObserver() {
        if (mObserver == null) {
            mObserver = new TabPersistentStoreObserver() {
                @Override
                public void onMetadataSavedAsynchronously(
                        TabModelSelectorMetadata modelSelectorMetadata) {
                    removeSavedTabs(modelSelectorMetadata.normalModelMetadata.ids);
                    backgroundServiceIfNecessary();
                }
            };
        }
        return mObserver;
    }

    private void launchTabInBrowserActionsModel(LoadUrlParams loadUrlParams) {
        mBrowserActionsSelector = BrowserActionsTabModelSelector.getInstance();
        mBrowserActionsSelector.addTabPersistentStoreObserver(getTabPersistentStoreObserver());
        Callback<Integer> tabCreatedCallback = new Callback<Integer>() {
            @Override
            public void onResult(Integer tabId) {
                // Service has been changed to background by opening Chrome so nothing needs to
                // update.
                if (sPendingCreatedUrlNum == 0) return;
                sPendingCreatedUrlNum--;
                if (tabId != Tab.INVALID_TAB_ID) sPendingTabIds.add(tabId);
                backgroundServiceIfNecessary();
                createNotification(true, Tab.INVALID_TAB_ID);
            }
        };
        sPendingCreatedUrlNum++;
        mBrowserActionsSelector.openNewTab(loadUrlParams, tabCreatedCallback);
    }

    private void removeSavedTabs(List<Integer> savedTabIds) {
        for (Integer tabId : savedTabIds) sPendingTabIds.remove(tabId);
    }

    private void sendBrowserActionsNotification(boolean isUpdate, int tabId) {
        ChromeNotificationBuilder builder = createNotificationBuilder(isUpdate, tabId);
        startForeground(NotificationConstants.NOTIFICATION_ID_BROWSER_ACTIONS, builder.build());
    }

    private ChromeNotificationBuilder createNotificationBuilder(boolean isUpdate, int tabId) {
        ChromeNotificationBuilder builder =
                NotificationBuilderFactory
                        .createChromeNotificationBuilder(
                                true /* preferCompat */, ChannelDefinitions.CHANNEL_ID_BROWSER)
                        .setSmallIcon(R.drawable.infobar_chrome)
                        .setLocalOnly(true)
                        .setAutoCancel(true)
                        .setContentText(this.getString(R.string.browser_actions_notification_text));
        sTitleResId = getNotificationTitleId(isUpdate);
        builder.setContentTitle(this.getString(sTitleResId));
        sNotificationIntent = buildNotificationIntent(isUpdate, tabId);
        PendingIntent notifyPendingIntent = PendingIntent.getActivity(
                this, 0, sNotificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);
        builder.setContentIntent(notifyPendingIntent);
        return builder;
    }

    private int getNotificationTitleId(boolean isUpdate) {
        if (isUpdate) {
            assert sTitleResId != 0;
        } else if (hasBrowserActionsNotification()) {
            sTitleResId = R.string.browser_actions_multi_links_open_notification_title;
        } else {
            sTitleResId = R.string.browser_actions_single_link_open_notification_title;
        }
        return sTitleResId;
    }

    /**
     * TODO(ltian:) fix ChromeTabbedActivity.processUrlViewIntent to handle Intent after browser
     * actions tab merging and use Tab.createBringTabToFrontIntent to create Intent for single url
     * in case of browser actions tab merging.
     */
    private Intent buildNotificationIntent(boolean isUpdate, int tabId) {
        boolean multipleUrls = hasBrowserActionsNotification();
        Intent intent;
        if (!multipleUrls && tabId != Tab.INVALID_TAB_ID) {
            intent = Tab.createBringTabToFrontIntent(tabId);
        } else {
            intent = new Intent(this, ChromeLauncherActivity.class);
            IntentHandler.addTrustedIntentExtras(intent);
        }
        if (isUpdate) {
            intent.putExtra(EXTRA_IS_SINGLE_URL, !multipleUrls);
        }
        return intent;
    }

    @VisibleForTesting
    static boolean hasBrowserActionsNotification() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_HAS_BROWSER_ACTIONS_NOTIFICATION, false);
    }

    private static void updateNumTabCreatedInBackground() {
        int tabNum =
                ContextUtils.getAppSharedPreferences().getInt(PREF_NUM_TAB_CREATED_IN_BACKGROUND, 0)
                + 1;
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putInt(PREF_NUM_TAB_CREATED_IN_BACKGROUND, tabNum)
                .apply();
    }

    /**
     * Called when Chrome tabbed mode come to the foreground.
     */
    public static void onTabbedModeForegrounded() {
        // If Chrome is shown, force the foreground service to be killed so notification bound to it
        // will be dismissed.
        if (!sPendingTabIds.isEmpty() || sPendingCreatedUrlNum > 0) {
            Context context = ContextUtils.getApplicationContext();
            Intent intent = new Intent(context, BrowserActionsService.class);
            intent.setAction(ACTION_TAB_CREATION_CHROME_DISPLAYED);
            context.startService(intent);
        } else {
            NotificationManager notificationManager =
                    (NotificationManager) ContextUtils.getApplicationContext().getSystemService(
                            Context.NOTIFICATION_SERVICE);
            notificationManager.cancel(NotificationConstants.NOTIFICATION_ID_BROWSER_ACTIONS);
        }

        ContextUtils.getAppSharedPreferences()
                .edit()
                .putBoolean(PREF_HAS_BROWSER_ACTIONS_NOTIFICATION, false)
                .apply();
        int tabNum = ContextUtils.getAppSharedPreferences().getInt(
                PREF_NUM_TAB_CREATED_IN_BACKGROUND, 0);
        if (tabNum != 0) {
            RecordHistogram.recordCountHistogram(
                    "BrowserActions.NumTabCreatedInBackground", tabNum);
            ContextUtils.getAppSharedPreferences()
                    .edit()
                    .remove(PREF_NUM_TAB_CREATED_IN_BACKGROUND)
                    .apply();
        }
    }

    /**
     * Checks whether Chrome should display tab switcher via Browser Actions Intent.
     * @param intent The intent to open the Chrome.
     * @param isOverviewVisible Whether tab switcher is shown.
     */
    public static boolean shouldToggleOverview(Intent intent, boolean isOverviewVisible) {
        if (!IntentHandler.wasIntentSenderChrome(intent)) return false;
        if (!IntentUtils.safeHasExtra(intent, EXTRA_IS_SINGLE_URL)) return false;
        boolean isSingleUrl = IntentUtils.safeGetBooleanExtra(intent, EXTRA_IS_SINGLE_URL, false);
        return isSingleUrl == isOverviewVisible;
    }

    /**
     * Checks whether an Intent is sent from the notification of opening tabs in background. If
     * so record user action of clicking the notification.
     * @param intent The {@link Intent} to check.
     */
    public static void recordTabOpenedNotificationClicked(Intent intent) {
        if (IntentUtils.safeHasExtra(intent, EXTRA_IS_SINGLE_URL)) {
            RecordUserAction.record("BrowserActions.TabOpenedNotificationClicked");
        }
    }

    /**
     * Sends a {@link Intent} to open a tab in {@link BrowserActionsService}.
     * @param linkUrl The url to be opened.
     * @param packageName The source package name which requests the tab creation.
     */
    public static void openLinkInBackground(String linkUrl, String packageName) {
        Context context = ContextUtils.getApplicationContext();
        Intent intent = new Intent(context, BrowserActionsService.class);
        intent.setAction(ACTION_TAB_CREATION_START);
        intent.putExtra(EXTRA_LINK_URL, linkUrl);
        intent.putExtra(EXTRA_SOURCE_PACKAGE_NAME, packageName);
        context.startService(intent);
    }
}
