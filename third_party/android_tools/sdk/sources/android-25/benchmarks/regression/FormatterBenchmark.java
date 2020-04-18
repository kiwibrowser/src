/*
 * Copyright (C) 2009 Google Inc.
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

import java.util.Formatter;
import java.util.Locale;

/**
 * Compares Formatter against hand-written StringBuilder code.
 */
public class FormatterBenchmark {
    public void timeFormatter_NoFormatting(int reps) {
        for (int i = 0; i < reps; i++) {
            Formatter f = new Formatter();
            f.format("this is a reasonably short string that doesn't actually need any formatting");
        }
    }

    public void timeStringBuilder_NoFormatting(int reps) {
        for (int i = 0; i < reps; i++) {
            StringBuilder sb = new StringBuilder();
            sb.append("this is a reasonably short string that doesn't actually need any formatting");
        }
    }

    public void timeFormatter_OneInt(int reps) {
        Integer value = Integer.valueOf(1024); // We're not trying to benchmark boxing here.
        for (int i = 0; i < reps; i++) {
            Formatter f = new Formatter();
            f.format("this is a reasonably short string that has an int %d in it", value);
        }
    }

    public void timeFormatter_OneIntArabic(int reps) {
        Locale arabic = new Locale("ar");
        Integer value = Integer.valueOf(1024); // We're not trying to benchmark boxing here.
        for (int i = 0; i < reps; i++) {
            Formatter f = new Formatter();
            f.format(arabic, "this is a reasonably short string that has an int %d in it", value);
        }
    }

    public void timeStringBuilder_OneInt(int reps) {
        for (int i = 0; i < reps; i++) {
            StringBuilder sb = new StringBuilder();
            sb.append("this is a reasonably short string that has an int ");
            sb.append(1024);
            sb.append(" in it");
        }
    }

    public void timeFormatter_OneHexInt(int reps) {
        Integer value = Integer.valueOf(1024); // We're not trying to benchmark boxing here.
        for (int i = 0; i < reps; i++) {
            Formatter f = new Formatter();
            f.format("this is a reasonably short string that has an int %x in it", value);
        }
    }

    public void timeStringBuilder_OneHexInt(int reps) {
        for (int i = 0; i < reps; i++) {
            StringBuilder sb = new StringBuilder();
            sb.append("this is a reasonably short string that has an int ");
            sb.append(Integer.toHexString(1024));
            sb.append(" in it");
        }
    }

    public void timeFormatter_OneFloat(int reps) {
        Float value = Float.valueOf(10.24f); // We're not trying to benchmark boxing here.
        for (int i = 0; i < reps; i++) {
            Formatter f = new Formatter();
            f.format("this is a reasonably short string that has a float %f in it", value);
        }
    }

    public void timeFormatter_OneFloat_dot2f(int reps) {
        Float value = Float.valueOf(10.24f); // We're not trying to benchmark boxing here.
        for (int i = 0; i < reps; i++) {
            Formatter f = new Formatter();
            f.format("this is a reasonably short string that has a float %.2f in it", value);
        }
    }

    public void timeFormatter_TwoFloats(int reps) {
        Float value = Float.valueOf(10.24f); // We're not trying to benchmark boxing here.
        for (int i = 0; i < reps; i++) {
            Formatter f = new Formatter();
            f.format("this is a reasonably short string that has two floats %f and %f in it", value, value);
        }
    }

    public void timeStringBuilder_OneFloat(int reps) {
        for (int i = 0; i < reps; i++) {
            StringBuilder sb = new StringBuilder();
            sb.append("this is a reasonably short string that has a float ");
            sb.append(10.24f);
            sb.append(" in it");
        }
    }

    public void timeFormatter_OneString(int reps) {
        for (int i = 0; i < reps; i++) {
            Formatter f = new Formatter();
            f.format("this is a reasonably short string that has a string %s in it", "hello");
        }
    }

    public void timeStringBuilder_OneString(int reps) {
        for (int i = 0; i < reps; i++) {
            StringBuilder sb = new StringBuilder();
            sb.append("this is a reasonably short string that has a string ");
            sb.append("hello");
            sb.append(" in it");
        }
    }
}
