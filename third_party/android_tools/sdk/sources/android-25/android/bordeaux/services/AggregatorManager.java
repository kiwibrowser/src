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


import android.bordeaux.services.StringString;
import android.content.Context;
import android.util.Log;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class AggregatorManager extends IAggregatorManager.Stub  {
    private final String TAG = "AggregatorMnager";
    // this maps from the aggregator name to the registered aggregator instance
    private static HashMap<String, Aggregator> mAggregators = new HashMap<String, Aggregator>();
    // this maps from the feature names to the aggregator that generates the feature
    private static HashMap<String, Aggregator> sFeatureMap = new HashMap<String, Aggregator>();
    private static AggregatorManager mManager = null;

    private String mFakeLocation = null;
    private String mFakeTimeOfDay = null;
    private String mFakeDayOfWeek = null;

    private AggregatorManager() {
        sFeatureMap = new HashMap<String, Aggregator>();
    }

    public static AggregatorManager getInstance() {
        if (mManager == null )
            mManager = new AggregatorManager();
        return mManager;
    }

    public String[] getListOfFeatures() {
        String[] s = new String[sFeatureMap.size()];
        int i = 0;
        for (Map.Entry<String, Aggregator> x : sFeatureMap.entrySet()) {
           s[i] = x.getKey();
           i++;
        }
        return s;
    }

    public void registerAggregator(Aggregator agg, AggregatorManager m) {
        if (mAggregators.get(agg.getClass().getName()) != null) {
            // only one instance
            // throw new RuntimeException("Can't register more than one instance");
        }
        mAggregators.put(agg.getClass().getName(), agg);
        agg.setManager(m);
        String[] fl = agg.getListOfFeatures();
        for ( int i  = 0; i< fl.length; i ++)
            sFeatureMap.put(fl[i], agg);
    }

    // Start of IAggregatorManager interface
    public ArrayList<StringString> getData(String dataName) {
        return getList(getDataMap(dataName));
    }

    public List<String> getLocationClusters() {
        LocationStatsAggregator agg = (LocationStatsAggregator)
                mAggregators.get(LocationStatsAggregator.class.getName());
        if (agg == null) return new ArrayList<String> ();
        return agg.getClusterNames();
    }

    public List<String> getTimeOfDayValues() {
        TimeStatsAggregator agg = (TimeStatsAggregator)
                mAggregators.get(TimeStatsAggregator.class.getName());
        if (agg == null) return new ArrayList<String>();
        return agg.getTimeOfDayValues();
    }

    public List<String> getDayOfWeekValues() {
        TimeStatsAggregator agg = (TimeStatsAggregator)
                mAggregators.get(TimeStatsAggregator.class.getName());
        if (agg == null) return new ArrayList<String>();
        return agg.getDayOfWeekValues();
    }

    // Set an empty string "" to disable the fake location
    public boolean setFakeLocation(String location) {
        LocationStatsAggregator agg = (LocationStatsAggregator)
                mAggregators.get(LocationStatsAggregator.class.getName());
        if (agg == null) return false;
        agg.setFakeLocation(location);
        mFakeLocation = location;
        return true;
    }

    // Set an empty string "" to disable the fake time of day
    public boolean setFakeTimeOfDay(String time_of_day) {
        TimeStatsAggregator agg = (TimeStatsAggregator)
                mAggregators.get(TimeStatsAggregator.class.getName());
        if (agg == null) return false;
        agg.setFakeTimeOfDay(time_of_day);
        mFakeTimeOfDay = time_of_day;
        return true;
    }

    // Set an empty string "" to disable the fake day of week
    public boolean setFakeDayOfWeek(String day_of_week) {
        TimeStatsAggregator agg = (TimeStatsAggregator)
                mAggregators.get(TimeStatsAggregator.class.getName());
        if (agg == null) return false;
        agg.setFakeDayOfWeek(day_of_week);
        mFakeDayOfWeek = day_of_week;
        return true;
    }

    // Get the current mode, if fake mode return true
    public boolean getFakeMode() {
        boolean fakeMode = false;
        // checking any features that are in the fake mode
        if (mFakeLocation != null && mFakeLocation.length() != 0)
            fakeMode = true;
        if (mFakeTimeOfDay != null && mFakeTimeOfDay.length() != 0)
            fakeMode = true;
        if (mFakeDayOfWeek != null && mFakeDayOfWeek.length() != 0)
            fakeMode = true;
        return fakeMode;
    }
    // End of IAggregatorManger interface

    public Map<String, String> getDataMap(String dataName) {
        if (sFeatureMap.get(dataName) != null)
            return sFeatureMap.get(dataName).getFeatureValue(dataName);
        else
            Log.e(TAG, "There is no feature called " + dataName);
        return null;
    }

    private ArrayList<StringString> getList(final Map<String, String> sample) {
        ArrayList<StringString> StringString_sample = new ArrayList<StringString>();
        for (Map.Entry<String, String> x : sample.entrySet()) {
           StringString v = new StringString();
           v.key = x.getKey();
           v.value = x.getValue();
           StringString_sample.add(v);
        }
        return StringString_sample;
    }
}
