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

public class StringIsEmptyBenchmark {
    public void timeIsEmpty_NonEmpty(int reps) {
        boolean result = true;
        for (int i = 0; i < reps; ++i) {
            result &= !("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx".isEmpty());
        }
        if (!result) throw new RuntimeException();
    }
    
    public void timeIsEmpty_Empty(int reps) {
        boolean result = true;
        for (int i = 0; i < reps; ++i) {
            result &= ("".isEmpty());
        }
        if (!result) throw new RuntimeException();
    }
    
    public void timeLengthEqualsZero(int reps) {
        boolean result = true;
        for (int i = 0; i < reps; ++i) {
            result &= !("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx".length() == 0);
        }
        if (!result) throw new RuntimeException();
    }
 
    public void timeEqualsEmpty(int reps) {
        boolean result = true;
        for (int i = 0; i < reps; ++i) {
            result &= !"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx".equals("");
        }
        if (!result) throw new RuntimeException();
    }
}
