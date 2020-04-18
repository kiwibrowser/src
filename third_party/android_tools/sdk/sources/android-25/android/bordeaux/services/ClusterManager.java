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

import android.content.Context;
import android.location.Location;
import android.text.format.Time;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

/**
 * ClusterManager incrementally indentify representitve clusters from the input location
 * stream. Clusters are updated online using leader based clustering algorithm. The input
 * locations initially are kept by the clusters. Periodially, a cluster consolidating
 * procedure is carried out to refine the cluster centers. After consolidation, the
 * location data are released.
 */
public class ClusterManager {

    private static String TAG = "ClusterManager";

    private static float LOCATION_CLUSTER_RADIUS = 25; // meter

    private static float SEMANTIC_CLUSTER_RADIUS = 75; // meter

    // Consoliate location clusters (and check for new semantic clusters)
    // every 10 minutes (600 seconds).
    private static final long CONSOLIDATE_INTERVAL = 600;

    // A location cluster can be labeled as a semantic cluster if it has been
    // stayed for at least 10 minutes (600 seconds) within a day.
    private static final long SEMANTIC_CLUSTER_THRESHOLD = 600; // seconds

    // Reset location cluters every 24 hours (86400 seconds).
    private static final long LOCATION_REFRESH_PERIOD = 86400; // seconds

    private static String UNKNOWN_LOCATION = "Unknown Location";

    private Location mLastLocation = null;

    private long mClusterDuration;

    private long mConsolidateRef = 0;

    private long mRefreshRef = 0;

    private long mSemanticClusterCount = 0;

    private ArrayList<LocationCluster> mLocationClusters = new ArrayList<LocationCluster>();

    private ArrayList<BaseCluster> mSemanticClusters = new ArrayList<BaseCluster>();

    private AggregatorRecordStorage mStorage;

    private static String SEMANTIC_TABLE = "SemanticTable";

    private static String SEMANTIC_ID = "ID";

    private static final String SEMANTIC_LONGITUDE = "Longitude";

    private static final String SEMANTIC_LATITUDE = "Latitude";

    private static final String SEMANTIC_DURATION = "Duration";

    private static final String[] SEMANTIC_COLUMNS =
        new String[]{ SEMANTIC_ID,
                      SEMANTIC_LONGITUDE,
                      SEMANTIC_LATITUDE,
                      SEMANTIC_DURATION,
                      TimeStatsAggregator.WEEKEND,
                      TimeStatsAggregator.WEEKDAY,
                      TimeStatsAggregator.MORNING,
                      TimeStatsAggregator.NOON,
                      TimeStatsAggregator.AFTERNOON,
                      TimeStatsAggregator.EVENING,
                      TimeStatsAggregator.NIGHT,
                      TimeStatsAggregator.LATENIGHT };

    private static final int mFeatureValueStart = 4;
    private static final int mFeatureValueEnd = 11;

    public ClusterManager(Context context) {
        mStorage = new AggregatorRecordStorage(context, SEMANTIC_TABLE, SEMANTIC_COLUMNS);

        loadSemanticClusters();
    }

    public void addSample(Location location) {
        float bestClusterDistance = Float.MAX_VALUE;
        int bestClusterIndex = -1;
        long lastDuration;
        long currentTime = location.getTime() / 1000; // measure time in seconds

        if (mLastLocation != null) {
            if (location.getTime() == mLastLocation.getTime()) {
                return;
            }
            // get the duration spent in the last location
            long duration = (location.getTime() - mLastLocation.getTime()) / 1000;
            mClusterDuration += duration;

            Log.v(TAG, "sample duration: " + duration +
                  ", number of clusters: " + mLocationClusters.size());

            synchronized (mLocationClusters) {
                // add the last location to cluster.
                // first find the cluster it belongs to.
                for (int i = 0; i < mLocationClusters.size(); ++i) {
                    float distance = mLocationClusters.get(i).distanceToCenter(mLastLocation);
                    Log.v(TAG, "clulster " + i + " is within " + distance + " meters");
                    if (distance < bestClusterDistance) {
                        bestClusterDistance = distance;
                        bestClusterIndex = i;
                    }
                }

                // add the location to the selected cluster
                if (bestClusterDistance < LOCATION_CLUSTER_RADIUS) {
                    mLocationClusters.get(bestClusterIndex).addSample(mLastLocation, duration);
                } else {
                    // if it is far away from all existing clusters, create a new cluster.
                  LocationCluster cluster = new LocationCluster(mLastLocation, duration);
                  mLocationClusters.add(cluster);
                }
            }
        } else {
            mConsolidateRef = currentTime;
            mRefreshRef = currentTime;

            if (mLocationClusters.isEmpty()) {
                mClusterDuration = 0;
            }
        }

        long collectDuration = currentTime - mConsolidateRef;
        Log.v(TAG, "collect duration: " + collectDuration);
        if (collectDuration > CONSOLIDATE_INTERVAL) {
            // TODO : conslidation takes time. move this to a separate thread later.
            consolidateClusters();
            mConsolidateRef = currentTime;

            long refreshDuration = currentTime - mRefreshRef;
            Log.v(TAG, "refresh duration: " + refreshDuration);
            if (refreshDuration >  LOCATION_REFRESH_PERIOD) {
                updateSemanticClusters();
                mRefreshRef = currentTime;
            }
            saveSemanticClusters();
        }

        mLastLocation = location;
    }

    private void consolidateClusters() {
        synchronized (mSemanticClusters) {
            LocationCluster locationCluster;
            for (int i = mLocationClusters.size() - 1; i >= 0; --i) {
                locationCluster = mLocationClusters.get(i);
                locationCluster.consolidate();
            }

            // merge clusters whose regions are overlapped. note that after merge
            // cluster center changes but cluster size remains unchanged.
            for (int i = mLocationClusters.size() - 1; i >= 0; --i) {
                locationCluster = mLocationClusters.get(i);
                for (int j = i - 1; j >= 0; --j) {
                    float distance =
                        mLocationClusters.get(j).distanceToCluster(locationCluster);
                    if (distance < LOCATION_CLUSTER_RADIUS) {
                        mLocationClusters.get(j).absorbCluster(locationCluster);
                        mLocationClusters.remove(locationCluster);
                    }
                }
            }
            Log.v(TAG, mLocationClusters.size() + " location clusters after consolidate");

            // assign each candidate to a semantic cluster and check if new semantic
            // clusters are found
            for (LocationCluster candidate : mLocationClusters) {
                if (candidate.hasSemanticId() ||
                    candidate.hasSemanticClusterId() ||
                    !candidate.passThreshold(SEMANTIC_CLUSTER_THRESHOLD)) {
                    continue;
                }

                // find the closest semantic cluster
                float bestClusterDistance = Float.MAX_VALUE;
                String bestClusterId = "Unused Id";
                for (BaseCluster cluster : mSemanticClusters) {
                    float distance = cluster.distanceToCluster(candidate);
                    Log.v(TAG, distance + "distance to semantic cluster: " +
                          cluster.getSemanticId());

                    if (distance < bestClusterDistance) {
                        bestClusterDistance = distance;
                        bestClusterId = cluster.getSemanticId();
                    }
                }

                // if candidate doesn't belong to any semantic cluster, create a new
                // semantic cluster
                if (bestClusterDistance > SEMANTIC_CLUSTER_RADIUS) {
                    candidate.generateSemanticId(mSemanticClusterCount++);
                    mSemanticClusters.add(candidate);
                } else {
                    candidate.setSemanticClusterId(bestClusterId);
                }
            }
            Log.v(TAG, mSemanticClusters.size() + " semantic clusters after consolidate");
        }
    }

    private void updateSemanticClusters() {
        synchronized (mSemanticClusters) {
            // create index to cluster map
            HashMap<String, BaseCluster> semanticIdMap =
                new HashMap<String, BaseCluster>();
            for (BaseCluster cluster : mSemanticClusters) {
                // TODO: apply forgetting factor on existing semantic cluster stats,
                // duration, histogram, etc.
                cluster.forgetPastHistory();
                semanticIdMap.put(cluster.getSemanticId(), cluster);
            }

            // assign each candidate to a semantic cluster
            for (LocationCluster cluster : mLocationClusters) {
                if (cluster.hasSemanticClusterId()) {
                    BaseCluster semanticCluster =
                        semanticIdMap.get(cluster.getSemanticClusterId());
                    semanticCluster.absorbCluster(cluster);
                }
            }
            // reset location clusters.
            mLocationClusters.clear();
        }
    }

    private void loadSemanticClusters() {
        List<Map<String, String> > allData = mStorage.getAllData();
        HashMap<String, Long> histogram = new HashMap<String, Long>();

        synchronized (mSemanticClusters) {
            mSemanticClusters.clear();
            for (Map<String, String> map : allData) {
                String semanticId = map.get(SEMANTIC_ID);
                double longitude = Double.valueOf(map.get(SEMANTIC_LONGITUDE));
                double latitude = Double.valueOf(map.get(SEMANTIC_LATITUDE));
                long duration = Long.valueOf(map.get(SEMANTIC_DURATION));
                BaseCluster cluster =
                    new BaseCluster(semanticId, longitude, latitude, duration);

                histogram.clear();
                for (int i = mFeatureValueStart; i <= mFeatureValueEnd; i++) {
                    String featureValue = SEMANTIC_COLUMNS[i];
                    if (map.containsKey(featureValue)) {
                      histogram.put(featureValue, Long.valueOf(map.get(featureValue)));
                    }
                }
                cluster.setHistogram(histogram);
                mSemanticClusters.add(cluster);
            }
            mSemanticClusterCount = mSemanticClusters.size();
            Log.e(TAG, "load " + mSemanticClusterCount + " semantic clusters.");
        }
    }

    private void saveSemanticClusters() {
        HashMap<String, String> rowFeatures = new HashMap<String, String>();

        mStorage.removeAllData();
        synchronized (mSemanticClusters) {
            for (BaseCluster cluster : mSemanticClusters) {
                rowFeatures.clear();
                rowFeatures.put(SEMANTIC_ID, cluster.getSemanticId());

                rowFeatures.put(SEMANTIC_LONGITUDE,
                            String.valueOf(cluster.getCenterLongitude()));
                rowFeatures.put(SEMANTIC_LATITUDE,
                            String.valueOf(cluster.getCenterLatitude()));
                rowFeatures.put(SEMANTIC_DURATION,
                            String.valueOf(cluster.getDuration()));

                HashMap<String, Long> histogram = cluster.getHistogram();
                for (Map.Entry<String, Long> entry : histogram.entrySet()) {
                    rowFeatures.put(entry.getKey(), String.valueOf(entry.getValue()));
                }
                mStorage.addData(rowFeatures);
                Log.e(TAG, "saving semantic cluster: " + rowFeatures);
            }
        }
    }

    public String getSemanticLocation() {
        String label = LocationStatsAggregator.UNKNOWN_LOCATION;

        // instead of using the last location, try acquiring the latest location.
        if (mLastLocation != null) {
            // TODO: use fast neatest neighbor search speed up location search
            synchronized (mSemanticClusters) {
                for (BaseCluster cluster: mSemanticClusters) {
                    if (cluster.distanceToCenter(mLastLocation) < SEMANTIC_CLUSTER_RADIUS) {
                        return cluster.getSemanticId();
                    }
                }
            }
        }
        return label;
    }

    public List<String> getClusterNames() {
        ArrayList<String> clusters = new ArrayList<String>();
        synchronized (mSemanticClusters) {
            for (BaseCluster cluster: mSemanticClusters) {
                clusters.add(cluster.getSemanticId());
            }
        }
        return clusters;
    }
}
