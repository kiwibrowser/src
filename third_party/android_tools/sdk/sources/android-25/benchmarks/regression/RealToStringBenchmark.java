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

public class RealToStringBenchmark {

    private static final float SMALL  = -123.45f;
    private static final float MEDIUM = -123.45e8f;
    private static final float LARGE  = -123.45e36f;

    public void timeFloat_toString_NaN(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Float.toString(Float.NaN);
        }
    }

    public void timeFloat_toString_NEGATIVE_INFINITY(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Float.toString(Float.NEGATIVE_INFINITY);
        }
    }

    public void timeFloat_toString_POSITIVE_INFINITY(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Float.toString(Float.POSITIVE_INFINITY);
        }
    }

    public void timeFloat_toString_zero(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Float.toString(0.0f);
        }
    }

    public void timeFloat_toString_minusZero(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Float.toString(-0.0f);
        }
    }

    public void timeFloat_toString_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Float.toString(SMALL);
        }
    }

    public void timeFloat_toString_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Float.toString(MEDIUM);
        }
    }

    public void timeFloat_toString_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Float.toString(LARGE);
        }
    }

    public void timeStringBuilder_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            new StringBuilder().append(SMALL);
        }
    }

    public void timeStringBuilder_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            new StringBuilder().append(MEDIUM);
        }
    }

    public void timeStringBuilder_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            new StringBuilder().append(LARGE);
        }
    }

    public void timeFormatter_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            String.format("%f", SMALL);
        }
    }

    public void timeFormatter_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            String.format("%f", MEDIUM);
        }
    }

    public void timeFormatter_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            String.format("%f", LARGE);
        }
    }

    public void timeFormatter_dot2f_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            String.format("%.2f", SMALL);
        }
    }

    public void timeFormatter_dot2f_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            String.format("%.2f", MEDIUM);
        }
    }

    public void timeFormatter_dot2f_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            String.format("%.2f", LARGE);
        }
    }
}
