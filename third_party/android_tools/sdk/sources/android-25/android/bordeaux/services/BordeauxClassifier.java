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

import android.bordeaux.services.ILearning_MulticlassPA;
import android.bordeaux.services.IntFloat;
import android.content.Context;
import android.os.RemoteException;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Map;

/** Classifier for the Learning framework.
 *  For training: call trainOneSample
 *  For classifying: call classify
 *  Data is represented as sparse key, value pair. And key is an integer, value
 *  is a float. Class label(target) for the training data is an integer.
 *  Note: since the actual classifier is running in a remote the service.
 *  Sometimes the connection may be lost or not established.
 *
 */
public class BordeauxClassifier {
    static final String TAG = "BordeauxClassifier";
    private Context mContext;
    private String mName;
    private ILearning_MulticlassPA mClassifier;
    private ArrayList<IntFloat> getArrayList(final HashMap<Integer, Float> sample) {
        ArrayList<IntFloat> intfloat_sample = new ArrayList<IntFloat>();
        for (Map.Entry<Integer, Float> x : sample.entrySet()) {
           IntFloat v = new IntFloat();
           v.index = x.getKey();
           v.value = x.getValue();
           intfloat_sample.add(v);
        }
        return intfloat_sample;
    }

    private boolean retrieveClassifier() {
        if (mClassifier == null)
            mClassifier = BordeauxManagerService.getClassifier(mContext, mName);
        // if classifier is not available, return false
        if (mClassifier == null) {
            Log.i(TAG,"Classifier not available.");
            return false;
        }
        return true;
    }

    public BordeauxClassifier(Context context) {
        mContext = context;
        mName = "defaultClassifier";
        mClassifier = BordeauxManagerService.getClassifier(context, mName);
    }

    public BordeauxClassifier(Context context, String name) {
        mContext = context;
        mName = name;
        mClassifier = BordeauxManagerService.getClassifier(context, mName);
    }

    public boolean update(final HashMap<Integer, Float> sample, int target) {
        if (!retrieveClassifier())
            return false;
        try {
            mClassifier.TrainOneSample(getArrayList(sample), target);
        } catch (RemoteException e) {
            Log.e(TAG,"Exception: training one sample.");
            return false;
        }
        return true;
    }

    public int classify(final HashMap<Integer, Float> sample) {
        // if classifier is not available return -1 as an indication of fail.
        if (!retrieveClassifier())
            return -1;
        try {
            return mClassifier.Classify(getArrayList(sample));
        } catch (RemoteException e) {
            Log.e(TAG,"Exception: classify the sample.");
            // return an invalid number.
            // TODO: throw exception.
            return -1;
        }
    }
}
