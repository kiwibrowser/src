/*
 * Copyright (C) 2011 Google Inc.
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
import java.util.Locale;

public class StringCaseMappingBenchmark {
    enum Inputs {
        EMPTY(""),

        // TODO: include hairy inputs like turkish and greek.
        // TODO: locale makes a difference too.

        LOWER2(lower(2)),
        UPPER2(upper(2)),
        MIXED2(mixed(2)),

        LOWER8(lower(8)),
        UPPER8(upper(8)),
        MIXED8(mixed(8)),

        LOWER32(lower(32)),
        UPPER32(upper(32)),
        MIXED32(mixed(32)),

        LOWER512(lower(512)),
        UPPER512(upper(512)),
        MIXED512(mixed(512)),

        LOWER2048(lower(2048)),
        UPPER2048(upper(2048)),
        MIXED2048(mixed(2048)),

        LOWER_1M(lower(1024*1024)),
        UPPER_1M(upper(1024*1024)),
        MIXED_1M(mixed(1024*1024));

        final String value;
        private Inputs(String value) { this.value = value; }
        private static String lower(int length) {
            return makeString(length, "a0b1c2d3e4f5g6h7i8j9klmnopqrstuvwxyz");
        }
        private static String upper(int length) {
            return makeString(length, "A0B1C2D3E4F5G6H7I8J9KLMNOPQRSTUVWXYZ");
        }
        private static String mixed(int length) {
            return makeString(length, "Aa0Bb1Cc2Dd3Ee4Ff5Gg6Hh7Ii8Jj9KkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz");
        }
        private static String makeString(int length, String alphabet) {
            StringBuilder sb = new StringBuilder(length);
            for (int i = 0; i < length; ++i) {
                sb.append(alphabet.charAt(i % alphabet.length()));
            }
            return sb.toString();
        }
    }
    @Param private Inputs s;

    public void timeToUpperCase_US(int reps) {
        for (int i = 0; i < reps; ++i) {
            s.value.toUpperCase(Locale.US);
        }
    }

    // toUpperCase for Greek is an extra-hard case that uses icu4c's Transliterator.
    public void timeToUpperCase_el_GR(int reps) {
        Locale el_GR = new Locale("el", "GR");
        for (int i = 0; i < reps; ++i) {
            s.value.toUpperCase(el_GR);
        }
    }

    public void timeToLowerCase_US(int reps) {
        for (int i = 0; i < reps; ++i) {
            s.value.toLowerCase(Locale.US);
        }
    }

    public void timeToUpperCase_Ascii(int reps) {
        for (int i = 0; i < reps; ++i) {
            toUpperCaseAscii(s.value);
        }
    }

    public void timeToLowerCase_Ascii(int reps) {
        for (int i = 0; i < reps; ++i) {
            toLowerCaseAscii(s.value);
        }
    }

    public void timeToUpperCase_ICU(int reps) {
        for (int i = 0; i < reps; ++i) {
            libcore.icu.ICU.toUpperCase(s.value, Locale.US);
        }
    }

    public void timeToLowerCase_ICU(int reps) {
        for (int i = 0; i < reps; ++i) {
            libcore.icu.ICU.toLowerCase(s.value, Locale.US);
        }
    }

    public static String toUpperCaseAscii(String s) {
        for (int i = 0, length = s.length(); i < length; i++) {
            char c = s.charAt(i);
            if (c < 'a' || c > 'z') {
                continue; // fast path avoids allocation
            }

            // slow path: s contains lower case chars
            char[] result = s.toCharArray();
            for (; i < length; i++) {
                c = result[i];
                if (c >= 'a' && c <= 'z') {
                    result[i] -= ('a' - 'A');
                }
            }
            return new String(result);
        }
        return s;
    }

    public static String toLowerCaseAscii(String s) {
        for (int i = 0, length = s.length(); i < length; i++) {
            char c = s.charAt(i);
            if (c < 'A' || c > 'Z') {
                continue; // fast path avoids allocation
            }

            // slow path: s contains upper case chars
            char[] result = s.toCharArray();
            for (; i < length; i++) {
                c = result[i];
                if (c >= 'A' && c <= 'Z') {
                    result[i] += ('a' - 'A');
                }
            }
            return new String(result);
        }
        return s;
    }
}
