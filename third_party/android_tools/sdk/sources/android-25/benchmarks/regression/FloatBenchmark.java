/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package benchmarks.regression;

public class FloatBenchmark {
    private float f = 1.2f;
    private int i = 1067030938;

    public void timeFloatToIntBits(int reps) {
        int result = 123;
        for (int rep = 0; rep < reps; ++rep) {
            result = Float.floatToIntBits(f);
        }
        if (result != i) {
            throw new RuntimeException(Integer.toString(result));
        }
    }

    public void timeFloatToRawIntBits(int reps) {
        int result = 123;
        for (int rep = 0; rep < reps; ++rep) {
            result = Float.floatToRawIntBits(f);
        }
        if (result != i) {
            throw new RuntimeException(Integer.toString(result));
        }
    }

    public void timeIntBitsToFloat(int reps) {
        float result = 123.0f;
        for (int rep = 0; rep < reps; ++rep) {
            result = Float.intBitsToFloat(i);
        }
        if (result != f) {
            throw new RuntimeException(Float.toString(result) + " " + Float.floatToRawIntBits(result));
        }
    }
}
