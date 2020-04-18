/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.support.customtabs.trusted;

import android.annotation.TargetApi;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.StrictMode;
import android.service.notification.StatusBarNotification;
import android.support.annotation.CallSuper;
import android.support.annotation.Nullable;
import android.support.customtabs.trusted.TrustedWebActivityServiceWrapper.ActiveNotificationsArgs;
import android.support.customtabs.trusted.TrustedWebActivityServiceWrapper.CancelNotificationArgs;
import android.support.customtabs.trusted.TrustedWebActivityServiceWrapper.NotifyNotificationArgs;
import android.support.customtabs.trusted.TrustedWebActivityServiceWrapper.ResultArgs;
import android.support.v4.app.NotificationManagerCompat;

import java.util.Arrays;
import java.util.Locale;

/**
 * The TrustedWebActivityService lives in a client app and serves requests from a Trusted Web
 * Activity provider (such as Google Chrome). At present it only serves requests to display
 * notifications.
 * <p>
 * When the provider receives a notification from a scope that is associated with a Trusted Web
 * Activity client app, it will attempt to connect to a TrustedWebActivityService and forward calls.
 * This allows the client app to display the notifications itself, meaning it is attributable to the
 * client app and is managed by notification permissions of the client app, not the provider.
 * <p>
 * TrustedWebActivityService is usable as it is, by adding the following to your AndroidManifest:
 *
 * <pre>
 * <service
 *     android:name="android.support.customtabs.trusted.TrustedWebActivityService"
 *     android:enabled="true"
 *     android:exported="true">
 *
 *     <meta-data android:name="android.support.customtabs.trusted.SMALL_ICON"
 *         android:resource="@drawable/ic_notification_icon" />
 *
 *     <intent-filter>
 *         <action android:name="android.support.customtabs.trusted.TRUSTED_WEB_ACTIVITY_SERVICE"/>
 *         <category android:name="android.intent.category.DEFAULT"/>
 *     </intent-filter>
 * </service>
 * </pre>
 *
 * The SMALL_ICON resource should point to a drawable to be used for the notification's small icon.
 * <p>
 * Alternatively for greater customization, TrustedWebActivityService can be extended and
 * {@link #onCreate}, {@link #getSmallIconId}, {@link #notifyNotificationWithChannel} and
 * {@link #cancelNotification} can be overridden. In this case the manifest entry should be updated
 * to point to the extending class.
 * <p>
 * As this is an AIDL Service, calls to {@link #getSmallIconId},
 * {@link #notifyNotificationWithChannel} and {@link #cancelNotification} can occur on different
 * Binder threads, so overriding implementations need to be thread-safe.
 * <p>
 * For security, the TrustedWebActivityService will check that whatever connects to it is the
 * Trusted Web Activity provider that it was previously verified with. For testing,
 * {@link #setVerifiedProviderForTesting} can be used to to allow connections from the given
 * package.
 */
public class TrustedWebActivityService extends Service {
    /** An Intent Action used by the provider to find the TrustedWebActivityService or subclass. */
    public static final String INTENT_ACTION =
            "android.support.customtabs.trusted.TRUSTED_WEB_ACTIVITY_SERVICE";
    /** The Android Manifest meta-data name to specify a small icon id to use. */
    public static final String SMALL_ICON_META_DATA_NAME =
            "android.support.customtabs.trusted.SMALL_ICON";

    private static final String PREFS_FILE = "TrustedWebActivityVerifiedProvider";
    private static final String PREFS_VERIFIED_PROVIDER = "Provider";

    private NotificationManager mNotificationManager;

    public int mVerifiedUid = -1;

    private final ITrustedWebActivityService.Stub mBinder =
            new ITrustedWebActivityService.Stub() {
        @Override
        public Bundle notifyNotificationWithChannel(Bundle bundle) {
            checkCaller();

            NotifyNotificationArgs args = NotifyNotificationArgs.fromBundle(bundle);

            boolean success = TrustedWebActivityService.this.notifyNotificationWithChannel(
                    args.platformTag, args.platformId, args.notification, args.channelName);

            return new ResultArgs(success).toBundle();
        }

        @Override
        public void cancelNotification(Bundle bundle) {
            checkCaller();

            CancelNotificationArgs args = CancelNotificationArgs.fromBundle(bundle);

            TrustedWebActivityService.this.cancelNotification(args.platformTag, args.platformId);
        }

        @Override
        public Bundle getActiveNotifications() {
            checkCaller();

            return new ActiveNotificationsArgs(
                    TrustedWebActivityService.this.getActiveNotifications()).toBundle();
        }

        @Override
        public int getSmallIconId() {
            checkCaller();

            return TrustedWebActivityService.this.getSmallIconId();
        }

        private void checkCaller() {
            if (mVerifiedUid == -1) {
                String[] packages = getPackageManager().getPackagesForUid(getCallingUid());
                // We need to read Preferences. This should only be called on the Binder thread
                // which is designed to handle long running, blocking tasks, so disk I/O should be
                // OK.
                StrictMode.ThreadPolicy policy = StrictMode.allowThreadDiskReads();
                try {
                    String verifiedPackage = getPreferences(TrustedWebActivityService.this)
                            .getString(PREFS_VERIFIED_PROVIDER, null);

                    if (Arrays.asList(packages).contains(verifiedPackage)) {
                        mVerifiedUid = getCallingUid();

                        return;
                    }
                } finally {
                    StrictMode.setThreadPolicy(policy);
                }
            }

            if (mVerifiedUid == getCallingUid()) return;

            throw new SecurityException("Caller is not verified as Trusted Web Activity provider.");
        }
    };

    /**
     * Called by the system when the service is first created. Do not call this method directly.
     * Overrides must call {@code super.onCreate()}.
     */
    @Override
    @CallSuper
    public void onCreate() {
        super.onCreate();
        mNotificationManager =
                (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
    }

    /**
     * Displays a notification.
     * @param platformTag The notification tag, see
     *                    {@link NotificationManager#notify(String, int, Notification)}.
     * @param platformId The notification id, see
     *                   {@link NotificationManager#notify(String, int, Notification)}.
     * @param notification The notification to be displayed, constructed by the provider.
     * @param channelName The name of the notification channel that the notification should be
     *                    displayed on. This method gets or creates a channel from the name and
     *                    modifies the notification to use that channel.
     * @return Whether the notification was successfully displayed (the channel/app may be blocked
     *         by the user).
     */
    protected boolean notifyNotificationWithChannel(String platformTag, int platformId,
            Notification notification, String channelName) {
        ensureOnCreateCalled();

        if (!NotificationManagerCompat.from(this).areNotificationsEnabled()) return false;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            String channelId = channelNameToId(channelName);
            // Create the notification channel, (no-op if already created).
            mNotificationManager.createNotificationChannel(new NotificationChannel(channelId,
                    channelName, NotificationManager.IMPORTANCE_DEFAULT));

            // Check that the channel is enabled.
            if (mNotificationManager.getNotificationChannel(channelId).getImportance() ==
                    NotificationManager.IMPORTANCE_NONE) {
                return false;
            }

            // Set our notification to have that channel.
            Notification.Builder builder = Notification.Builder.recoverBuilder(this, notification);
            builder.setChannelId(channelId);
            notification = builder.build();
        }

        mNotificationManager.notify(platformTag, platformId, notification);
        return true;
    }

    /**
     * Cancels a notification.
     * @param platformTag The notification tag, see
     *                    {@link NotificationManager#cancel(String, int)}.
     * @param platformId The notification id, see
     *                   {@link NotificationManager#cancel(String, int)}.
     */
    protected void cancelNotification(String platformTag, int platformId) {
        ensureOnCreateCalled();
        mNotificationManager.cancel(platformTag, platformId);
    }

    /**
     * Returns a list of active notifications, essentially calling
     * NotificationManager#getActiveNotifications. The default implementation does not work on
     * pre-Android M.
     * @return An array of StatusBarNotification.
     */
    @TargetApi(Build.VERSION_CODES.M)
    protected StatusBarNotification[] getActiveNotifications() {
        ensureOnCreateCalled();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return mNotificationManager.getActiveNotifications();
        }
        throw new IllegalStateException("getActiveNotifications cannot be called pre-M.");
    }

    /**
     * Returns the Android resource id of a drawable to be used for the small icon of the
     * notification. This is called by the provider as it is constructing the notification, so a
     * complete notification can be passed to the client.
     *
     * Default behaviour looks for meta-data with the name {@link #SMALL_ICON_META_DATA_NAME} in
     * service section of the manifest.
     * @return A resource id for the small icon, or -1 if not found.
     */
    protected int getSmallIconId() {
        try {
            ServiceInfo info = getPackageManager().getServiceInfo(
                    new ComponentName(this, getClass()), PackageManager.GET_META_DATA);

            if (info.metaData == null) return -1;

            return info.metaData.getInt(SMALL_ICON_META_DATA_NAME, -1);
        } catch (PackageManager.NameNotFoundException e) {
            // Will only happen if the package provided (the one we are running in) is not
            // installed - so should never happen.
            return -1;
        }
    }

    @Override
    final public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    final public boolean onUnbind(Intent intent) {
        mVerifiedUid = -1;

        return super.onUnbind(intent);
    }

    /**
     * Should *not* be called on UI Thread, as accessing Preferences may hit disk.
     */
    private static SharedPreferences getPreferences(Context context) {
        return context.getSharedPreferences(PREFS_FILE, Context.MODE_PRIVATE);
    }

    /**
     * Sets the package that this service will accept connections from. This should only be used for
     * testing as the appropriate provider will be set when the client app launches a Trusted
     * Web Activity.
     * @param context A context to be used to access SharedPreferences.
     * @param provider The package of the provider to accept connections from or null to clear.
     */
    public static final void setVerifiedProviderForTesting(Context context,
            @Nullable String provider) {
        setVerifiedProvider(context, provider);
    }

    /**
     * Sets the package that this service will accept connections from.
     * @param context A context to be used to access SharedPreferences.
     * @param provider The package of the provider to accept connections from or null to clear.
     * @hide
     */
    public static final void setVerifiedProvider(final Context context,
            @Nullable String provider) {
        final String providerEmptyChecked =
                (provider == null || provider.isEmpty()) ? null : provider;

        // Perform on a background thread as accessing Preferences may cause disk access.
        new AsyncTask<Void, Void, Void>() {

            @Override
            protected Void doInBackground(Void... voids) {
                SharedPreferences.Editor editor = getPreferences(context).edit();
                editor.putString(PREFS_VERIFIED_PROVIDER, providerEmptyChecked);
                editor.apply();
                return null;
            }

        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private static String channelNameToId(String name) {
        return name.toLowerCase(Locale.ROOT).replace(' ', '_') + "_channel_id";
    }

    private void ensureOnCreateCalled() {
        if (mNotificationManager != null) return;
        throw new IllegalStateException("TrustedWebActivityService has not been properly "
                + "initialized. Did onCreate() call super.onCreate()?");
    }
}
