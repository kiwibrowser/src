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

import com.google.caliper.Param;

public class StringBenchmark {
    enum StringLengths {
        EMPTY(""),
        SHORT("short"),
        EIGHTY(makeString(80)),
        EIGHT_KI(makeString(8192));
        final String value;
        private StringLengths(String value) { this.value = value; }
    }
    @Param private StringLengths s;

    private static String makeString(int length) {
        StringBuilder result = new StringBuilder(length);
        for (int i = 0; i < length; ++i) {
            result.append((char) i);
        }
        return result.toString();
    }

    public void timeHashCode(int reps) {
        for (int i = 0; i < reps; ++i) {
            s.value.hashCode();
        }
    }
}
