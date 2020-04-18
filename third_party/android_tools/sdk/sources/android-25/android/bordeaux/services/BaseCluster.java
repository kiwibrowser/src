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

public class BaseCluster {

    public static String TAG = "BaseCluster";

    public double[] mCenter;
  // protected double[] mCenter;

    protected static final int VECTOR_LENGTH = 3;

    private static final long FORGETTING_ENUMERATOR = 95;
    private static final long FORGETTING_DENOMINATOR = 100;

    // Histogram illustrates the pattern of visit during time of day,
    protected HashMap<String, Long> mHistogram = new HashMap<String, Long>();

    protected long mDuration;

    protected String mSemanticId;

    protected static final double EARTH_RADIUS = 6378100f;

    public BaseCluster(Location location) {
        mCenter = getLocationVector(location);
        mDuration = 0;
    }

    public BaseCluster(String semanticId, double longitude, double latitude,
                     long duration) {
        mSemanticId = semanticId;
        mCenter = getLocationVector(longitude, latitude);
        mDuration = duration;
    }

    public String getSemanticId() {
        return mSemanticId;
    }

    public void generateSemanticId(long index) {
        mSemanticId = "cluser: " + String.valueOf(index);
    }

    public long getDuration() {
        return mDuration;
    }

    public boolean hasSemanticId() {
        return mSemanticId != null;
    }

    protected double[] getLocationVector(Location location) {
        return getLocationVector(location.getLongitude(), location.getLatitude());
    }

    protected double[] getLocationVector(double longitude, double latitude) {
        double vector[] = new double[VECTOR_LENGTH];
        double lambda = Math.toRadians(longitude);
        double phi = Math.toRadians(latitude);

        vector[0] = Math.cos(lambda) * Math.cos(phi);
        vector[1] = Math.sin(lambda) * Math.cos(phi);
        vector[2] = Math.sin(phi);
        return vector;
    }

    protected double getCenterLongitude() {
        // Because latitude ranges from -90 to 90 degrees, cosPhi >= 0.
        double cosPhi = Math.cos(Math.asin(mCenter[2]));
        double longitude = Math.toDegrees(Math.asin(mCenter[1] / cosPhi));
        if (mCenter[0] < 0) {
            longitude = (longitude > 0) ? 180f - longitude : -180 - longitude;
        }
        return longitude;
    }

    protected double getCenterLatitude() {
        return Math.toDegrees(Math.asin(mCenter[2]));
    }

    private double computeDistance(double[] vector1, double[] vector2) {
        double product = 0f;
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            product += vector1[i] * vector2[i];
        }
        double radian = Math.acos(Math.min(product, 1f));
        return radian * EARTH_RADIUS;
    }

    /*
     * This computes the distance from loation to the cluster center in meter.
     */
    public float distanceToCenter(Location location) {
        return (float) computeDistance(mCenter, getLocationVector(location));
    }

    public float distanceToCluster(BaseCluster cluster) {
        return (float) computeDistance(mCenter, cluster.mCenter);
    }

    public void absorbCluster(BaseCluster cluster) {
        averageCenter(cluster.mCenter, cluster.mDuration);
        absorbHistogram(cluster);
    }

    public void setCluster(BaseCluster cluster) {
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            mCenter[i] = cluster.mCenter[i];
        }
        mHistogram.clear();
        mHistogram.putAll(cluster.mHistogram);
        mDuration = cluster.mDuration;
    }

    private void absorbHistogram(BaseCluster cluster) {
        for (Map.Entry<String, Long> entry : cluster.mHistogram.entrySet()) {
            String timeLabel = entry.getKey();
            long duration = entry.getValue();

            if (mHistogram.containsKey(timeLabel)) {
                duration += mHistogram.get(timeLabel);
            }
            mHistogram.put(timeLabel, duration);
        }
        mDuration += cluster.mDuration;
    }

    public boolean passThreshold(long durationThreshold) {
        // TODO: might want to keep semantic cluster
        return mDuration > durationThreshold;
    }

    public final HashMap<String, Long> getHistogram() {
        return mHistogram;
    }

    public void setHistogram(Map<String, Long> histogram) {
        mHistogram.clear();
        mHistogram.putAll(histogram);

        mDuration = 0;
        if (mHistogram.containsKey(TimeStatsAggregator.WEEKEND)) {
            mDuration += mHistogram.get(TimeStatsAggregator.WEEKEND);
        }
        if (mHistogram.containsKey(TimeStatsAggregator.WEEKDAY)) {
            mDuration += mHistogram.get(TimeStatsAggregator.WEEKDAY);
        }
    }

    public void forgetPastHistory() {
        mDuration *= FORGETTING_ENUMERATOR;
        mDuration /= FORGETTING_DENOMINATOR;

        for (Map.Entry<String, Long> entry : mHistogram.entrySet()) {
            String key = entry.getKey();
            long value = entry.getValue();

            value *= FORGETTING_ENUMERATOR;
            value /= FORGETTING_DENOMINATOR;

            mHistogram.put(key, value);
        }
    }

    protected void normalizeCenter() {
        double norm = 0;
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            norm += mCenter[i] * mCenter[i];
        }
        norm = Math.sqrt(norm);
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            mCenter[i] /= norm;
        }
    }

    protected void averageCenter(double[] newCenter, long newDuration) {
        double weight = ((double) mDuration) / (mDuration + newDuration);
        double newWeight = 1f - weight;

        double norm = 0;
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            mCenter[i] = weight * mCenter[i] + newWeight * newCenter[i];
            norm += mCenter[i] * mCenter[i];
        }
        norm = Math.sqrt(norm);
        for (int i = 0; i < VECTOR_LENGTH; ++i) {
            mCenter[i] /= norm;
        }
    }
}
