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

public class ThreadLocalBenchmark {
    private static final ThreadLocal<char[]> BUFFER = new ThreadLocal<char[]>() {
        @Override protected char[] initialValue() {
            return new char[20];
        }
    };

    public void timeThreadLocal_get(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            BUFFER.get();
        }
    }
}
