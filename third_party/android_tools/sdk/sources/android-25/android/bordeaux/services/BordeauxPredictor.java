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

import android.bordeaux.services.IPredictor;
import android.content.Context;
import android.os.RemoteException;
import android.util.Log;
import android.util.Pair;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Map;

/** Predictor for the Learning framework.
 */
public class BordeauxPredictor {
    static final String TAG = "BordeauxPredictor";
    static final String PREDICTOR_NOTAVAILABLE = "Predictor is not available.";
    private Context mContext;
    private String mName;
    private IPredictor mPredictor;

    public BordeauxPredictor(Context context) {
        mContext = context;
        mName = "defaultPredictor";
        mPredictor = BordeauxManagerService.getPredictor(context, mName);
    }

    public BordeauxPredictor(Context context, String name) {
        mContext = context;
        mName = name;
        mPredictor = BordeauxManagerService.getPredictor(context, mName);
    }

    public boolean reset() {
        if (!retrievePredictor()){
            Log.e(TAG, "reset: " + PREDICTOR_NOTAVAILABLE);
            return false;
        }
        try {
            mPredictor.resetPredictor();
            return true;
        } catch (RemoteException e) {
        }
        return false;
    }

    public boolean retrievePredictor() {
        if (mPredictor == null) {
            mPredictor = BordeauxManagerService.getPredictor(mContext, mName);
        }
        if (mPredictor == null) {
            Log.e(TAG, "retrievePredictor: " + PREDICTOR_NOTAVAILABLE);
            return false;
        }
        return true;
    }

    public void addSample(String sampleName) {
        if (!retrievePredictor())
            throw new RuntimeException(PREDICTOR_NOTAVAILABLE);
        try {
            mPredictor.pushNewSample(sampleName);
        } catch (RemoteException e) {
            Log.e(TAG,"Exception: pushing a new example");
            throw new RuntimeException(PREDICTOR_NOTAVAILABLE);
        }
    }

    public ArrayList<Pair<String, Float> > getTopSamples() {
        return getTopSamples(0);
    }

    public ArrayList<Pair<String, Float> > getTopSamples(int topK) {
        try {
            ArrayList<StringFloat> topList =
                    (ArrayList<StringFloat>) mPredictor.getTopCandidates(topK);

            ArrayList<Pair<String, Float> > topSamples =
                    new ArrayList<Pair<String, Float> >(topList.size());
            for (int i = 0; i < topList.size(); ++i) {
                topSamples.add(new Pair<String, Float>(topList.get(i).key, topList.get(i).value));
            }
            return topSamples;
        } catch(RemoteException e) {
            Log.e(TAG,"Exception: getTopSamples");
            throw new RuntimeException(PREDICTOR_NOTAVAILABLE);
        }
    }

    public boolean setParameter(String key, String value) {
        if (!retrievePredictor())
            throw new RuntimeException(PREDICTOR_NOTAVAILABLE);
        try {
            return mPredictor.setPredictorParameter(key, value);
        } catch (RemoteException e) {
            Log.e(TAG,"Exception: setting predictor parameter");
            throw new RuntimeException(PREDICTOR_NOTAVAILABLE);
        }
    }
}
