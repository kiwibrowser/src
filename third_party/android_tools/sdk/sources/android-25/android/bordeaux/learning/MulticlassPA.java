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

/**
 * Wrapper for multiclass passive aggressive classifier.
 * version 1 supports indexed sparse feature only.
 */
public class MulticlassPA {

    public MulticlassPA(int numClasses, int numDimensions, float aggressiveness) {
      nativeClassifier = initNativeClassifier(numClasses, numDimensions,
                                              aggressiveness);
    }

    /**
     * Train on one example
     */
    public boolean sparseTrainOneExample(int[] index_array,
                                       float[] float_array,
                                       int target) {
        return nativeSparseTrainOneExample(
                index_array, float_array, target, nativeClassifier);
    }

    /**
     * Train on one example
     */
    public int sparseGetClass(int[] index_array, float[] float_array) {
        return nativeSparseGetClass(index_array, float_array, nativeClassifier);
    }

    static {
        System.loadLibrary("bordeaux");
    }

    private long nativeClassifier;

    /*
     * Initialize native classifier
     */
    private native long initNativeClassifier(int num_classes, int num_dims, float aggressiveness);

    private native void deleteNativeClassifier(long classPtr);

    private native boolean nativeSparseTrainOneExample(int[] index_array,
                                                     float[] float_array,
                                                     int target, long classPtr);

    private native int nativeSparseGetClass(int[] index_array,
                                            float[] float_array,
                                            long classPtr);
}
