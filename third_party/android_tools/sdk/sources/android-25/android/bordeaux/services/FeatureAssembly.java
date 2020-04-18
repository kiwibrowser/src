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

import android.os.IBinder;
import android.util.Log;
import android.util.Pair;
import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.Iterator;

class FeatureAssembly {
    private static final String TAG = "FeatureAssembly";
    private List<String> mPossibleFeatures;
    private HashSet<String> mUseFeatures;
    private HashSet<Pair<String, String> > mUsePairedFeatures;
    private AggregatorManager mAggregatorManager;

    public FeatureAssembly() {
        mAggregatorManager = AggregatorManager.getInstance();
        mPossibleFeatures = Arrays.asList(mAggregatorManager.getListOfFeatures());
        mUseFeatures = new HashSet<String>();
        mUsePairedFeatures = new HashSet<Pair<String, String> >();
    }

    public boolean registerFeature(String s) {
        if (mPossibleFeatures.contains(s)) {
            mUseFeatures.add(s);
            return true;
        } else {
            return false;
        }
    }

    public boolean registerFeaturePair(String[] features) {
        if (features.length != 2 ||
            !mPossibleFeatures.contains(features[0]) ||
            !mPossibleFeatures.contains(features[1])) {
            return false;
        } else {
            mUseFeatures.add(features[0]);
            mUseFeatures.add(features[1]);
            mUsePairedFeatures.add(Pair.create(features[0], features[1]));
            return true;
        }
    }

    public Set<String> getUsedFeatures() {
        return (Set) mUseFeatures;
    }

    public Map<String, String> getFeatureMap() {
        HashMap<String, String> featureMap = new HashMap<String, String>();

        Iterator itr = mUseFeatures.iterator();
        while(itr.hasNext()) {
            String f = (String) itr.next();
            Map<String, String> features = mAggregatorManager.getDataMap(f);

            // TODO: sanity check for now.
            if (features.size() > 1) {
              throw new RuntimeException("Incorrect feature format extracted from aggregator.");
            }
            featureMap.putAll(features);
        }

        if (!mUsePairedFeatures.isEmpty()) {
            itr = mUsePairedFeatures.iterator();
            while(itr.hasNext()) {
                Pair<String, String> pair = (Pair<String, String>) itr.next();
                if (featureMap.containsKey(pair.first) &&
                    featureMap.containsKey(pair.second)) {
                    String key = pair.first + Predictor.FEATURE_SEPARATOR + pair.second;
                    String value = featureMap.get(pair.first) + Predictor.FEATURE_SEPARATOR +
                            featureMap.get(pair.second);
                    featureMap.put(key, value);
                }
            }
        }
        return (Map)featureMap;
    }


    public String augmentFeatureInputString(String s) {
        String fs = s;
        Iterator itr = mUseFeatures.iterator();
        while(itr.hasNext()) {
            String f = (String) itr.next();
            fs = fs + "+" + mAggregatorManager.getDataMap(f).get(f);
        }
        return fs;
    }
}
