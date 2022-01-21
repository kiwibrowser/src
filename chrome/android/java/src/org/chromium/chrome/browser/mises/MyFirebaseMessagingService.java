package org.chromium.chrome.browser.mises;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Build;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationManagerCompat;

import org.chromium.base.Log;
import org.chromium.chrome.browser.ChromeTabbedActivity;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;
import org.chromium.chrome.R;
import org.chromium.components.url_formatter.UrlFormatter;

public class MyFirebaseMessagingService extends FirebaseMessagingService {
    private static final String CHANNEL_NAME = "News";

    @Override
    public void onMessageReceived(RemoteMessage remoteMessage) {
        super.onMessageReceived(remoteMessage);
        if (remoteMessage == null)
            return;
        if (remoteMessage.getNotification() != null && remoteMessage.getData() != null) {
            final int tag = (int) System.currentTimeMillis();
            String CHANNEL_ID = getString(R.string.default_fcm_channel_id);
            Log.d("luobo notification", remoteMessage.getNotification().getTitle() + " " + remoteMessage.getNotification().getBody());
            Log.d("luobo data", remoteMessage.getData().toString());
            String url = remoteMessage.getData().get("mises_url");
            Intent intent = null;
            if (url != null && !url.isEmpty()) {
                String fixedUrl = UrlFormatter.fixupUrl(url);
                intent = new Intent(Intent.ACTION_VIEW, Uri.parse(fixedUrl));
                intent.setPackage(getPackageName());
            }  else {
                intent = new Intent(this, ChromeTabbedActivity.class);
            }
            PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_ONE_SHOT);
            NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                NotificationChannel channel = new NotificationChannel(CHANNEL_ID, CHANNEL_NAME, NotificationManager.IMPORTANCE_DEFAULT);
                notificationManager.createNotificationChannel(channel);
            }
            Uri defaultSoundUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
            NotificationCompat.Builder notificationBuilder = new NotificationCompat.Builder(this, CHANNEL_ID)
                    .setSmallIcon(R.mipmap.ic_launcher)
                    .setContentTitle(remoteMessage.getNotification().getTitle())
                    .setContentText(remoteMessage.getNotification().getBody())
                    .setAutoCancel(true)
                    .setSound(defaultSoundUri)
                    .setContentIntent(pendingIntent);

            NotificationManagerCompat.from(this).notify(0, notificationBuilder.build());
        }
    }
}

