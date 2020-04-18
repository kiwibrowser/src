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

import java.nio.charset.StandardCharsets;

public class StringToBytesBenchmark {
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
        char[] chars = new char[length];
        for (int i = 0; i < length; ++i) {
            chars[i] = (char) i;
        }
        return new String(chars);
    }

    @Param StringLengths string;

    public void timeGetBytesUtf8(int nreps) {
        for (int i = 0; i < nreps; ++i) {
            string.value.getBytes(StandardCharsets.UTF_8);
        }
    }

    public void timeGetBytesIso88591(int nreps) {
        for (int i = 0; i < nreps; ++i) {
            string.value.getBytes(StandardCharsets.ISO_8859_1);
        }
    }

    public void timeGetBytesAscii(int nreps) {
        for (int i = 0; i < nreps; ++i) {
            string.value.getBytes(StandardCharsets.US_ASCII);
        }
    }
}
