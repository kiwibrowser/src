// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Build;
import android.provider.Browser;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.notifications.ChromeNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.NotificationManagerProxy;
import org.chromium.chrome.browser.notifications.NotificationManagerProxyImpl;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.chrome.browser.notifications.channels.ChannelsInitializer;
import org.chromium.chrome.browser.ntp.snippets.ContentSuggestionsNotificationAction;
import org.chromium.chrome.browser.ntp.snippets.ContentSuggestionsNotificationOptOut;
import org.chromium.chrome.browser.preferences.NotificationsPreferences;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.content.browser.BrowserStartupController;
import org.chromium.content.browser.BrowserStartupController.StartupCallback;

import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

/**
 * Provides functionality needed for content suggestion notifications.
 *
 * Exposes helper functions to native C++ code.
 */
public class ContentSuggestionsNotifier {
    private static final String NOTIFICATION_TAG = "ContentSuggestionsNotification";
    private static final String NOTIFICATION_ID_EXTRA = "notification_id";
    private static final String NOTIFICATION_CATEGORY_EXTRA = "category";
    private static final String NOTIFICATION_ID_WITHIN_CATEGORY_EXTRA = "id_within_category";

    private static final String PREF_CHANNEL_CREATED =
            "ntp.content_suggestions.notification.channel_created";

    private static final String PREF_CACHED_ACTION_TAP =
            "ntp.content_suggestions.notification.cached_action_tap";
    private static final String PREF_CACHED_ACTION_DISMISSAL =
            "ntp.content_suggestions.notification.cached_action_dismissal";
    private static final String PREF_CACHED_ACTION_HIDE_DEADLINE =
            "ntp.content_suggestions.notification.cached_action_hide_deadline";
    private static final String PREF_CACHED_ACTION_HIDE_EXPIRY =
            "ntp.content_suggestions.notification.cached_action_hide_expiry";
    private static final String PREF_CACHED_ACTION_HIDE_FRONTMOST =
            "ntp.content_suggestions.notification.cached_action_hide_frontmost";
    private static final String PREF_CACHED_ACTION_HIDE_DISABLED =
            "ntp.content_suggestions.notification.cached_action_hide_disabled";
    private static final String PREF_CACHED_ACTION_HIDE_SHUTDOWN =
            "ntp.content_suggestions.notification.cached_action_hide_shutdown";
    private static final String PREF_CACHED_CONSECUTIVE_IGNORED =
            "ntp.content_suggestions.notification.cached_consecutive_ignored";

    // Tracks which URIs there is an active notification for.
    private static final String PREF_ACTIVE_NOTIFICATIONS =
            "ntp.content_suggestions.notification.active";

    private ContentSuggestionsNotifier() {} // Prevent instantiation

    /**
     * Records the reason why Content Suggestions notifications have been opted out.
     * @see ContentSuggestionsNotificationOptOut;
     */
    public static void recordNotificationOptOut(@ContentSuggestionsNotificationOptOut int reason) {
        nativeRecordNotificationOptOut(reason);
    }

    /**
     * Records an action performed on a Content Suggestions notification.
     * @see ContentSuggestionsNotificationAction;
     */
    public static void recordNotificationAction(@ContentSuggestionsNotificationAction int action) {
        nativeRecordNotificationAction(action);
    }

    /**
     * Opens the content suggestion when notification is tapped.
     */
    public static final class OpenUrlReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            int category = intent.getIntExtra(NOTIFICATION_CATEGORY_EXTRA, -1);
            String idWithinCategory = intent.getStringExtra(NOTIFICATION_ID_WITHIN_CATEGORY_EXTRA);
            openUrl(intent.getData());
            hideNotification(category, idWithinCategory, ContentSuggestionsNotificationAction.TAP);
        }
    }

    /**
     * Records dismissal when notification is swiped away.
     */
    public static final class DeleteReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            int category = intent.getIntExtra(NOTIFICATION_CATEGORY_EXTRA, -1);
            String idWithinCategory = intent.getStringExtra(NOTIFICATION_ID_WITHIN_CATEGORY_EXTRA);
            if (removeActiveNotification(category, idWithinCategory)) {
                recordCachedActionMetric(ContentSuggestionsNotificationAction.DISMISSAL);
            }
        }
    }

    /**
     * Removes the notification after a timeout period.
     */
    public static final class TimeoutReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            int category = intent.getIntExtra(NOTIFICATION_CATEGORY_EXTRA, -1);
            String idWithinCategory = intent.getStringExtra(NOTIFICATION_ID_WITHIN_CATEGORY_EXTRA);
            hideNotification(
                    category, idWithinCategory, ContentSuggestionsNotificationAction.HIDE_DEADLINE);
        }
    }

    private static void openUrl(Uri uri) {
        Context context = ContextUtils.getApplicationContext();
        Intent intent = new Intent()
                                .setAction(Intent.ACTION_VIEW)
                                .setData(uri)
                                .setClass(context, ChromeLauncherActivity.class)
                                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                                .putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName())
                                .putExtra(ShortcutHelper.REUSE_URL_MATCHING_TAB_ELSE_NEW_TAB, true);
        IntentHandler.addTrustedIntentExtras(intent);
        context.startActivity(intent);
    }

    @CalledByNative
    private static boolean showNotification(int category, String idWithinCategory, String url,
            String title, String text, Bitmap image, long timeoutAtMillis, int priority) {
        if (findActiveNotification(category, idWithinCategory) != null) return false;

        // Post notification.
        Context context = ContextUtils.getApplicationContext();
        NotificationManager manager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);

        int nextId = nextNotificationId();
        Uri uri = Uri.parse(url);
        PendingIntent contentIntent = PendingIntent.getBroadcast(context, 0,
                new Intent(context, OpenUrlReceiver.class)
                        .setData(uri)
                        .putExtra(NOTIFICATION_CATEGORY_EXTRA, category)
                        .putExtra(NOTIFICATION_ID_WITHIN_CATEGORY_EXTRA, idWithinCategory),
                0);
        PendingIntent deleteIntent = PendingIntent.getBroadcast(context, 0,
                new Intent(context, DeleteReceiver.class)
                        .setData(uri)
                        .putExtra(NOTIFICATION_CATEGORY_EXTRA, category)
                        .putExtra(NOTIFICATION_ID_WITHIN_CATEGORY_EXTRA, idWithinCategory),
                0);
        ChromeNotificationBuilder builder =
                NotificationBuilderFactory
                        .createChromeNotificationBuilder(true /* preferCompat */,
                                ChannelDefinitions.CHANNEL_ID_CONTENT_SUGGESTIONS)
                        .setContentIntent(contentIntent)
                        .setDeleteIntent(deleteIntent)
                        .setContentTitle(title)
                        .setContentText(text)
                        .setGroup(NOTIFICATION_TAG)
                        .setPriorityBeforeO(priority)
                        .setLargeIcon(image)
                        .setSmallIcon(R.drawable.ic_chrome);
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            PendingIntent settingsIntent = PendingIntent.getActivity(context, 0,
                    PreferencesLauncher.createIntentForSettingsPage(
                            context, NotificationsPreferences.class.getName()),
                    0);
            builder.addAction(R.drawable.settings_cog, context.getString(R.string.preferences),
                    settingsIntent);
        }
        if (priority >= 0) {
            builder.setDefaults(Notification.DEFAULT_ALL);
        }
        manager.notify(NOTIFICATION_TAG, nextId, builder.build());
        NotificationUmaTracker.getInstance().onNotificationShown(
                NotificationUmaTracker.CONTENT_SUGGESTION,
                ChannelDefinitions.CHANNEL_ID_CONTENT_SUGGESTIONS);
        addActiveNotification(new ActiveNotification(nextId, category, idWithinCategory, uri));

        // Set timeout.
        if (timeoutAtMillis != Long.MAX_VALUE) {
            AlarmManager alarmManager =
                    (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
            Intent timeoutIntent =
                    new Intent(context, TimeoutReceiver.class)
                            .setData(Uri.parse(url))
                            .putExtra(NOTIFICATION_ID_EXTRA, nextId)
                            .putExtra(NOTIFICATION_CATEGORY_EXTRA, category)
                            .putExtra(NOTIFICATION_ID_WITHIN_CATEGORY_EXTRA, idWithinCategory);
            alarmManager.set(AlarmManager.RTC, timeoutAtMillis,
                    PendingIntent.getBroadcast(
                            context, 0, timeoutIntent, PendingIntent.FLAG_UPDATE_CURRENT));
        }
        return true;
    }

    /**
     * Hides a notification and records an action to the Actions histogram.
     *
     * If the notification is not actually visible, then no action will be taken, and the action
     * will not be recorded.
     */
    @CalledByNative
    private static void hideNotification(int category, String idWithinCategory, int why) {
        ActiveNotification activeNotification = findActiveNotification(category, idWithinCategory);
        if (!removeActiveNotification(category, idWithinCategory)) return;

        Context context = ContextUtils.getApplicationContext();
        NotificationManager manager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        manager.cancel(NOTIFICATION_TAG, activeNotification.mId);
        recordCachedActionMetric(why);
    }

    @CalledByNative
    private static void hideAllNotifications(int why) {
        Context context = ContextUtils.getApplicationContext();
        NotificationManager manager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        for (ActiveNotification activeNotification : getActiveNotifications()) {
            manager.cancel(NOTIFICATION_TAG, activeNotification.mId);
            if (removeActiveNotification(
                        activeNotification.mCategory, activeNotification.mIdWithinCategory)) {
                recordCachedActionMetric(why);
            }
        }
        assert getActiveNotifications().isEmpty();
    }

    private static class ActiveNotification {
        final int mId;
        final int mCategory;
        final String mIdWithinCategory;
        final Uri mUri;

        ActiveNotification(int id, int category, String idWithinCategory, Uri uri) {
            mId = id;
            mCategory = category;
            mIdWithinCategory = idWithinCategory;
            mUri = uri;
        }

        /** Parses the fields out of a chrome://content-suggestions-notification URI */
        static ActiveNotification fromUri(Uri notificationUri) {
            assert notificationUri.getScheme().equals("chrome");
            assert notificationUri.getAuthority().equals("content-suggestions-notification");
            assert notificationUri.getQueryParameter("id") != null;
            assert notificationUri.getQueryParameter("category") != null;
            assert notificationUri.getQueryParameter("idWithinCategory") != null;
            assert notificationUri.getQueryParameter("uri") != null;

            return new ActiveNotification(Integer.parseInt(notificationUri.getQueryParameter("id")),
                    Integer.parseInt(notificationUri.getQueryParameter("category")),
                    notificationUri.getQueryParameter("idWithinCategory"),
                    Uri.parse(notificationUri.getQueryParameter("uri")));
        }

        /** Serializes the fields to a chrome://content-suggestions-notification URI */
        Uri toUri() {
            return new Uri.Builder()
                    .scheme("chrome")
                    .authority("content-suggestions-notification")
                    .appendQueryParameter("id", Integer.toString(mId))
                    .appendQueryParameter("category", Integer.toString(mCategory))
                    .appendQueryParameter("idWithinCategory", mIdWithinCategory)
                    .appendQueryParameter("uri", mUri.toString())
                    .build();
        }
    }

    /** Returns a mutable copy of the named pref. Never returns null. */
    private static Set<String> getMutableStringSetPreference(
            SharedPreferences prefs, String prefName) {
        Set<String> prefValue = prefs.getStringSet(prefName, null);
        if (prefValue == null) {
            return new HashSet<String>();
        }
        return new HashSet<String>(prefValue);
    }

    /** Adds notification to the "active" set. */
    private static void addActiveNotification(ActiveNotification notification) {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        Set<String> activeNotifications =
                getMutableStringSetPreference(prefs, PREF_ACTIVE_NOTIFICATIONS);
        activeNotifications.add(notification.toUri().toString());
        prefs.edit().putStringSet(PREF_ACTIVE_NOTIFICATIONS, activeNotifications).apply();
    }

    /** Removes notification from the "active" set. Returns false if it wasn't there. */
    private static boolean removeActiveNotification(int category, String idWithinCategory) {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        ActiveNotification notification = findActiveNotification(category, idWithinCategory);
        if (notification == null) return false;

        Set<String> activeNotifications =
                getMutableStringSetPreference(prefs, PREF_ACTIVE_NOTIFICATIONS);
        boolean result = activeNotifications.remove(notification.toUri().toString());
        prefs.edit().putStringSet(PREF_ACTIVE_NOTIFICATIONS, activeNotifications).apply();
        return result;
    }

    private static Collection<ActiveNotification> getActiveNotifications() {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        Set<String> activeNotifications = prefs.getStringSet(PREF_ACTIVE_NOTIFICATIONS, null);
        if (activeNotifications == null) return Collections.emptySet();

        Set<ActiveNotification> result = new HashSet<ActiveNotification>();
        for (String serialized : activeNotifications) {
            Uri notificationUri = Uri.parse(serialized);
            ActiveNotification activeNotification = ActiveNotification.fromUri(notificationUri);
            if (activeNotification != null) result.add(activeNotification);
        }
        return result;
    }

    /** Returns an ActiveNotification if a corresponding one is found, otherwise null. */
    private static ActiveNotification findActiveNotification(
            int category, String idWithinCategory) {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        Set<String> activeNotifications = prefs.getStringSet(PREF_ACTIVE_NOTIFICATIONS, null);
        if (activeNotifications == null) return null;

        for (String serialized : activeNotifications) {
            Uri notificationUri = Uri.parse(serialized);
            ActiveNotification activeNotification = ActiveNotification.fromUri(notificationUri);
            if ((activeNotification != null) && (activeNotification.mCategory == category)
                    && (activeNotification.mIdWithinCategory.equals(idWithinCategory))) {
                return activeNotification;
            }
        }
        return null;
    }

    /** Returns a non-negative integer greater than any active notification's notification ID. */
    private static int nextNotificationId() {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        Set<String> activeNotifications = prefs.getStringSet(PREF_ACTIVE_NOTIFICATIONS, null);
        if (activeNotifications == null) return 0;

        int nextId = 0;
        for (String serialized : activeNotifications) {
            Uri notificationUri = Uri.parse(serialized);
            ActiveNotification activeNotification = ActiveNotification.fromUri(notificationUri);
            if ((activeNotification != null) && (activeNotification.mId >= nextId)) {
                nextId = activeNotification.mId + 1;
            }
        }
        return nextId;
    }

    private static String cachedMetricNameForAction(
            @ContentSuggestionsNotificationAction int action) {
        switch (action) {
            case ContentSuggestionsNotificationAction.TAP:
                return PREF_CACHED_ACTION_TAP;
            case ContentSuggestionsNotificationAction.DISMISSAL:
                return PREF_CACHED_ACTION_DISMISSAL;
            case ContentSuggestionsNotificationAction.HIDE_DEADLINE:
                return PREF_CACHED_ACTION_HIDE_DEADLINE;
            case ContentSuggestionsNotificationAction.HIDE_EXPIRY:
                return PREF_CACHED_ACTION_HIDE_EXPIRY;
            case ContentSuggestionsNotificationAction.HIDE_FRONTMOST:
                return PREF_CACHED_ACTION_HIDE_FRONTMOST;
            case ContentSuggestionsNotificationAction.HIDE_DISABLED:
                return PREF_CACHED_ACTION_HIDE_DISABLED;
            case ContentSuggestionsNotificationAction.HIDE_SHUTDOWN:
                return PREF_CACHED_ACTION_HIDE_SHUTDOWN;
        }
        return "";
    }

    /**
     * Records that an action was performed on a notification.
     *
     * Also tracks the number of consecutively-ignored notifications, resetting it on a tap or
     * otherwise incrementing it.
     *
     * This method may be called when the native library is not loaded. If it is loaded, the metrics
     * will immediately be sent to C++. If not, it will cache them for a later call to
     * flushCachedMetrics().
     *
     * @param action The action to update the pref for.
     */
    private static void recordCachedActionMetric(@ContentSuggestionsNotificationAction int action) {
        String prefName = cachedMetricNameForAction(action);
        assert !prefName.isEmpty();

        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        int currentValue = prefs.getInt(prefName, 0);

        int consecutiveIgnored = prefs.getInt(PREF_CACHED_CONSECUTIVE_IGNORED, 0);
        switch (action) {
            case ContentSuggestionsNotificationAction.TAP:
                consecutiveIgnored = 0;
                break;
            case ContentSuggestionsNotificationAction.DISMISSAL:
            case ContentSuggestionsNotificationAction.HIDE_DEADLINE:
            case ContentSuggestionsNotificationAction.HIDE_EXPIRY:
                ++consecutiveIgnored;
                break;
            case ContentSuggestionsNotificationAction.HIDE_FRONTMOST:
            case ContentSuggestionsNotificationAction.HIDE_DISABLED:
            case ContentSuggestionsNotificationAction.HIDE_SHUTDOWN:
                break; // no change
        }

        prefs.edit()
                .putInt(prefName, currentValue + 1)
                .putInt(PREF_CACHED_CONSECUTIVE_IGNORED, consecutiveIgnored)
                .apply();

        flushCachedMetrics();
    }

    /**
     * Registers the Content Suggestions notification channel on Android O.
     *
     * <p>The registration state is tracked in a preference, as a boolean. If the pref is false or
     * missing, the channel will be (re-)registered.
     *
     * <p>May be called on any Android version; before Android O, it is a no-op.
     *
     * <p>Returns true if the channel was registered, false if not (pre-O, or already registered).
     *
     * @param enabled If false, the channel is created in a disabled state.
     */
    @CalledByNative
    private static boolean registerChannel(boolean enabled) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return false;
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        if (prefs.getBoolean(PREF_CHANNEL_CREATED, false)) return false;

        ChannelsInitializer initializer = new ChannelsInitializer(
                new NotificationManagerProxyImpl(
                        (NotificationManager) ContextUtils.getApplicationContext().getSystemService(
                                Context.NOTIFICATION_SERVICE)),
                ContextUtils.getApplicationContext().getResources());
        if (enabled) {
            initializer.ensureInitialized(ChannelDefinitions.CHANNEL_ID_CONTENT_SUGGESTIONS);
        } else {
            initializer.ensureInitializedAndDisabled(
                    ChannelDefinitions.CHANNEL_ID_CONTENT_SUGGESTIONS);
        }
        prefs.edit().putBoolean(PREF_CHANNEL_CREATED, true).apply();
        return true;
    }

    /**
     * Unregisters the Content Suggestions notification channel on Android O.
     *
     * <p>May be called on any Android version; before Android O, it is a no-op.
     */
    @CalledByNative
    private static void unregisterChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        if (!prefs.getBoolean(PREF_CHANNEL_CREATED, false)) return;

        NotificationManagerProxy manager = new NotificationManagerProxyImpl(
                (NotificationManager) ContextUtils.getApplicationContext().getSystemService(
                        Context.NOTIFICATION_SERVICE));
        manager.deleteNotificationChannel(ChannelDefinitions.CHANNEL_ID_CONTENT_SUGGESTIONS);
        prefs.edit().remove(PREF_CHANNEL_CREATED).apply();
    }

    /**
     * Invokes nativeReceiveFlushedMetrics() with cached metrics and resets them.
     *
     * It may be called from either native or Java code. If the browser has not been started-up, or
     * startup has not completed (as when the native component flushes metrics during creation of
     * the keyed service) the flush will be deferred until startup is complete.
     */
    @CalledByNative
    private static void flushCachedMetrics() {
        BrowserStartupController browserStartup =
                BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER);
        if (!browserStartup.isStartupSuccessfullyCompleted()) {
            browserStartup.addStartupCompletedObserver(new StartupCallback() {
                @Override
                public void onSuccess() {
                    flushCachedMetrics();
                }
                @Override
                public void onFailure() {}
            });
            return;
        }

        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        int tapCount = prefs.getInt(PREF_CACHED_ACTION_TAP, 0);
        int dismissalCount = prefs.getInt(PREF_CACHED_ACTION_DISMISSAL, 0);
        int hideDeadlineCount = prefs.getInt(PREF_CACHED_ACTION_HIDE_DEADLINE, 0);
        int hideExpiryCount = prefs.getInt(PREF_CACHED_ACTION_HIDE_EXPIRY, 0);
        int hideFrontmostCount = prefs.getInt(PREF_CACHED_ACTION_HIDE_FRONTMOST, 0);
        int hideDisabledCount = prefs.getInt(PREF_CACHED_ACTION_HIDE_DISABLED, 0);
        int hideShutdownCount = prefs.getInt(PREF_CACHED_ACTION_HIDE_SHUTDOWN, 0);
        int consecutiveIgnored = prefs.getInt(PREF_CACHED_CONSECUTIVE_IGNORED, 0);

        if (tapCount > 0 || dismissalCount > 0 || hideDeadlineCount > 0 || hideExpiryCount > 0
                || hideFrontmostCount > 0 || hideDisabledCount > 0 || hideShutdownCount > 0) {
            nativeReceiveFlushedMetrics(tapCount, dismissalCount, hideDeadlineCount,
                    hideExpiryCount, hideFrontmostCount, hideDisabledCount, hideShutdownCount,
                    consecutiveIgnored);
            prefs.edit()
                    .remove(PREF_CACHED_ACTION_TAP)
                    .remove(PREF_CACHED_ACTION_DISMISSAL)
                    .remove(PREF_CACHED_ACTION_HIDE_DEADLINE)
                    .remove(PREF_CACHED_ACTION_HIDE_EXPIRY)
                    .remove(PREF_CACHED_ACTION_HIDE_FRONTMOST)
                    .remove(PREF_CACHED_ACTION_HIDE_DISABLED)
                    .remove(PREF_CACHED_ACTION_HIDE_SHUTDOWN)
                    .remove(PREF_CACHED_CONSECUTIVE_IGNORED)
                    .apply();
        }
    }

    private static native void nativeReceiveFlushedMetrics(int tapCount, int dismissalCount,
            int hideDeadlineCount, int hideExpiryCount, int hideFrontmostCount,
            int hideDisabledCount, int hideShutdownCount, int consecutiveIgnored);
    private static native void nativeRecordNotificationOptOut(
            @ContentSuggestionsNotificationOptOut int reason);
    private static native void nativeRecordNotificationAction(
            @ContentSuggestionsNotificationAction int action);
}
