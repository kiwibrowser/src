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

import android.util.Log;
import java.util.HashMap;
import java.util.Map;

public class MotionStatsAggregator extends Aggregator {
    final String TAG = "MotionStatsAggregator";
    public static final String CURRENT_MOTION = "Current Motion";
    public String[] getListOfFeatures(){
        String [] list = new String[1];
        list[0] = CURRENT_MOTION;
        return list;
    }
    public Map<String,String> getFeatureValue(String featureName) {
        HashMap<String,String> m = new HashMap<String,String>();
        if (featureName.equals(CURRENT_MOTION))
            m.put(CURRENT_MOTION,"Running"); //TODO maybe use clustering for user motion
        else
            Log.e(TAG, "There is no motion feature called " + featureName);
        return (Map) m;
    }
}
