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

import android.location.Location;
import android.text.format.Time;
import android.util.Log;

import java.lang.Math;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class LocationCluster extends BaseCluster {
    public static String TAG = "LocationCluster";

    private ArrayList<Location> mLocations = new ArrayList<Location>();
    private HashMap<String, Long> mNewHistogram = new HashMap<String, Long>();

    private String mSemanticClusterId = null;

    public void setSemanticClusterId(String semanticClusterId) {
        mSemanticClusterId = semanticClusterId;
    }

    public String getSemanticClusterId() {
        return mSemanticClusterId;
    }

    public boolean hasSemanticClusterId() {
        return mSemanticClusterId != null;
    }

    // TODO: make it a singleton class
    public LocationCluster(Location location, long duration) {
        super(location);
        addSample(location, duration);
    }

    public void addSample(Location location, long duration) {
        updateTemporalHistogram(location.getTime(), duration);

        // use time field to store duation of this location
        // TODO: extend Location class with additional field for this.
        location.setTime(duration);
        mLocations.add(location);
    }

    public void consolidate() {
        // If there is no new location added during this period, do nothing.
        if (mLocations.size() == 0) {
            return;
        }

        double[] newCenter = {0f, 0f, 0f};
        long newDuration = 0l;
        // update cluster center
        for (Location location : mLocations) {
            double[] vector = getLocationVector(location);
            long duration = location.getTime(); // in seconds

            if (duration == 0) {
                throw new RuntimeException("location duration is zero");
            }

            newDuration += duration;
            for (int i = 0; i < VECTOR_LENGTH; ++i) {
                newCenter[i] += vector[i] * duration;
            }
        }
        if (newDuration == 0l) {
            throw new RuntimeException("new duration is zero!");
        }
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            newCenter[i] /= newDuration;
        }
        // remove location data
        mLocations.clear();

        // The updated center is the weighted average of the existing and the new
        // centers. Note that if the cluster is consolidated for the first time,
        // the weight for the existing cluster would be zero.
        averageCenter(newCenter, newDuration);

        // update histogram
        for (Map.Entry<String, Long> entry : mNewHistogram.entrySet()) {
            String timeLabel = entry.getKey();
            long duration = entry.getValue();
            if (mHistogram.containsKey(timeLabel)) {
                duration += mHistogram.get(timeLabel);
            }
            mHistogram.put(timeLabel, duration);
        }
        mDuration += newDuration;
        mNewHistogram.clear();
    }

    /*
     * if the new created cluster whose covered area overlaps with any existing
     * cluster move the center away from that cluster till there is no overlap.
     */
    public void moveAwayCluster(LocationCluster cluster, float distance) {
        double[] vector = new double[VECTOR_LENGTH];

        double dot = 0f;
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            dot += mCenter[i] * cluster.mCenter[i];
        }
        double norm = 0f;
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            vector[i] = mCenter[i] - dot * cluster.mCenter[i];
            norm += vector[i] * vector[i];
        }
        norm = Math.sqrt(norm);

        double radian = distance / EARTH_RADIUS;
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            mCenter[i] = cluster.mCenter[i] * Math.cos(radian) +
                    (vector[i] / norm) * Math.sin(radian);
        }
    }

    private void updateTemporalHistogram(long time, long duration) {
        HashMap<String, String> timeFeatures = TimeStatsAggregator.getAllTimeFeatures(time);

        String timeOfWeek = timeFeatures.get(TimeStatsAggregator.TIME_OF_WEEK);
        long totalDuration = (mNewHistogram.containsKey(timeOfWeek)) ?
            mNewHistogram.get(timeOfWeek) + duration : duration;
        mNewHistogram.put(timeOfWeek, totalDuration);

        String timeOfDay = timeFeatures.get(TimeStatsAggregator.TIME_OF_DAY);
        totalDuration = (mNewHistogram.containsKey(timeOfDay)) ?
            mNewHistogram.get(timeOfDay) + duration : duration;
        mNewHistogram.put(timeOfDay, totalDuration);
    }
}
