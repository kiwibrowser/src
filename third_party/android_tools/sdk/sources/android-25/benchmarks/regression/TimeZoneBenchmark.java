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

import java.util.TimeZone;

public class TimeZoneBenchmark {
    public void timeTimeZone_getDefault(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            TimeZone.getDefault();
        }
    }

    public void timeTimeZone_getTimeZoneUTC(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            TimeZone.getTimeZone("UTC");
        }
    }

    public void timeTimeZone_getTimeZone_default(int reps) throws Exception {
        String defaultId = TimeZone.getDefault().getID();
        for (int rep = 0; rep < reps; ++rep) {
            TimeZone.getTimeZone(defaultId);
        }
    }

    // A time zone with relatively few transitions.
    public void timeTimeZone_getTimeZone_America_Caracas(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            TimeZone.getTimeZone("America/Caracas");
        }
    }

    // A time zone with a lot of transitions.
    public void timeTimeZone_getTimeZone_America_Santiago(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            TimeZone.getTimeZone("America/Santiago");
        }
    }

    public void timeTimeZone_getTimeZone_GMT_plus_10(int reps) throws Exception {
        for (int rep = 0; rep < reps; ++rep) {
            TimeZone.getTimeZone("GMT+10");
        }
    }
}
