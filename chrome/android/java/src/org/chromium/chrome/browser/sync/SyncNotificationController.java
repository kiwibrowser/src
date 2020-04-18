// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.sync;

import android.app.Activity;
import android.app.Fragment;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.ChromeNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.NotificationConstants;
import org.chromium.chrome.browser.notifications.NotificationManagerProxy;
import org.chromium.chrome.browser.notifications.NotificationManagerProxyImpl;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.components.sync.AndroidSyncSettings;

/**
 * {@link SyncNotificationController} provides functionality for displaying Android notifications
 * regarding the user sync status.
 */
public class SyncNotificationController implements ProfileSyncService.SyncStateChangedListener {
    private static final String TAG = "SyncNotificationController";
    private final Context mApplicationContext;
    private final NotificationManagerProxy mNotificationManager;
    private final Class<? extends Activity> mPassphraseRequestActivity;
    private final Class<? extends Fragment> mAccountManagementFragment;
    private final ProfileSyncService mProfileSyncService;

    public SyncNotificationController(Context context,
            Class<? extends Activity> passphraseRequestActivity,
            Class<? extends Fragment> accountManagementFragment) {
        mApplicationContext = context.getApplicationContext();
        mNotificationManager = new NotificationManagerProxyImpl(
                (NotificationManager) mApplicationContext.getSystemService(
                        Context.NOTIFICATION_SERVICE));
        mProfileSyncService = ProfileSyncService.get();
        assert mProfileSyncService != null;
        mPassphraseRequestActivity = passphraseRequestActivity;
        mAccountManagementFragment = accountManagementFragment;
    }

    /**
     * Callback for {@link ProfileSyncService.SyncStateChangedListener}.
     */
    @Override
    public void syncStateChanged() {
        ThreadUtils.assertOnUiThread();

        // Auth errors take precedence over passphrase errors.
        if (!AndroidSyncSettings.isSyncEnabled(mApplicationContext)) {
            mNotificationManager.cancel(NotificationConstants.NOTIFICATION_ID_SYNC);
            return;
        }
        if (shouldSyncAuthErrorBeShown()) {
            showSyncNotification(
                    mProfileSyncService.getAuthError().getMessage(), createSettingsIntent());
        } else if (mProfileSyncService.isEngineInitialized()
                && mProfileSyncService.isPassphraseRequiredForDecryption()) {
            if (mProfileSyncService.isPassphrasePrompted()) {
                return;
            }
            switch (mProfileSyncService.getPassphraseType()) {
                case IMPLICIT_PASSPHRASE: // Falling through intentionally.
                case FROZEN_IMPLICIT_PASSPHRASE: // Falling through intentionally.
                case CUSTOM_PASSPHRASE:
                    showSyncNotification(R.string.sync_need_passphrase, createPasswordIntent());
                    break;
                case KEYSTORE_PASSPHRASE: // Falling through intentionally.
                default:
                    mNotificationManager.cancel(NotificationConstants.NOTIFICATION_ID_SYNC);
                    return;
            }
        } else {
            mNotificationManager.cancel(NotificationConstants.NOTIFICATION_ID_SYNC);
            return;
        }
    }

    /**
     * Builds and shows a notification for the |message|.
     *
     * @param message Resource id of the message to display in the notification.
     * @param intent Intent to send when the user activates the notification.
     */
    private void showSyncNotification(int message, Intent intent) {
        String title = mApplicationContext.getString(R.string.app_name);
        String text = mApplicationContext.getString(R.string.sign_in_sync) + ": "
                + mApplicationContext.getString(message);

        PendingIntent contentIntent = PendingIntent.getActivity(mApplicationContext, 0, intent, 0);

        // There is no need to provide a group summary notification because the NOTIFICATION_ID_SYNC
        // notification id ensures there's only one sync notification at a time.
        ChromeNotificationBuilder builder =
                NotificationBuilderFactory
                        .createChromeNotificationBuilder(
                                true /* preferCompat */, ChannelDefinitions.CHANNEL_ID_BROWSER)
                        .setAutoCancel(true)
                        .setContentIntent(contentIntent)
                        .setContentTitle(title)
                        .setContentText(text)
                        .setSmallIcon(R.drawable.ic_chrome)
                        .setTicker(text)
                        .setLocalOnly(true)
                        .setGroup(NotificationConstants.GROUP_SYNC);

        Notification notification = builder.buildWithBigTextStyle(text);

        mNotificationManager.notify(NotificationConstants.NOTIFICATION_ID_SYNC, notification);
        NotificationUmaTracker.getInstance().onNotificationShown(
                NotificationUmaTracker.SYNC, ChannelDefinitions.CHANNEL_ID_BROWSER);
    }

    private boolean shouldSyncAuthErrorBeShown() {
        switch (mProfileSyncService.getAuthError()) {
            case NONE:
            case CONNECTION_FAILED:
            case SERVICE_UNAVAILABLE:
            case REQUEST_CANCELED:
            case INVALID_GAIA_CREDENTIALS:
                return false;
            case USER_NOT_SIGNED_UP:
            case CAPTCHA_REQUIRED:
            case ACCOUNT_DELETED:
            case ACCOUNT_DISABLED:
            case TWO_FACTOR:
                return true;
            default:
                Log.w(TAG, "Not showing unknown Auth Error: " + mProfileSyncService.getAuthError());
                return false;
        }
    }

    /**
     * Creates an intent that launches the Chrome settings, and automatically opens the fragment
     * for signed in users.
     *
     * @return the intent for opening the settings
     */
    private Intent createSettingsIntent() {
        return PreferencesLauncher.createIntentForSettingsPage(
                mApplicationContext, mAccountManagementFragment.getCanonicalName());
    }

    /**
     * Creates an intent that launches an activity that requests the users password/passphrase.
     *
     * @return the intent for opening the password/passphrase activity
     */
    private Intent createPasswordIntent() {
        // Make sure we don't prompt too many times.
        mProfileSyncService.setPassphrasePrompted(true);

        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setComponent(new ComponentName(mApplicationContext, mPassphraseRequestActivity));
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        // This activity will become the start of a new task on this history stack.
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        // Clears the task stack above this activity if it already exists.
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        return intent;
    }
}
