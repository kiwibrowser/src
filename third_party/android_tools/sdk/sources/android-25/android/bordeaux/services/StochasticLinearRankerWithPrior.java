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

import android.bordeaux.learning.StochasticLinearRanker;
import java.util.HashMap;
import java.util.Map;
import java.io.Serializable;

public class StochasticLinearRankerWithPrior extends StochasticLinearRanker {
    private final String TAG = "StochasticLinearRankerWithPrior";
    private final float EPSILON = 0.0001f;

    /* If the is parameter is true, the final score would be a
    linear combination of user model and prior model */
    private final String USE_PRIOR = "usePriorInformation";

    /* When prior model is used, this parmaeter will set the mixing factor, alpha. */
    private final String SET_ALPHA = "setAlpha";

    /* When prior model is used, If this parameter is true then algorithm will use
    the automatic cross validated alpha for mixing user model and prior model */
    private final String USE_AUTO_ALPHA = "useAutoAlpha";

    /* When automatic cross validation is active, this parameter will
    set the forget rate in cross validation. */
    private final String SET_FORGET_RATE = "setForgetRate";

    /* When automatic cross validation is active, this parameter will
    set the minium number of required training pairs before using the user model */
    private final String SET_MIN_TRAIN_PAIR = "setMinTrainingPair";

    private final String SET_USER_PERF = "setUserPerformance";
    private final String SET_PRIOR_PERF = "setPriorPerformance";
    private final String SET_NUM_TRAIN_PAIR = "setNumberTrainingPairs";
    private final String SET_AUTO_ALPHA = "setAutoAlpha";



    private HashMap<String, Float> mPriorWeights = new HashMap<String, Float>();
    private float mAlpha = 0;
    private float mAutoAlpha = 0;
    private float mForgetRate = 0;
    private float mUserRankerPerf = 0;
    private float mPriorRankerPerf = 0;
    private int mMinReqTrainingPair = 0;
    private int mNumTrainPair = 0;
    private boolean mUsePrior = false;
    private boolean mUseAutoAlpha = false;

    static public class Model implements Serializable {
        public StochasticLinearRanker.Model uModel = new StochasticLinearRanker.Model();
        public HashMap<String, Float> priorWeights = new HashMap<String, Float>();
        public HashMap<String, String> priorParameters = new HashMap<String, String>();
    }

    @Override
    public void resetRanker(){
        super.resetRanker();
        mPriorWeights.clear();
        mAlpha = 0;
        mAutoAlpha = 0;
        mForgetRate = 0;
        mMinReqTrainingPair = 0;
        mUserRankerPerf = 0;
        mPriorRankerPerf = 0;
        mNumTrainPair = 0;
        mUsePrior = false;
        mUseAutoAlpha = false;
    }

    @Override
    public float scoreSample(String[] keys, float[] values) {
        if (!mUsePrior){
            return super.scoreSample(keys, values);
        } else {
            if (mUseAutoAlpha) {
                if (mNumTrainPair > mMinReqTrainingPair)
                    return (1 - mAutoAlpha) * super.scoreSample(keys,values) +
                            mAutoAlpha * priorScoreSample(keys,values);
                else
                    return priorScoreSample(keys,values);
            } else
                return (1 - mAlpha) * super.scoreSample(keys,values) +
                        mAlpha * priorScoreSample(keys,values);
        }
    }

    public float priorScoreSample(String[] keys, float[] values) {
        float score = 0;
        for (int i=0; i< keys.length; i++){
            if (mPriorWeights.get(keys[i]) != null )
                score = score + mPriorWeights.get(keys[i]) * values[i];
        }
        return score;
    }

    @Override
    public boolean updateClassifier(String[] keys_positive,
                                    float[] values_positive,
                                    String[] keys_negative,
                                    float[] values_negative){
        if (mUsePrior && mUseAutoAlpha && (mNumTrainPair > mMinReqTrainingPair))
            updateAutoAlpha(keys_positive, values_positive, keys_negative, values_negative);
        mNumTrainPair ++;
        return super.updateClassifier(keys_positive, values_positive,
                                      keys_negative, values_negative);
    }

    void updateAutoAlpha(String[] keys_positive,
                     float[] values_positive,
                     String[] keys_negative,
                     float[] values_negative) {
        float positiveUserScore = super.scoreSample(keys_positive, values_positive);
        float negativeUserScore = super.scoreSample(keys_negative, values_negative);
        float positivePriorScore = priorScoreSample(keys_positive, values_positive);
        float negativePriorScore = priorScoreSample(keys_negative, values_negative);
        float userDecision = 0;
        float priorDecision = 0;
        if (positiveUserScore > negativeUserScore)
            userDecision = 1;
        if (positivePriorScore > negativePriorScore)
            priorDecision = 1;
        mUserRankerPerf = (1 - mForgetRate) * mUserRankerPerf + userDecision;
        mPriorRankerPerf = (1 - mForgetRate) * mPriorRankerPerf + priorDecision;
        mAutoAlpha = (mPriorRankerPerf + EPSILON) / (mUserRankerPerf + mPriorRankerPerf + EPSILON);
    }

    public Model getModel(){
        Model m = new Model();
        m.uModel = super.getUModel();
        m.priorWeights.putAll(mPriorWeights);
        m.priorParameters.put(SET_ALPHA, String.valueOf(mAlpha));
        m.priorParameters.put(SET_AUTO_ALPHA, String.valueOf(mAutoAlpha));
        m.priorParameters.put(SET_FORGET_RATE, String.valueOf(mForgetRate));
        m.priorParameters.put(SET_MIN_TRAIN_PAIR, String.valueOf(mMinReqTrainingPair));
        m.priorParameters.put(SET_USER_PERF, String.valueOf(mUserRankerPerf));
        m.priorParameters.put(SET_PRIOR_PERF, String.valueOf(mPriorRankerPerf));
        m.priorParameters.put(SET_NUM_TRAIN_PAIR, String.valueOf(mNumTrainPair));
        m.priorParameters.put(USE_AUTO_ALPHA, String.valueOf(mUseAutoAlpha));
        m.priorParameters.put(USE_PRIOR, String.valueOf(mUsePrior));
        return m;
    }

    public boolean loadModel(Model m) {
        mPriorWeights.clear();
        mPriorWeights.putAll(m.priorWeights);
        for (Map.Entry<String, String> e : m.priorParameters.entrySet()) {
            boolean res = setModelParameter(e.getKey(), e.getValue());
            if (!res) return false;
        }
        return super.loadModel(m.uModel);
    }

    public boolean setModelPriorWeights(HashMap<String, Float> pw){
        mPriorWeights.clear();
        mPriorWeights.putAll(pw);
        return true;
    }

    public boolean setModelParameter(String key, String value){
        if (key.equals(USE_AUTO_ALPHA)){
            mUseAutoAlpha = Boolean.parseBoolean(value);
        } else if (key.equals(USE_PRIOR)){
            mUsePrior = Boolean.parseBoolean(value);
        } else if (key.equals(SET_ALPHA)){
            mAlpha = Float.valueOf(value.trim()).floatValue();
        }else if (key.equals(SET_AUTO_ALPHA)){
            mAutoAlpha = Float.valueOf(value.trim()).floatValue();
        }else if (key.equals(SET_FORGET_RATE)){
            mForgetRate = Float.valueOf(value.trim()).floatValue();
        }else if (key.equals(SET_MIN_TRAIN_PAIR)){
            mMinReqTrainingPair = (int) Float.valueOf(value.trim()).floatValue();
        }else if (key.equals(SET_USER_PERF)){
            mUserRankerPerf = Float.valueOf(value.trim()).floatValue();
        }else if (key.equals(SET_PRIOR_PERF)){
            mPriorRankerPerf = Float.valueOf(value.trim()).floatValue();
        }else if (key.equals(SET_NUM_TRAIN_PAIR)){
            mNumTrainPair = (int) Float.valueOf(value.trim()).floatValue();
        }else
            return super.setModelParameter(key, value);
        return true;
    }

    public void print(Model m){
        super.print(m.uModel);
        String Spw = "";
        for (Map.Entry<String, Float> e : m.priorWeights.entrySet())
            Spw = Spw + "<" + e.getKey() + "," + e.getValue() + "> ";
        Log.i(TAG, "Prior model is " + Spw);
        String Spp = "";
        for (Map.Entry<String, String> e : m.priorParameters.entrySet())
            Spp = Spp + "<" + e.getKey() + "," + e.getValue() + "> ";
        Log.i(TAG, "Prior parameters are " + Spp);
    }
}
