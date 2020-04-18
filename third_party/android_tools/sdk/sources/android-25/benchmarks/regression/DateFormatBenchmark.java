/*
 * Copyright (C) 2016 Google Inc.
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

import com.google.caliper.BeforeExperiment;

import java.text.DateFormat;
import java.util.Locale;

public final class DateFormatBenchmark {

    private Locale locale1;
    private Locale locale2;
    private Locale locale3;
    private Locale locale4;

    @BeforeExperiment
    protected void setUp() throws Exception {
        locale1 = Locale.TAIWAN;
        locale2 = Locale.GERMANY;
        locale3 = Locale.FRANCE;
        locale4 = Locale.ITALY;
    }

    public void timeGetDateTimeInstance(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            DateFormat.getDateTimeInstance();
        }
    }

    public void timeGetDateTimeInstance_multiple(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT, locale1);
            DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT, locale2);
            DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT, locale3);
            DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT, locale4);
        }
    }
}
