package org.chromium.chrome.browser.mises;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Build;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.support.v4.app.NotificationCompat;

import lcd.MLightNode;
import lcd.Lcd;

import org.chromium.base.Log;
import org.chromium.chrome.R;

public class MisesLCDService extends Service {
    private static final String CHANNEL_ID = "1001";
    private static final String CHANNEL_NAME = "Event Tracker";
    private static final int SERVICE_ID = 1;
    public static Boolean IS_RUNNING = false;
    private static final String TAG = "MISES_LCD_SERVICE";
    private static final String ACTION_STOP_FOREGROUND_SERVICE = "ACTION_STOP_FOREGROUND_SERVICE";
    private static final String ACTION_OPEN_APP = "ACTION_OPEN_APP";
    private static final String KEY_DATA = "KEY_DATA";

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "onBind");
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate");
        startForegroundService();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "ON START COMMAND");
        if (intent != null) {
            if (intent.getAction() != null) {
                if (intent.getAction() .equals(ACTION_STOP_FOREGROUND_SERVICE)) {
                    stopService();
                } else if (intent.getAction().equals(ACTION_OPEN_APP)) {
                    String key_data = intent.getStringExtra(KEY_DATA);
                    openAppHomePage(key_data);
                }
            }
        }
        return START_STICKY;
    }

    private void openAppHomePage(String keydata) {

    }

    private void startForegroundService() {
        //Create Notification channel for all the notifications sent from this app.
        createNotificationChannel();

        // Start foreground service.
        startLCDService();

    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel chan = new NotificationChannel(
                    CHANNEL_ID,
                    CHANNEL_NAME, NotificationManager.IMPORTANCE_DEFAULT
            );

            chan.setLightColor(Color.BLUE);
            chan.setLockscreenVisibility(NotificationCompat.VISIBILITY_PRIVATE);
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            manager.createNotificationChannel(chan);
        }
    }

    private void startLCDService() {
        String description = getString(R.string.msg_notification_service_desc);
        String title = String.format(
                getString(R.string.title_foreground_service_notification),
                getString(R.string.app_name)
        );

        startForeground(SERVICE_ID, getStickyNotification(title, description));
        IS_RUNNING = true;

        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "mises light node starting");
                try {
                    Lcd.setHomePath(MisesLCDService.this.getApplicationContext().getFilesDir().getAbsolutePath());
                    MLightNode node = Lcd.newMLightNode();
                    node.setChainID("test");
                    node.setEndpoints("http://e1.mises.site:26657", "http://e2.mises.site:26657");
                    node.setTrust("56100", "98B49C7E46A2903BEB3816C501155325EA10FC1CCEC4F58AF62253E72061801B");
                    node.serve("tcp://0.0.0.0:26657");
                    Log.d(TAG, "mises light node started");
                } catch (Exception e) {
                    Log.e(TAG, "mises light node start error");
                }
            }
        }).start();
    }

    private Notification getStickyNotification(String title, String message) {
        PendingIntent pendingIntent = PendingIntent.getActivity(getApplicationContext(), 0, new Intent(), 0);

        // Create notification builder.
        NotificationCompat.Builder builder = new NotificationCompat.Builder(getApplicationContext(), CHANNEL_ID);
        // Make notification show big text.
        NotificationCompat.BigTextStyle bigTextStyle = new NotificationCompat.BigTextStyle();
        bigTextStyle.setBigContentTitle(title);
        bigTextStyle.bigText(message);
        // Set big text style.
        builder.setStyle(bigTextStyle);
        builder.setWhen(System.currentTimeMillis());
        builder.setSmallIcon(R.mipmap.ic_launcher);
        //val largeIconBitmap = BitmapFactory.decodeResource(resources, R.drawable.ic_alarm_on)
        //builder.setLargeIcon(largeIconBitmap)
        // Make the notification max priority.
        builder.setPriority(NotificationCompat.PRIORITY_DEFAULT);

        // Make head-up notification.
        builder.setFullScreenIntent(pendingIntent, true);


        // Add Open App button in notification.
        Intent openAppIntent = new Intent(getApplicationContext(), MisesLCDService.class);
        openAppIntent.setAction(ACTION_OPEN_APP);
        openAppIntent.putExtra(KEY_DATA, "" + System.currentTimeMillis());
        PendingIntent pendingPlayIntent = PendingIntent.getService(getApplicationContext(), 0, openAppIntent, 0);
        NotificationCompat.Action openAppAction = new NotificationCompat.Action(
                android.R.drawable.ic_menu_view,
                getString(R.string.lbl_btn_sticky_notification_open_app),
                pendingPlayIntent
        );
        builder.addAction(openAppAction);

        // Build the notification.
        return builder.build();
    }

    private void stopService() {
        // Stop foreground service and remove the notification.
        stopForeground(true);
        // Stop the foreground service.
        stopSelf();

        IS_RUNNING = false;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
        IS_RUNNING = false;
    }
}