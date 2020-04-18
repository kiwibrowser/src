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

import java.util.regex.Pattern;

public class StringSplitBenchmark {
    public void timeStringSplitComma(int reps) {
        for (int i = 0; i < reps; ++i) {
            "this,is,a,simple,example".split(",");
        }
    }

    public void timeStringSplitLiteralDot(int reps) {
        for (int i = 0; i < reps; ++i) {
            "this.is.a.simple.example".split("\\.");
        }
    }

    public void timeStringSplitNewline(int reps) {
        for (int i = 0; i < reps; ++i) {
            "this\nis\na\nsimple\nexample\n".split("\n");
        }
    }

    public void timePatternSplitComma(int reps) {
        Pattern p = Pattern.compile(",");
        for (int i = 0; i < reps; ++i) {
            p.split("this,is,a,simple,example");
        }
    }

    public void timePatternSplitLiteralDot(int reps) {
        Pattern p = Pattern.compile("\\.");
        for (int i = 0; i < reps; ++i) {
            p.split("this.is.a.simple.example");
        }
    }

    public void timeStringSplitHard(int reps) {
        for (int i = 0; i < reps; ++i) {
            "this,is,a,harder,example".split("[,]");
        }
    }
}
