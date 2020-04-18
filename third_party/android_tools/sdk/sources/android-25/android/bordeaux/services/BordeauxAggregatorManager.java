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

import android.bordeaux.services.IAggregatorManager;
import android.bordeaux.services.StringString;
import android.content.Context;
import android.os.RemoteException;
import android.util.Log;
import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/** AggregatorManger for Learning framework.
 */
public class BordeauxAggregatorManager {
    static final String TAG = "BordeauxAggregatorManager";
    static final String AggregatorManager_NOTAVAILABLE = "AggregatorManager not Available";
    private Context mContext;
    private IAggregatorManager mAggregatorManager;

    public boolean retrieveAggregatorManager() {
        if (mAggregatorManager == null) {
            mAggregatorManager = BordeauxManagerService.getAggregatorManager(mContext);
            if (mAggregatorManager == null) {
                Log.e(TAG, AggregatorManager_NOTAVAILABLE);
                return false;
            }
        }
        return true;
    }

    public BordeauxAggregatorManager (Context context) {
        mContext = context;
        mAggregatorManager = BordeauxManagerService.getAggregatorManager(mContext);
    }

    public Map<String, String> GetData(final String dataName) {
        if (!retrieveAggregatorManager())
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        try {
            return getMap(mAggregatorManager.getData(dataName));
        } catch (RemoteException e) {
            Log.e(TAG,"Exception in Getting " + dataName);
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        }
    }

    public List<String> getLocationClusters() {
        if (!retrieveAggregatorManager())
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        try {
            return mAggregatorManager.getLocationClusters();
        } catch (RemoteException e) {
            Log.e(TAG,"Error getting location clusters");
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        }
    }

    public List<String> getTimeOfDayValues() {
        if (!retrieveAggregatorManager())
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        try {
            return mAggregatorManager.getTimeOfDayValues();
        } catch (RemoteException e) {
            Log.e(TAG,"Error getting time of day values");
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        }
    }

    public List<String> getDayOfWeekValues() {
        if (!retrieveAggregatorManager())
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        try {
            return mAggregatorManager.getDayOfWeekValues();
        } catch (RemoteException e) {
            Log.e(TAG,"Error getting day of week values");
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        }
    }

    public boolean setFakeLocation(final String name) {
        if (!retrieveAggregatorManager())
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        try {
            return mAggregatorManager.setFakeLocation(name);
        } catch (RemoteException e) {
            Log.e(TAG,"Error setting fake location:" + name);
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        }
    }

    public boolean setFakeTimeOfDay(final String time_of_day) {
        if (!retrieveAggregatorManager())
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        try {
            return mAggregatorManager.setFakeTimeOfDay(time_of_day);
        } catch (RemoteException e) {
            Log.e(TAG,"Error setting fake time of day:" + time_of_day);
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        }
    }

    public boolean setFakeDayOfWeek(final String day_of_week) {
        if (!retrieveAggregatorManager())
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        try {
            return mAggregatorManager.setFakeDayOfWeek(day_of_week);
        } catch (RemoteException e) {
            Log.e(TAG,"Error setting fake day of week:" + day_of_week);
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        }
    }

    public boolean getFakeMode() {
        if (!retrieveAggregatorManager())
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        try {
            return mAggregatorManager.getFakeMode();
        } catch (RemoteException e) {
            Log.e(TAG,"Error getting fake mode");
            throw new RuntimeException(AggregatorManager_NOTAVAILABLE);
        }
    }

    private Map<String, String> getMap(final List<StringString> sample) {
        HashMap<String, String> map = new HashMap<String, String>();
        for (int i =0; i < sample.size(); i++) {
            map.put(sample.get(i).key, sample.get(i).value);
        }
        return (Map) map;
    }
}
