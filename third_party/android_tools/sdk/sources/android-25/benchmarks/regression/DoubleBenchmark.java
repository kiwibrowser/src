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

public class DoubleBenchmark {
    private double d = 1.2;
    private long l = 4608083138725491507L;

    public void timeDoubleToLongBits(int reps) {
        long result = 123;
        for (int rep = 0; rep < reps; ++rep) {
            result = Double.doubleToLongBits(d);
        }
        if (result != l) {
            throw new RuntimeException(Long.toString(result));
        }
    }

    public void timeDoubleToRawLongBits(int reps) {
        long result = 123;
        for (int rep = 0; rep < reps; ++rep) {
            result = Double.doubleToRawLongBits(d);
        }
        if (result != l) {
            throw new RuntimeException(Long.toString(result));
        }
    }

    public void timeLongBitsToDouble(int reps) {
        double result = 123.0;
        for (int rep = 0; rep < reps; ++rep) {
            result = Double.longBitsToDouble(l);
        }
        if (result != d) {
            throw new RuntimeException(Double.toString(result) + " " + Double.doubleToRawLongBits(result));
        }
    }
}
