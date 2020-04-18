/*
 * Copyright (C) 2011 The Android Open Source Project
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

package android.bordeaux.learning;

import android.util.Log;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;
/**
 * A histogram based predictor which records co-occurrences of applations with a speficic
 * feature, for example, location, * time of day, etc. The histogram is kept in a two level
 * hash table. The first level key is the feature value and the second level key is the app
 * id.
 */
// TODOS:
// 1. Use forgetting factor to downweight istances propotional to the time
// 2. Different features could have different weights on prediction scores.
// 3. Add function to remove sampleid (i.e. remove apps that are uninstalled).


public class HistogramPredictor {
    final static String TAG = "HistogramPredictor";

    private HashMap<String, HistogramCounter> mPredictor =
            new HashMap<String, HistogramCounter>();

    private HashMap<String, Integer> mClassCounts = new HashMap<String, Integer>();
    private HashSet<String> mBlacklist = new HashSet<String>();

    private static final int MINIMAL_FEATURE_VALUE_COUNTS = 5;
    private static final int MINIMAL_APP_APPEARANCE_COUNTS = 5;

    // This parameter ranges from 0 to 1 which determines the effect of app prior.
    // When it is set to 0, app prior means completely neglected. When it is set to 1
    // the predictor is a standard naive bayes model.
    private static final int PRIOR_K_VALUE = 1;

    private static final String[] APP_BLACKLIST = {
        "com.android.contacts",
        "com.android.chrome",
        "com.android.providers.downloads.ui",
        "com.android.settings",
        "com.android.vending",
        "com.android.mms",
        "com.google.android.gm",
        "com.google.android.gallery3d",
        "com.google.android.apps.googlevoice",
    };

    public HistogramPredictor(String[] blackList) {
        for (String appName : blackList) {
            mBlacklist.add(appName);
        }
    }

    /*
     * This class keeps the histogram counts for each feature and provide the
     * joint probabilities of <feature, class>.
     */
    private class HistogramCounter {
        private HashMap<String, HashMap<String, Integer> > mCounter =
                new HashMap<String, HashMap<String, Integer> >();

        public HistogramCounter() {
            mCounter.clear();
        }

        public void setCounter(HashMap<String, HashMap<String, Integer> > counter) {
            resetCounter();
            mCounter.putAll(counter);
        }

        public void resetCounter() {
            mCounter.clear();
        }

        public void addSample(String className, String featureValue) {
            HashMap<String, Integer> classCounts;

            if (!mCounter.containsKey(featureValue)) {
                classCounts = new HashMap<String, Integer>();
                mCounter.put(featureValue, classCounts);
            } else {
                classCounts = mCounter.get(featureValue);
            }
            int count = (classCounts.containsKey(className)) ?
                    classCounts.get(className) + 1 : 1;
            classCounts.put(className, count);
        }

        public HashMap<String, Double> getClassScores(String featureValue) {
            HashMap<String, Double> classScores = new HashMap<String, Double>();

            if (mCounter.containsKey(featureValue)) {
                int totalCount = 0;
                for(Map.Entry<String, Integer> entry :
                        mCounter.get(featureValue).entrySet()) {
                    String app = entry.getKey();
                    int count = entry.getValue();

                    // For apps with counts less than or equal to one, we treated
                    // those as having count one. Hence their score, i.e. log(count)
                    // would be zero. classScroes stores only apps with non-zero scores.
                    // Note that totalCount also neglect app with single occurrence.
                    if (count > 1) {
                        double score = Math.log((double) count);
                        classScores.put(app, score);
                        totalCount += count;
                    }
                }
                if (totalCount < MINIMAL_FEATURE_VALUE_COUNTS) {
                    classScores.clear();
                }
            }
            return classScores;
        }

        public byte[] getModel() {
            try {
                ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
                ObjectOutputStream objStream = new ObjectOutputStream(byteStream);
                synchronized(mCounter) {
                    objStream.writeObject(mCounter);
                }
                byte[] bytes = byteStream.toByteArray();
                return bytes;
            } catch (IOException e) {
                throw new RuntimeException("Can't get model");
            }
        }

        public boolean setModel(final byte[] modelData) {
            mCounter.clear();
            HashMap<String, HashMap<String, Integer> > model;

            try {
                ByteArrayInputStream input = new ByteArrayInputStream(modelData);
                ObjectInputStream objStream = new ObjectInputStream(input);
                model = (HashMap<String, HashMap<String, Integer> >) objStream.readObject();
            } catch (IOException e) {
                throw new RuntimeException("Can't load model");
            } catch (ClassNotFoundException e) {
                throw new RuntimeException("Learning class not found");
            }

            synchronized(mCounter) {
                mCounter.putAll(model);
            }

            return true;
        }


        public HashMap<String, HashMap<String, Integer> > getCounter() {
            return mCounter;
        }

        public String toString() {
            String result = "";
            for (Map.Entry<String, HashMap<String, Integer> > entry :
                     mCounter.entrySet()) {
                result += "{ " + entry.getKey() + " : " +
                    entry.getValue().toString() + " }";
            }
            return result;
        }
    }

    /*
     * Given a map of feature name -value pairs returns topK mostly likely apps to
     * be launched with corresponding likelihoods. If topK is set zero, it will return
     * the whole list.
     */
    public List<Map.Entry<String, Double> > findTopClasses(Map<String, String> features, int topK) {
        // Most sophisticated function in this class
        HashMap<String, Double> appScores = new HashMap<String, Double>();
        int validFeatureCount = 0;

        // compute all app scores
        for (Map.Entry<String, HistogramCounter> entry : mPredictor.entrySet()) {
            String featureName = entry.getKey();
            HistogramCounter counter = entry.getValue();

            if (features.containsKey(featureName)) {
                String featureValue = features.get(featureName);
                HashMap<String, Double> scoreMap = counter.getClassScores(featureValue);

                if (scoreMap.isEmpty()) {
                  continue;
                }
                validFeatureCount++;

                for (Map.Entry<String, Double> item : scoreMap.entrySet()) {
                    String appName = item.getKey();
                    double appScore = item.getValue();
                    if (appScores.containsKey(appName)) {
                        appScore += appScores.get(appName);
                    }
                    appScores.put(appName, appScore);
                }
            }
        }

        HashMap<String, Double> appCandidates = new HashMap<String, Double>();
        for (Map.Entry<String, Double> entry : appScores.entrySet()) {
            String appName = entry.getKey();
            if (mBlacklist.contains(appName)) {
                Log.i(TAG, appName + " is in blacklist");
                continue;
            }
            if (!mClassCounts.containsKey(appName)) {
                throw new RuntimeException("class count error!");
            }
            int appCount = mClassCounts.get(appName);
            if (appCount < MINIMAL_APP_APPEARANCE_COUNTS) {
                Log.i(TAG, appName + " doesn't have enough counts");
                continue;
            }

            double appScore = entry.getValue();
            double appPrior = Math.log((double) appCount);
            appCandidates.put(appName,
                              appScore - appPrior * (validFeatureCount - PRIOR_K_VALUE));
        }

        // sort app scores
        List<Map.Entry<String, Double> > appList =
               new ArrayList<Map.Entry<String, Double> >(appCandidates.size());
        appList.addAll(appCandidates.entrySet());
        Collections.sort(appList, new  Comparator<Map.Entry<String, Double> >() {
            public int compare(Map.Entry<String, Double> o1,
                               Map.Entry<String, Double> o2) {
                return o2.getValue().compareTo(o1.getValue());
            }
        });

        if (topK == 0) {
            topK = appList.size();
        }
        return appList.subList(0, Math.min(topK, appList.size()));
    }

    /*
     * Add a new observation of given sample id and features to the histograms
     */
    public void addSample(String sampleId, Map<String, String> features) {
        for (Map.Entry<String, String> entry : features.entrySet()) {
            String featureName = entry.getKey();
            String featureValue = entry.getValue();

            useFeature(featureName);
            HistogramCounter counter = mPredictor.get(featureName);
            counter.addSample(sampleId, featureValue);
        }

        int sampleCount = (mClassCounts.containsKey(sampleId)) ?
            mClassCounts.get(sampleId) + 1 : 1;
        mClassCounts.put(sampleId, sampleCount);
    }

    /*
     * reset predictor to a empty model
     */
    public void resetPredictor() {
        // TODO: not sure this step would reduce memory waste
        for (HistogramCounter counter : mPredictor.values()) {
            counter.resetCounter();
        }
        mPredictor.clear();
        mClassCounts.clear();
    }

    /*
     * convert the prediction model into a byte array
     */
    public byte[] getModel() {
        // TODO: convert model to a more memory efficient data structure.
        HashMap<String, HashMap<String, HashMap<String, Integer > > > model =
                new HashMap<String, HashMap<String, HashMap<String, Integer > > >();
        for(Map.Entry<String, HistogramCounter> entry : mPredictor.entrySet()) {
            model.put(entry.getKey(), entry.getValue().getCounter());
        }

        try {
            ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
            ObjectOutputStream objStream = new ObjectOutputStream(byteStream);
            objStream.writeObject(model);
            byte[] bytes = byteStream.toByteArray();
            return bytes;
        } catch (IOException e) {
            throw new RuntimeException("Can't get model");
        }
    }

    /*
     * set the prediction model from a model data in the format of byte array
     */
    public boolean setModel(final byte[] modelData) {
        HashMap<String, HashMap<String, HashMap<String, Integer > > > model;

        try {
            ByteArrayInputStream input = new ByteArrayInputStream(modelData);
            ObjectInputStream objStream = new ObjectInputStream(input);
            model = (HashMap<String, HashMap<String, HashMap<String, Integer > > >)
                    objStream.readObject();
        } catch (IOException e) {
            throw new RuntimeException("Can't load model");
        } catch (ClassNotFoundException e) {
            throw new RuntimeException("Learning class not found");
        }

        resetPredictor();
        for (Map.Entry<String, HashMap<String, HashMap<String, Integer> > > entry :
                model.entrySet()) {
            useFeature(entry.getKey());
            mPredictor.get(entry.getKey()).setCounter(entry.getValue());
        }

        // TODO: this is a temporary fix for now
        loadClassCounter();

        return true;
    }

    private void loadClassCounter() {
        String TIME_OF_WEEK = "Time of Week";

        if (!mPredictor.containsKey(TIME_OF_WEEK)) {
            throw new RuntimeException("Precition model error: missing Time of Week!");
        }

        HashMap<String, HashMap<String, Integer> > counter =
            mPredictor.get(TIME_OF_WEEK).getCounter();

        mClassCounts.clear();
        for (HashMap<String, Integer> map : counter.values()) {
            for (Map.Entry<String, Integer> entry : map.entrySet()) {
                int classCount = entry.getValue();
                String className = entry.getKey();
                // mTotalClassCount += classCount;

                if (mClassCounts.containsKey(className)) {
                    classCount += mClassCounts.get(className);
                }
                mClassCounts.put(className, classCount);
            }
        }
        Log.i(TAG, "class counts: " + mClassCounts);
    }

    private void useFeature(String featureName) {
        if (!mPredictor.containsKey(featureName)) {
            mPredictor.put(featureName, new HistogramCounter());
        }
    }
}
