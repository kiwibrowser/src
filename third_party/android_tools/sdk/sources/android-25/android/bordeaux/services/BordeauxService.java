/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.bordeaux.services;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Process;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

//import android.bordeaux.R;
import android.util.Log;

import java.io.*;

/**
 * Machine Learning service that runs in a remote process.
 * The application doesn't use this class directly.
 *
 */
public class BordeauxService extends Service {
    private final String TAG = "BordeauxService";
    /**
     * This is a list of callbacks that have been registered with the
     * service.
     * It's a place holder for future communications with all registered
     * clients.
     */
    final RemoteCallbackList<IBordeauxServiceCallback> mCallbacks =
            new RemoteCallbackList<IBordeauxServiceCallback>();

    int mValue = 0;
    NotificationManager mNotificationManager;

    BordeauxSessionManager mSessionManager;
    AggregatorManager mAggregatorManager;
    TimeStatsAggregator mTimeStatsAggregator;
    LocationStatsAggregator mLocationStatsAggregator;
    MotionStatsAggregator mMotionStatsAggregator;

    @Override
    public void onCreate() {
        Log.i(TAG, "Bordeaux service created.");
        //mNotificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        mSessionManager = new BordeauxSessionManager(this);
        mMotionStatsAggregator = new MotionStatsAggregator();
        mLocationStatsAggregator = new LocationStatsAggregator(this);
        mTimeStatsAggregator = new TimeStatsAggregator();
        mAggregatorManager = AggregatorManager.getInstance();
        mAggregatorManager.registerAggregator(mMotionStatsAggregator, mAggregatorManager);
        mAggregatorManager.registerAggregator(mLocationStatsAggregator, mAggregatorManager);
        mAggregatorManager.registerAggregator(mTimeStatsAggregator, mAggregatorManager);

        // Display a notification about us starting.
        // TODO: don't display the notification after the service is
        // automatically started by the system, currently it's useful for
        // debugging.
        showNotification();
    }

    @Override
    public void onDestroy() {
        // Save the sessions
        mSessionManager.saveSessions();

        // Cancel the persistent notification.
        //mNotificationManager.cancel(R.string.remote_service_started);

        // Tell the user we stopped.
        //Toast.makeText(this, R.string.remote_service_stopped, Toast.LENGTH_SHORT).show();

        // Unregister all callbacks.
        mCallbacks.kill();

        mLocationStatsAggregator.release();

        Log.i(TAG, "Bordeaux service stopped.");
    }

    @Override
    public IBinder onBind(Intent intent) {
        // Return the requested interface.
        if (IBordeauxService.class.getName().equals(intent.getAction())) {
            return mBinder;
        }
        return null;
    }


    // The main interface implemented by the service.
    private final IBordeauxService.Stub mBinder = new IBordeauxService.Stub() {
        private IBinder getLearningSession(Class learnerClass, String name) {
            PackageManager pm = getPackageManager();
            String uidname = pm.getNameForUid(getCallingUid());
            Log.i(TAG,"Name for uid: " + uidname);
            BordeauxSessionManager.SessionKey key =
                    mSessionManager.getSessionKey(uidname, learnerClass, name);
            Log.i(TAG, "request learning session: " + key.value);
            try {
                IBinder iLearner = mSessionManager.getSessionBinder(learnerClass, key);
                return iLearner;
            } catch (RuntimeException e) {
                Log.e(TAG, "Error getting learning interface" + e);
                return null;
            }
        }

        public IBinder getClassifier(String name) {
            return getLearningSession(Learning_MulticlassPA.class, name);
        }

        public IBinder getRanker(String name) {
            return getLearningSession(Learning_StochasticLinearRanker.class, name);
        }

        public IBinder getPredictor(String name) {
            return getLearningSession(Predictor.class, name);
        }

        public IBinder getAggregatorManager() {
            return (IBinder) mAggregatorManager;
        }

        public void registerCallback(IBordeauxServiceCallback cb) {
            if (cb != null) mCallbacks.register(cb);
        }

        public void unregisterCallback(IBordeauxServiceCallback cb) {
            if (cb != null) mCallbacks.unregister(cb);
        }
    };

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        Toast.makeText(this, "Task removed: " + rootIntent, Toast.LENGTH_LONG).show();
    }

    /**
     * Show a notification while this service is running.
     * TODO: remove the code after production (when service is loaded
     * automatically by the system).
     */
    private void showNotification() {
        /*// In this sample, we'll use the same text for the ticker and the expanded notification
        CharSequence text = getText(R.string.remote_service_started);

        // The PendingIntent to launch our activity if the user selects this notification
        PendingIntent contentIntent =
                PendingIntent.getActivity(this, 0,
                                          new Intent("android.bordeaux.DEBUG_CONTROLLER"), 0);

       // // Set the info for the views that show in the notification panel.

        Notification.Builder builder = new Notification.Builder(this);
        builder.setSmallIcon(R.drawable.ic_bordeaux);
        builder.setWhen(System.currentTimeMillis());
        builder.setTicker(text);
        builder.setContentTitle(text);
        builder.setContentIntent(contentIntent);
        Notification notification = builder.getNotification();
        // Send the notification.
        // We use a string id because it is a unique number.  We use it later to cancel.
        mNotificationManager.notify(R.string.remote_service_started, notification); */
    }

}
