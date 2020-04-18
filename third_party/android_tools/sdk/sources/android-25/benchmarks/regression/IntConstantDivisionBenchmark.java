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

public class IntConstantDivisionBenchmark {
    public int timeDivideIntByConstant2(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result /= 2;
        }
        return result;
    }
    public int timeDivideIntByConstant8(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result /= 8;
        }
        return result;
    }
    public int timeDivideIntByConstant10(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result /= 10;
        }
        return result;
    }
    public int timeDivideIntByConstant100(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result /= 100;
        }
        return result;
    }
    public int timeDivideIntByConstant100_HandOptimized(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result = (int) ((0x51eb851fL * result) >>> 37);
        }
        return result;
    }
    public int timeDivideIntByConstant2048(int reps) {
        int result = 1;
        for (int i = 0; i < reps; ++i) {
            result /= 2048;
        }
        return result;
    }
    public int timeDivideIntByVariable2(int reps) {
        int result = 1;
        int factor = 2;
        for (int i = 0; i < reps; ++i) {
            result /= factor;
        }
        return result;
    }
    public int timeDivideIntByVariable10(int reps) {
        int result = 1;
        int factor = 10;
        for (int i = 0; i < reps; ++i) {
            result /= factor;
        }
        return result;
    }
}
