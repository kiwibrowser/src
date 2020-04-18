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

public class IntegralToStringBenchmark {

    private static final int SMALL  = 12;
    private static final int MEDIUM = 12345;
    private static final int LARGE  = 12345678;

    public void time_IntegerToString_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(SMALL);
        }
    }

    public void time_IntegerToString_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(MEDIUM);
        }
    }

    public void time_IntegerToString_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(LARGE);
        }
    }

    public void time_IntegerToString2_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(SMALL, 2);
        }
    }

    public void time_IntegerToString2_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(MEDIUM, 2);
        }
    }

    public void time_IntegerToString2_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(LARGE, 2);
        }
    }

    public void time_IntegerToString10_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(SMALL, 10);
        }
    }

    public void time_IntegerToString10_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(MEDIUM, 10);
        }
    }

    public void time_IntegerToString10_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(LARGE, 10);
        }
    }

    public void time_IntegerToString16_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(SMALL, 16);
        }
    }

    public void time_IntegerToString16_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(MEDIUM, 16);
        }
    }

    public void time_IntegerToString16_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toString(LARGE, 16);
        }
    }

    public void time_IntegerToBinaryString_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toBinaryString(SMALL);
        }
    }

    public void time_IntegerToBinaryString_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toBinaryString(MEDIUM);
        }
    }

    public void time_IntegerToBinaryString_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toBinaryString(LARGE);
        }
    }

    public void time_IntegerToHexString_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toHexString(SMALL);
        }
    }

    public void time_IntegerToHexString_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toHexString(MEDIUM);
        }
    }

    public void time_IntegerToHexString_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            Integer.toHexString(LARGE);
        }
    }

    public void time_StringBuilder_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            new StringBuilder().append(SMALL);
        }
    }

    public void time_StringBuilder_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            new StringBuilder().append(MEDIUM);
        }
    }

    public void time_StringBuilder_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            new StringBuilder().append(LARGE);
        }
    }

    public void time_Formatter_small(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            String.format("%d", SMALL);
        }
    }

    public void time_Formatter_medium(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            String.format("%d", MEDIUM);
        }
    }

    public void time_Formatter_large(int reps) {
        for (int rep = 0; rep < reps; ++rep) {
            String.format("%d", LARGE);
        }
    }
}
