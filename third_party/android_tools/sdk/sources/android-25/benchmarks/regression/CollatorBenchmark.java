/*
 * Copyright (C) 2015 Google Inc.
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

import java.text.Collator;
import java.text.RuleBasedCollator;
import java.util.Locale;

public class CollatorBenchmark {

    private static final RuleBasedCollator collator = (RuleBasedCollator)
            Collator.getInstance(Locale.US);

    public void timeCollatorPrimary(int reps) {
        collator.setStrength(Collator.PRIMARY);
        for (int i = 0; i < reps; i++) {
            collator.compare("abcde", "abcdf");
            collator.compare("abcde", "abcde");
            collator.compare("abcdf", "abcde");
        }
    }

    public void timeCollatorSecondary(int reps) {
        collator.setStrength(Collator.SECONDARY);
        for (int i = 0; i < reps; i++) {
            collator.compare("abcdÂ", "abcdÄ");
            collator.compare("abcdÂ", "abcdÂ");
            collator.compare("abcdÄ", "abcdÂ");
        }
    }

    public void timeCollatorTertiary(int reps) {
        collator.setStrength(Collator.TERTIARY);
        for (int i = 0; i < reps; i++) {
            collator.compare("abcdE", "abcde");
            collator.compare("abcde", "abcde");
            collator.compare("abcde", "abcdE");
        }
    }

    public void timeCollatorIdentical(int reps) {
        collator.setStrength(Collator.IDENTICAL);
        for (int i = 0; i < reps; i++) {
            collator.compare("abcdȪ", "abcdȫ");
            collator.compare("abcdȪ", "abcdȪ");
            collator.compare("abcdȫ", "abcdȪ");
        }
    }
}
