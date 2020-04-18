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

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.Process;
import android.os.SystemClock;
import android.text.format.Time;
import android.util.Log;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// TODO: add functionality to detect speed (use GPS) when needed
// withouth draining the battery quickly
public class LocationStatsAggregator extends Aggregator {
    final String TAG = "LocationStatsAggregator";
    public static final String CURRENT_LOCATION = "Current Location";
    public static final String CURRENT_SPEED = "Current Speed";
    public static final String UNKNOWN_LOCATION = "Unknown Location";

    private static final long REPEAT_INTERVAL = 120000;

    private static final long FRESH_THRESHOLD = 90000;

    private static final int LOCATION_CHANGE = 1;

    // record time when the location provider is set
    private long mProviderSetTime;

    private Handler mHandler;
    private HandlerThread mHandlerThread;
    private AlarmManager mAlarmManager;
    private LocationManager mLocationManager;

    private ClusterManager mClusterManager;

    private Criteria mCriteria = new Criteria();

    private LocationUpdater mLocationUpdater;

    private Context mContext;
    private PendingIntent mPendingIntent;

    // Fake location, used for testing.
    private String mFakeLocation = null;

    public LocationStatsAggregator(final Context context) {
        mLocationManager =
            (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        mAlarmManager =
            (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);

        setClusteringThread(context);

        mCriteria.setAccuracy(Criteria.ACCURACY_COARSE);
        mCriteria.setPowerRequirement(Criteria.POWER_LOW);
        /*
        mCriteria.setAltitudeRequired(false);
        mCriteria.setBearingRequired(false);
        mCriteria.setSpeedRequired(true);
        */
        mCriteria.setCostAllowed(true);


        IntentFilter filter = new IntentFilter(LocationUpdater.LOCATION_UPDATE);
        mLocationUpdater = new LocationUpdater();
        context.registerReceiver(mLocationUpdater, filter);

        Intent intent = new Intent(LocationUpdater.LOCATION_UPDATE);

        mContext = context;
        mPendingIntent = PendingIntent.getBroadcast(mContext, 0, intent, 0);

        mAlarmManager.setInexactRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                                          SystemClock.elapsedRealtime() + 30000, //
                                          REPEAT_INTERVAL,
                                          mPendingIntent);
    }

    public void release() {
        mContext.unregisterReceiver(mLocationUpdater);
        mAlarmManager.cancel(mPendingIntent);
    }

    public String[] getListOfFeatures(){
        String[] list = { CURRENT_LOCATION } ;
        return list;
    }

    public Map<String,String> getFeatureValue(String featureName) {
        HashMap<String,String> feature = new HashMap<String,String>();

        if (featureName.equals(CURRENT_LOCATION)) {
            // TODO: check last known location first before sending out location request.
            /*
              Location location =
              mLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
            */
            String location = mClusterManager.getSemanticLocation();
            if (!location.equals(UNKNOWN_LOCATION)) {
                if (mFakeLocation != null) {
                    feature.put(CURRENT_LOCATION, mFakeLocation);
                } else {
                    feature.put(CURRENT_LOCATION, location);
                }
            }
        }
        return (Map) feature;
    }

    public List<String> getClusterNames() {
        return mClusterManager.getClusterNames();
    }

    // set a fake location using cluster name.
    // Set an empty string "" to disable the fake location
    public void setFakeLocation(String name) {
        if (name != null && name.length() != 0)
            mFakeLocation = name;
        else mFakeLocation = null;
    }

    private Location getLastKnownLocation() {
        List<String> providers = mLocationManager.getAllProviders();
        Location bestResult = null;
        float bestAccuracy = Float.MAX_VALUE;
        long bestTime;

        // get the latest location data
        long currTime =  System.currentTimeMillis();
        for (String provider : providers) {
            Location location = mLocationManager.getLastKnownLocation(provider);

            if (location != null) {
                float accuracy = location.getAccuracy();
                long time = location.getTime();

                if (currTime - time < FRESH_THRESHOLD && accuracy < bestAccuracy) {
                    bestResult = location;
                    bestAccuracy = accuracy;
                    bestTime = time;
                }
            }
        }
        if (bestResult != null) {
            Log.i(TAG, "found location for free: " + bestResult);
        }
        return bestResult;
    }

    private class LocationUpdater extends BroadcastReceiver {
        String TAG = "LocationUpdater";

        public static final String LOCATION_UPDATE = "android.bordeaux.services.LOCATION_UPDATE";

        @Override
        public void onReceive(Context context, Intent intent) {
            Location location = getLastKnownLocation();

            if (location == null) {
                String provider = mLocationManager.getBestProvider(mCriteria, true);
                Log.i(TAG, "Best Available Location Provider: " + provider);
                mLocationManager.requestSingleUpdate(provider, mLocationListener,
                                                     mHandlerThread.getLooper());
            } else {
                mHandler.sendMessage(mHandler.obtainMessage(LOCATION_CHANGE, location));
            }
        }
    }

    private void setClusteringThread(Context context) {
        mClusterManager = new ClusterManager(context);

        mHandlerThread = new HandlerThread("Location Handler",
                Process.THREAD_PRIORITY_BACKGROUND);
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper()) {

            @Override
            public void handleMessage(Message msg) {
                if (!(msg.obj instanceof Location)) {
                    return;
                }
                Location location = (Location) msg.obj;
                switch(msg.what) {
                    case LOCATION_CHANGE:
                        mClusterManager.addSample(location);
                        break;
                    default:
                        super.handleMessage(msg);
                }
            }
        };
    }

    private final LocationListener mLocationListener = new LocationListener() {
        private static final String TAG = "LocationListener";

        public void onLocationChanged(Location location) {
            mHandler.sendMessage(mHandler.obtainMessage(LOCATION_CHANGE, location));
            mLocationManager.removeUpdates(this);
        }

        public void onStatusChanged(String provider, int status, Bundle extras) { }

        public void onProviderEnabled(String provider) { }

        public void onProviderDisabled(String provider) { }
    };
}
