/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License.
 */

package benchmarks.regression;

import java.math.BigDecimal;
import java.text.AttributedCharacterIterator;
import java.text.Bidi;
import java.text.DecimalFormat;

public class BidiBenchmark {

    private static final AttributedCharacterIterator charIter =
            DecimalFormat.getInstance().formatToCharacterIterator(new BigDecimal(Math.PI));

    public void time_createBidiFromIter(int reps) {
        for (int i = 0; i < reps; i++) {
            Bidi bidi = new Bidi(charIter);
        }
    }

    public void time_createBidiFromCharArray(int reps) {
        for (int i = 0; i < reps; i++) {
            Bidi bd = new Bidi(new char[]{'s', 's', 's'}, 0, new byte[]{(byte) 1,
                    (byte) 2, (byte) 3}, 0, 3, Bidi.DIRECTION_RIGHT_TO_LEFT);
        }
    }

    public void time_createBidiFromString(int reps) {
        for (int i = 0; i < reps; i++) {
            Bidi bidi = new Bidi("Hello", Bidi.DIRECTION_LEFT_TO_RIGHT);
        }
    }

    public void time_reorderVisually(int reps) {
        for (int i = 0; i < reps; i++) {
            Bidi.reorderVisually(new byte[]{2, 1, 3, 0, 4}, 0,
                    new String[]{"H", "e", "l", "l", "o"}, 0, 5);
        }
    }

    public void time_hebrewBidi(int reps) {
        for (int i = 0; i < reps; i++) {
            Bidi bd = new Bidi(new char[]{'\u05D0', '\u05D0', '\u05D0'}, 0,
                    new byte[]{(byte) -1, (byte) -2, (byte) -3}, 0, 3,
                    Bidi.DIRECTION_DEFAULT_RIGHT_TO_LEFT);
            bd = new Bidi(new char[]{'\u05D0', '\u05D0', '\u05D0'}, 0,
                    new byte[]{(byte) -1, (byte) -2, (byte) -3}, 0, 3,
                    Bidi.DIRECTION_LEFT_TO_RIGHT);
        }
    }

    public void time_complicatedOverrideBidi(int reps) {
        for (int i = 0; i < reps; i++) {
            Bidi bd = new Bidi("a\u05D0a\"a\u05D0\"\u05D0a".toCharArray(), 0,
                    new byte[]{0, 0, 0, -3, -3, 2, 2, 0, 3}, 0, 9,
                    Bidi.DIRECTION_RIGHT_TO_LEFT);
        }
    }

    public void time_requiresBidi(int reps) {
        for (int i = 0; i < reps; i++) {
            Bidi.requiresBidi("\u05D0".toCharArray(), 1, 1);  // false.
            Bidi.requiresBidi("\u05D0".toCharArray(), 0, 1);  // true.
        }
    }

}
