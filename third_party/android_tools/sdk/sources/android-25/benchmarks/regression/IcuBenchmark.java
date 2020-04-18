/*
 * Copyright (C) 2013 Google Inc.
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

import java.util.Locale;
import libcore.icu.ICU;

public class IcuBenchmark {

    private static final String ASCII_LOWERCASE = makeUnicodeRange(97, 122);
    private static final String ASCII_UPPERCASE = makeUnicodeRange(65, 90);

    private static final String LAT_1_SUPPLEMENT = makeUnicodeRange(0xC0, 0xFF);
    private static final String LAT_EXTENDED_A = makeUnicodeRange(0x100, 0x17F);
    private static final String LAT_EXTENDED_B = makeUnicodeRange(0x180, 0x24F);

    private static final Locale CZECH_LOCALE = Locale.forLanguageTag("cs-CZ");
    private static final Locale PINYIN_LOCALE = Locale.forLanguageTag("zh-Latn");

    public static String makeUnicodeRange(int startingCodePoint, int endingCodePoint) {
        char[] tmp = new char[endingCodePoint - startingCodePoint + 1];
        for (int i = startingCodePoint; i <= endingCodePoint; i++) {
            tmp[i - startingCodePoint] = (char) i;
        }
        return new String(tmp);
    }

    public void time_getBestDateTimePattern(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            ICU.getBestDateTimePattern("dEEEMMM", new Locale("en", "US"));
        }
    }

    // Convert standard lowercase ASCII characters to uppercase using ICU4C in the US locale.
    public void time_toUpperCaseAsciiUS(int reps) {
        for (int i = 0; i < reps; i++) {
            ICU.toUpperCase(ASCII_LOWERCASE, Locale.US);
        }
    }

    // Convert standard uppercase ASCII characters to lowercase.
    public void time_toLowerCaseAsciiUs(int reps) {
        for (int i = 0; i < reps; i++) {
            ICU.toLowerCase(ASCII_UPPERCASE, Locale.US);
        }
    }

    // Convert Latin 1 supplement characters to uppercase in France locale.
    public void time_toUpperCaseLat1SuplFr(int reps) {
        for (int i = 0; i < reps; i++) {
            ICU.toUpperCase(LAT_1_SUPPLEMENT, Locale.FRANCE);
        }
    }

    // Convert Latin Extension A characters to uppercase in Czech locale
    public void time_toUpperCaseLatExtACz(int reps) {
        for (int i = 0; i < reps; i++) {
            ICU.toUpperCase(LAT_EXTENDED_A, CZECH_LOCALE);
        }
    }

    // Convert Latin Extension B characters to uppercase in Pinyin locale.
    public void time_toUpperCaseLatExtBPinyin(int reps) {
        for (int i = 0; i < reps; i++) {
            ICU.toUpperCase(LAT_EXTENDED_B, PINYIN_LOCALE);
        }
    }

}
