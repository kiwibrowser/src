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

public class IntConstantMultiplicationBenchmark {
    public int timeMultiplyIntByConstant6(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result *= 6;
        }
        return result;
    }
    public int timeMultiplyIntByConstant7(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result *= 7;
        }
        return result;
    }
    public int timeMultiplyIntByConstant8(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result *= 8;
        }
        return result;
    }
    public int timeMultiplyIntByConstant8_Shift(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result <<= 3;
        }
        return result;
    }
    public int timeMultiplyIntByConstant10(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result *= 10;
        }
        return result;
    }
    public int timeMultiplyIntByConstant10_Shift(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result = (result + (result << 2)) << 1;
        }
        return result;
    }
    public int timeMultiplyIntByConstant2047(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result *= 2047;
        }
        return result;
    }
    public int timeMultiplyIntByConstant2048(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result *= 2048;
        }
        return result;
    }
    public int timeMultiplyIntByConstant2049(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result *= 2049;
        }
        return result;
    }
    public int timeMultiplyIntByVariable10(int reps) {
        int result = 1;
        int factor = 10;
        for (int i = 0; i < reps; ++i) {
            result *= factor;
        }
        return result;
    }
    public int timeMultiplyIntByVariable8(int reps) {
        int result = 1;
        int factor = 8;
        for (int i = 0; i < reps; ++i) {
            result *= factor;
        }
        return result;
    }
}
