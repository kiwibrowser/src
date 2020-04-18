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

package benchmarks;

import java.util.Arrays;

public class ArrayCopyBenchmark {
    public void timeManualArrayCopy(int reps) {
        char[] src = new char[8192];
        for (int rep = 0; rep < reps; ++rep) {
            char[] dst = new char[8192];
            for (int i = 0; i < 8192; ++i) {
                dst[i] = src[i];
            }
        }
    }

    public void time_System_arrayCopy(int reps) {
        char[] src = new char[8192];
        for (int rep = 0; rep < reps; ++rep) {
            char[] dst = new char[8192];
            System.arraycopy(src, 0, dst, 0, 8192);
        }
    }

    public void time_Arrays_copyOf(int reps) {
        char[] src = new char[8192];
        for (int rep = 0; rep < reps; ++rep) {
            char[] dst = Arrays.copyOf(src, 8192);
        }
    }

    public void time_Arrays_copyOfRange(int reps) {
        char[] src = new char[8192];
        for (int rep = 0; rep < reps; ++rep) {
            char[] dst = Arrays.copyOfRange(src, 0, 8192);
        }
    }
}
