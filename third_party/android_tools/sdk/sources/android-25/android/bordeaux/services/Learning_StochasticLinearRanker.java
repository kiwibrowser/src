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

import android.bordeaux.learning.StochasticLinearRanker;
import android.bordeaux.services.IBordeauxLearner.ModelChangeCallback;
import android.os.IBinder;
import android.util.Log;
import java.util.List;
import java.util.ArrayList;
import java.io.*;
import java.lang.ClassNotFoundException;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.io.ByteArrayOutputStream;
import java.util.HashMap;
import java.util.Map;

public class Learning_StochasticLinearRanker extends ILearning_StochasticLinearRanker.Stub
        implements IBordeauxLearner {

    private final String TAG = "ILearning_StochasticLinearRanker";
    private StochasticLinearRankerWithPrior mLearningSlRanker = null;
    private ModelChangeCallback modelChangeCallback = null;

    public Learning_StochasticLinearRanker(){
    }

    public void ResetRanker(){
        if (mLearningSlRanker == null)
            mLearningSlRanker = new StochasticLinearRankerWithPrior();
        mLearningSlRanker.resetRanker();
    }

    public boolean UpdateClassifier(List<StringFloat> sample_1, List<StringFloat> sample_2){
        ArrayList<StringFloat> temp_1 = (ArrayList<StringFloat>)sample_1;
        String[] keys_1 = new String[temp_1.size()];
        float[] values_1 = new float[temp_1.size()];
        for (int i = 0; i < temp_1.size(); i++){
            keys_1[i] = temp_1.get(i).key;
            values_1[i] = temp_1.get(i).value;
        }
        ArrayList<StringFloat> temp_2 = (ArrayList<StringFloat>)sample_2;
        String[] keys_2 = new String[temp_2.size()];
        float[] values_2 = new float[temp_2.size()];
        for (int i = 0; i < temp_2.size(); i++){
            keys_2[i] = temp_2.get(i).key;
            values_2[i] = temp_2.get(i).value;
        }
        if (mLearningSlRanker == null)
            mLearningSlRanker = new StochasticLinearRankerWithPrior();
        boolean res = mLearningSlRanker.updateClassifier(keys_1,values_1,keys_2,values_2);
        if (res && modelChangeCallback != null) {
            modelChangeCallback.modelChanged(this);
        }
        return res;
    }

    public float ScoreSample(List<StringFloat> sample) {
        ArrayList<StringFloat> temp = (ArrayList<StringFloat>)sample;
        String[] keys = new String[temp.size()];
        float[] values = new float[temp.size()];
        for (int i = 0; i < temp.size(); i++){
            keys[i] = temp.get(i).key;
            values[i] = temp.get(i).value;
        }
        if (mLearningSlRanker == null)
            mLearningSlRanker = new StochasticLinearRankerWithPrior();
        return mLearningSlRanker.scoreSample(keys,values);
    }

    public boolean SetModelPriorWeight(List<StringFloat> sample) {
        ArrayList<StringFloat> temp = (ArrayList<StringFloat>)sample;
        HashMap<String, Float> weights = new HashMap<String, Float>();
        for (int i = 0; i < temp.size(); i++)
            weights.put(temp.get(i).key, temp.get(i).value);
        if (mLearningSlRanker == null)
            mLearningSlRanker = new StochasticLinearRankerWithPrior();
        return mLearningSlRanker.setModelPriorWeights(weights);
    }

    public boolean SetModelParameter(String key, String value) {
        if (mLearningSlRanker == null)
            mLearningSlRanker = new StochasticLinearRankerWithPrior();
        return mLearningSlRanker.setModelParameter(key,value);
    }

    // Beginning of the IBordeauxLearner Interface implementation
    public byte [] getModel() {
        if (mLearningSlRanker == null)
            mLearningSlRanker = new StochasticLinearRankerWithPrior();
        StochasticLinearRankerWithPrior.Model model = mLearningSlRanker.getModel();
        try {
            ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
            ObjectOutputStream objStream = new ObjectOutputStream(byteStream);
            objStream.writeObject(model);
            //return byteStream.toByteArray();
            byte[] bytes = byteStream.toByteArray();
            return bytes;
        } catch (IOException e) {
            throw new RuntimeException("Can't get model");
        }
    }

    public boolean setModel(final byte [] modelData) {
        try {
            ByteArrayInputStream input = new ByteArrayInputStream(modelData);
            ObjectInputStream objStream = new ObjectInputStream(input);
            StochasticLinearRankerWithPrior.Model model =
                    (StochasticLinearRankerWithPrior.Model) objStream.readObject();
            if (mLearningSlRanker == null)
                mLearningSlRanker = new StochasticLinearRankerWithPrior();
            boolean res = mLearningSlRanker.loadModel(model);
            return res;
        } catch (IOException e) {
            throw new RuntimeException("Can't load model");
        } catch (ClassNotFoundException e) {
            throw new RuntimeException("Learning class not found");
        }
    }

    public IBinder getBinder() {
        return this;
    }

    public void setModelChangeCallback(ModelChangeCallback callback) {
        modelChangeCallback = callback;
    }
    // End of IBordeauxLearner Interface implemenation
}
