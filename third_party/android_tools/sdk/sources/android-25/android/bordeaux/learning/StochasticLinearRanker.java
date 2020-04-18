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

import java.io.Serializable;
import java.util.List;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * Stochastic Linear Ranker, learns how to rank a sample. The learned rank score
 * can be used to compare samples.
 * This java class wraps the native StochasticLinearRanker class.
 * To update the ranker, call updateClassifier with two samples, with the first
 * one having higher rank than the second one.
 * To get the rank score of the sample call scoreSample.
 *  TODO: adding more interfaces for changing the learning parameters
 */
public class StochasticLinearRanker {
    String TAG = "StochasticLinearRanker";
    public static int VAR_NUM = 14;
    static public class Model implements Serializable {
        public HashMap<String, Float> weights = new HashMap<String, Float>();
        public float weightNormalizer = 1;
        public HashMap<String, String> parameters = new HashMap<String, String>();
    }

    /**
     * Initializing a ranker
     */
    public StochasticLinearRanker() {
        mNativeClassifier = initNativeClassifier();
    }

    /**
     * Reset the ranker
     */
    public void resetRanker(){
        deleteNativeClassifier(mNativeClassifier);
        mNativeClassifier = initNativeClassifier();
    }

    /**
     * Train the ranker with a pair of samples. A sample,  a pair of arrays of
     * keys and values. The first sample should have higher rank than the second
     * one.
     */
    public boolean updateClassifier(String[] keys_positive,
                                    float[] values_positive,
                                    String[] keys_negative,
                                    float[] values_negative) {
        return nativeUpdateClassifier(keys_positive, values_positive,
                                      keys_negative, values_negative,
                                      mNativeClassifier);
    }

    /**
     * Get the rank score of the sample, a sample is a list of key, value pairs.
     */
    public float scoreSample(String[] keys, float[] values) {
        return nativeScoreSample(keys, values, mNativeClassifier);
    }

    /**
     * Get the current model and parameters of ranker
     */
    public Model getUModel(){
        Model slrModel = new Model();
        int len = nativeGetLengthClassifier(mNativeClassifier);
        String[] wKeys = new String[len];
        float[] wValues = new float[len];
        float wNormalizer = 1;
        nativeGetWeightClassifier(wKeys, wValues, wNormalizer, mNativeClassifier);
        slrModel.weightNormalizer = wNormalizer;
        for (int  i=0; i< wKeys.length ; i++)
            slrModel.weights.put(wKeys[i], wValues[i]);

        String[] paramKeys = new String[VAR_NUM];
        String[] paramValues = new String[VAR_NUM];
        nativeGetParameterClassifier(paramKeys, paramValues, mNativeClassifier);
        for (int  i=0; i< paramKeys.length ; i++)
            slrModel.parameters.put(paramKeys[i], paramValues[i]);
        return slrModel;
    }

    /**
     * load the given model and parameters to the ranker
     */
    public boolean loadModel(Model model) {
        String[] wKeys = new String[model.weights.size()];
        float[] wValues = new float[model.weights.size()];
        int i = 0 ;
        for (Map.Entry<String, Float> e : model.weights.entrySet()){
            wKeys[i] = e.getKey();
            wValues[i] = e.getValue();
            i++;
        }
        boolean res = setModelWeights(wKeys, wValues, model.weightNormalizer);
        if (!res)
            return false;

        for (Map.Entry<String, String> e : model.parameters.entrySet()){
            res = setModelParameter(e.getKey(), e.getValue());
            if (!res)
                return false;
        }
        return res;
    }

    public boolean setModelWeights(String[] keys, float [] values, float normalizer){
        return nativeSetWeightClassifier(keys, values, normalizer, mNativeClassifier);
    }

    public boolean setModelParameter(String key, String value){
        boolean res = nativeSetParameterClassifier(key, value, mNativeClassifier);
        return res;
    }

    /**
     * Print a model for debugging
     */
    public void print(Model model){
        String Sw = "";
        String Sp = "";
        for (Map.Entry<String, Float> e : model.weights.entrySet())
            Sw = Sw + "<" + e.getKey() + "," + e.getValue() + "> ";
        for (Map.Entry<String, String> e : model.parameters.entrySet())
            Sp = Sp + "<" + e.getKey() + "," + e.getValue() + "> ";
        Log.i(TAG, "Weights are " + Sw);
        Log.i(TAG, "Normalizer is " + model.weightNormalizer);
        Log.i(TAG, "Parameters are " + Sp);
    }

    @Override
    protected void finalize() throws Throwable {
        deleteNativeClassifier(mNativeClassifier);
    }

    static {
        System.loadLibrary("bordeaux");
    }

    private long mNativeClassifier;

    /*
     * The following methods are the java stubs for the jni implementations.
     */
    private native long initNativeClassifier();

    private native void deleteNativeClassifier(long classifierPtr);

    private native boolean nativeUpdateClassifier(
            String[] keys_positive,
            float[] values_positive,
            String[] keys_negative,
            float[] values_negative,
            long classifierPtr);

    private native float nativeScoreSample(String[] keys, float[] values, long classifierPtr);

    private native void nativeGetWeightClassifier(String [] keys, float[] values, float normalizer,
                                                  long classifierPtr);

    private native void nativeGetParameterClassifier(String [] keys, String[] values,
                                                  long classifierPtr);

    private native int nativeGetLengthClassifier(long classifierPtr);

    private native boolean nativeSetWeightClassifier(String [] keys, float[] values,
                                                     float normalizer, long classifierPtr);

    private native boolean nativeSetParameterClassifier(String key, String value,
                                                        long classifierPtr);
}
