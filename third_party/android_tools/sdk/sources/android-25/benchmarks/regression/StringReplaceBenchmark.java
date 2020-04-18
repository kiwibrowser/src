/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package benchmarks.regression;

import com.google.caliper.Param;

public class StringReplaceBenchmark {
    static enum StringLengths {
        EMPTY(""),
        L_16(makeString(16)),
        L_64(makeString(64)),
        L_256(makeString(256)),
        L_512(makeString(512));

        private final String value;

        private StringLengths(String s) {
            this.value = s;
        }
    }

    private static final String makeString(int length) {
        final String sequence8 = "abcdefghijklmnop";
        final int numAppends = (length / 16) - 1;
        StringBuilder stringBuilder = new StringBuilder(length);

        // (n-1) occurences of "abcdefghijklmnop"
        for (int i = 0; i < numAppends; ++i) {
            stringBuilder.append(sequence8);
        }

        // and one final occurence of qrstuvwx.
        stringBuilder.append("qrstuvwx");

        return stringBuilder.toString();
    }

    @Param private StringLengths s;

    public void timeReplaceCharNonExistent(int reps) {
        for (int i = 0; i < reps; ++i) {
            s.value.replace('z', '0');
        }
    }

    public void timeReplaceCharRepeated(int reps) {
        for (int i = 0; i < reps; ++i) {
            s.value.replace('a', '0');
        }
    }

    public void timeReplaceSingleChar(int reps) {
        for (int i = 0; i < reps; ++i) {
            s.value.replace('q', '0');
        }
    }

    public void timeReplaceSequenceNonExistent(int reps) {
        for (int i = 0; i < reps; ++i) {
            s.value.replace("fish", "0");
        }
    }

    public void timeReplaceSequenceRepeated(int reps) {
        for (int i = 0; i < reps; ++i) {
            s.value.replace("jklm", "0");
        }
    }

    public void timeReplaceSingleSequence(int reps) {
        for (int i = 0; i < reps; ++i) {
            s.value.replace("qrst", "0");
        }
    }
}
