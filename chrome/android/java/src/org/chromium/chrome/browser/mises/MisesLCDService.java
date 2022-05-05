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
import android.os.Handler;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.support.v4.app.NotificationCompat;

import lcd.MLightNode;
import lcd.Lcd;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Random;

public class MisesLCDService extends Service {
    private static final String CHANNEL_ID = "1001";
    private static final String CHANNEL_NAME = "Event Tracker";
    private static final int SERVICE_ID = 1;
    public static Boolean IS_RUNNING = false;
    private static final String TAG = "MISES_LCD_SERVICE";
    private static final String ACTION_RESTART_FOREGROUND_SERVICE = "ACTION_RESTART_FOREGROUND_SERVICE";
    private static final String ACTION_OPEN_APP = "ACTION_OPEN_APP";
    public static final String KEY_DATA = "KEY_DATA";
    private Handler retryHandler = new Handler();
    private int retryCounter = 0;

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
                if (intent.getAction() .equals(ACTION_RESTART_FOREGROUND_SERVICE)) {
		    retryCounter = 0;
                    startLCDService();
                } else if (intent.getAction().equals(ACTION_OPEN_APP)) {
                    String key_data = intent.getStringExtra(KEY_DATA);
                    openAppHomePage(key_data);
                }
            }
        }
        return START_STICKY;
    }

    private void openAppHomePage(String keydata) {

        sendBroadcast(new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS));

        Intent newintent = new Intent(MisesLCDService.this.getApplicationContext(), ChromeTabbedActivity.class);
        newintent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK );
        startActivity(newintent);



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

        startForeground(SERVICE_ID, getStickyNotification(
                getString(R.string.title_foreground_service_notification_running),
                getString(R.string.msg_notification_service_desc), true
        ));
        IS_RUNNING = true;

        retryHandler.removeCallbacksAndMessages(null);
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "mises light node starting");
                String block_height = "";
                String block_hash = "";
                String primary_node = "";
                String witness_nodes = "";
                String chain_id = "";
                boolean first_run = false;
                final String home_path = MisesLCDService.this.getApplicationContext().getFilesDir().getAbsolutePath() + File.separator;
                File f = new File(home_path+ "//.misestm//light//light-client-db.db");
                if (!f.exists()) {
                    first_run = true;
                }
                ArrayList<String> trust_nodes = new ArrayList<String>();
                try {
                    JSONObject ret = MisesLCDService.this.MisesApiGet("mises/chaininfo");
                    if (ret != null && ret.has("code")) {
                        int code = ret.getInt("code");
                        if (code == 0) {
                            JSONObject data = ret.getJSONObject("data");
                            if (data != null) {
                                block_hash = data.getString("block_hash");
                                block_height = data.getInt("block_height") + "";
                                JSONArray nodes  = data.getJSONArray("trust_nodes");
                                if (nodes != null) {
                                    for( int i = 0 ;i < nodes.length(); i ++ ) {
                                        trust_nodes.add(nodes.getString(i));
                                    }
                                }
                                chain_id = data.getString("chain_id");
                            }

                        }
                    }
                }catch (Exception e) {
                    Log.e(TAG, "fail to get mises chain ino");

                }finally {
                    if (block_hash.isEmpty() || block_height.isEmpty()) {
                        if (first_run) {
                            //if no old light data, force trust an hash
                            block_hash = "98B49C7E46A2903BEB3816C501155325EA10FC1CCEC4F58AF62253E72061801B";
                            block_height = "56100";
                            Log.i(TAG, "trust the default block");
                        } else {
                            //leave block_hash and block_height empty so that trust the existing block
                            Log.i(TAG, "trust the existing block");
                        }
                    }
                    if (trust_nodes.isEmpty()) {
                        trust_nodes.add("http://e1.mises.site:26657");
                        trust_nodes.add("http://e2.mises.site:26657");
                        trust_nodes.add("http://w1.mises.site:26657");
                        trust_nodes.add("http://w2.mises.site:26657");
                    }
                    if (chain_id.isEmpty()) {
                        chain_id = "mainnet";
                    }
                    Random rand = new Random(System.currentTimeMillis());
                    int n = rand.nextInt(trust_nodes.size());
                    primary_node = trust_nodes.remove(n);
                    StringBuilder str = new StringBuilder("");
                    // Traversing the ArrayList
                    for (String node : trust_nodes) {
                        str.append(node).append(",");
                    }
                    if (trust_nodes.size() > 0) {
                        str.deleteCharAt(str.lastIndexOf(","));
                    }
                    witness_nodes = str.toString();

                }
                try {


                    Lcd.setHomePath(home_path);
                    MLightNode node = Lcd.newMLightNode();
                    node.setChainID(chain_id);
                    node.setEndpoints(primary_node, witness_nodes);
                    node.setTrust(block_height, block_hash);
                    node.serve("tcp://0.0.0.0:26657");
                    Log.d(TAG, "mises light node finish");
                } catch (Exception e) {
                    Log.e(TAG, "mises light node start error");
                }

                startForeground(SERVICE_ID, getStickyNotification(
                        getString(R.string.title_foreground_service_notification_error),
                        getString(R.string.msg_notification_service_desc), false
                ));
		int retryDelay = 30000;
		if (retryCounter < 0) {
		  retryDelay = 30000;
		} else if (retryCounter < 6) {
		  retryDelay = (int)Math.round(Math.pow(2, retryCounter) * 30000);
		} else {
	 	  retryDelay = 960000;
		}
		retryCounter += 1;
                retryHandler.postDelayed( () -> {
                    startLCDService();
                }, retryDelay);
            }
        }).start();
    }

    private Notification getStickyNotification(String title, String message, boolean running) {
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
        if (running) {
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
        } else {
            Intent restartIntent = new Intent(getApplicationContext(), MisesLCDService.class);
            restartIntent.setAction(ACTION_RESTART_FOREGROUND_SERVICE);
            restartIntent.putExtra(KEY_DATA, "" + System.currentTimeMillis());
            PendingIntent pendingPlayIntent = PendingIntent.getService(getApplicationContext(), 0, restartIntent, 0);
            NotificationCompat.Action restartAction = new NotificationCompat.Action(
                    android.R.drawable.ic_menu_view,
                    getString(R.string.lbl_btn_sticky_notification_restart_lcd),
                    pendingPlayIntent
            );
            builder.addAction(restartAction);
        }


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


    private JSONObject MisesApiGet(String path) {
        JSONObject result = null;
        HttpURLConnection urlConnection = null;
        try {
            URL url = new URL("https://api.alb.mises.site/api/v1/" + path);
            urlConnection = (HttpURLConnection) url.openConnection();
            urlConnection.setConnectTimeout(20000);
            urlConnection.setDoOutput(false);
            urlConnection.setDoInput(true);
            urlConnection.setUseCaches(false);
            urlConnection.setRequestMethod("GET");
//            String userAgent = ContentUtils.getBrowserUserAgent();
//            urlConnection.setRequestProperty("User-Agent", userAgent);
//            urlConnection.setRequestProperty("Authorization", "Bearer " + mToken);
            urlConnection.setRequestProperty("Connection", "Keep-alive");
            urlConnection.setRequestProperty("Charset", "UTF-8");
            urlConnection.setRequestProperty("Content-Type", "application/json");


            int resCode = urlConnection.getResponseCode();
            Log.d(TAG, "mises api get " + path + ", ret " + resCode);
            if (resCode == 200) {
                InputStream is = urlConnection.getInputStream();
                ByteArrayOutputStream bo = new ByteArrayOutputStream();
                int i = is.read();
                while (i != -1) {
                    bo.write(i);
                    i = is.read();
                }
                String resJson = bo.toString();
                result = new JSONObject(resJson);
//                int code = -1;
//                if (resJsonObject.has("code")) {
//                    code = resJsonObject.getInt("code");
//                }
//                return code;
            } else {
                InputStream is = urlConnection.getErrorStream();
                ByteArrayOutputStream bo = new ByteArrayOutputStream();
                int i = is.read();
                while (i != -1) {
                    bo.write(i);
                    i = is.read();
                }
                String err = bo.toString();
                Log.e(TAG, "Share to mises " + err);
            }
        } catch (JSONException e) {
            Log.e(TAG, "mises api get error " + e.toString());
        } catch (MalformedURLException e) {
            Log.e(TAG, "mises api get  error " + e.toString());
        } catch (IOException e) {
            Log.e(TAG, "mises api get  error " + e.toString());
        } catch (IllegalStateException e) {
            Log.e(TAG, "mises api get  error " + e.toString());
        } finally {
            if (urlConnection != null) urlConnection.disconnect();
        }
        return result;
    }
}
