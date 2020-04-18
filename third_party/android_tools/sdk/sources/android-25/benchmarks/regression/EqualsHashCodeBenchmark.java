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

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Param;
import java.net.URI;
import java.net.URL;

public final class EqualsHashCodeBenchmark {
    private enum Type {
        URI() {
            @Override Object newInstance(String text) throws Exception {
                return new URI(text);
            }
        },
        URL() {
            @Override Object newInstance(String text) throws Exception {
                return new URL(text);
            }
        };
        abstract Object newInstance(String text) throws Exception;
    }

    private static final String QUERY = "%E0%AE%A8%E0%AE%BE%E0%AE%AE%E0%AF%8D+%E0%AE%AE%E0%AF%81%E0%AE%95%E0%AF%8D%E0%AE%95%E0%AE%BF%E0%AE%AF%E0%AE%AE%E0%AE%BE%E0%AE%A9%2C+%E0%AE%9A%E0%AF%81%E0%AE%B5%E0%AE%BE%E0%AE%B0%E0%AE%B8%E0%AF%8D%E0%AE%AF%E0%AE%AE%E0%AE%BE%E0%AE%A9+%E0%AE%87%E0%AE%B0%E0%AF%81%E0%AE%AA%E0%AF%8D%E0%AE%AA%E0%AF%87%E0%AE%BE%E0%AE%AE%E0%AF%8D%2C+%E0%AE%86%E0%AE%A9%E0%AE%BE%E0%AE%B2%E0%AF%8D+%E0%AE%9A%E0%AE%BF%E0%AE%B2+%E0%AE%A8%E0%AF%87%E0%AE%B0%E0%AE%99%E0%AF%8D%E0%AE%95%E0%AE%B3%E0%AE%BF%E0%AE%B2%E0%AF%8D+%E0%AE%9A%E0%AF%82%E0%AE%B4%E0%AF%8D%E0%AE%A8%E0%AE%BF%E0%AE%B2%E0%AF%88+%E0%AE%8F%E0%AE%B1%E0%AF%8D%E0%AE%AA%E0%AE%9F%E0%AF%81%E0%AE%AE%E0%AF%8D+%E0%AE%8E%E0%AE%A9%E0%AF%8D%E0%AE%AA%E0%AE%A4%E0%AE%BE%E0%AE%B2%E0%AF%8D+%E0%AE%AA%E0%AE%A3%E0%AE%BF%E0%AE%AF%E0%AF%88%E0%AE%AF%E0%AF%81%E0%AE%AE%E0%AF%8D+%E0%AE%B5%E0%AE%B2%E0%AE%BF+%E0%AE%85%E0%AE%B5%E0%AE%B0%E0%AF%88+%E0%AE%9A%E0%AE%BF%E0%AE%B2+%E0%AE%AA%E0%AF%86%E0%AE%B0%E0%AE%BF%E0%AE%AF+%E0%AE%95%E0%AF%86%E0%AE%BE%E0%AE%B3%E0%AF%8D%E0%AE%AE%E0%AF%81%E0%AE%A4%E0%AE%B2%E0%AF%8D+%E0%AE%AE%E0%AF%81%E0%AE%9F%E0%AE%BF%E0%AE%AF%E0%AF%81%E0%AE%AE%E0%AF%8D.+%E0%AE%85%E0%AE%A4%E0%AF%81+%E0%AE%9A%E0%AE%BF%E0%AE%B2+%E0%AE%A8%E0%AE%A9%E0%AF%8D%E0%AE%AE%E0%AF%88%E0%AE%95%E0%AE%B3%E0%AF%88+%E0%AE%AA%E0%AF%86%E0%AE%B1+%E0%AE%A4%E0%AE%B5%E0%AE%BF%E0%AE%B0%2C+%E0%AE%8E%E0%AE%AA%E0%AF%8D%E0%AE%AA%E0%AF%87%E0%AE%BE%E0%AE%A4%E0%AF%81%E0%AE%AE%E0%AF%8D+%E0%AE%89%E0%AE%B4%E0%AF%88%E0%AE%95%E0%AF%8D%E0%AE%95+%E0%AE%89%E0%AE%9F%E0%AE%B1%E0%AF%8D%E0%AE%AA%E0%AE%AF%E0%AE%BF%E0%AE%B1%E0%AF%8D%E0%AE%9A%E0%AE%BF+%E0%AE%AE%E0%AF%87%E0%AE%B1%E0%AF%8D%E0%AE%95%E0%AF%86%E0%AE%BE%E0%AE%B3%E0%AF%8D%E0%AE%95%E0%AE%BF%E0%AE%B1%E0%AE%A4%E0%AF%81+%E0%AE%8E%E0%AE%99%E0%AF%8D%E0%AE%95%E0%AE%B3%E0%AF%81%E0%AE%95%E0%AF%8D%E0%AE%95%E0%AF%81+%E0%AE%87%E0%AE%A4%E0%AF%81+%E0%AE%92%E0%AE%B0%E0%AF%81+%E0%AE%9A%E0%AE%BF%E0%AE%B1%E0%AE%BF%E0%AE%AF+%E0%AE%89%E0%AE%A4%E0%AE%BE%E0%AE%B0%E0%AE%A3%E0%AE%AE%E0%AF%8D%2C+%E0%AE%8E%E0%AE%9F%E0%AF%81%E0%AE%95%E0%AF%8D%E0%AE%95.+%E0%AE%B0%E0%AE%AF%E0%AE%BF%E0%AE%B2%E0%AF%8D+%E0%AE%8E%E0%AE%A8%E0%AF%8D%E0%AE%A4+%E0%AE%B5%E0%AE%BF%E0%AE%B3%E0%AF%88%E0%AE%B5%E0%AE%BE%E0%AE%95+%E0%AE%87%E0%AE%A9%E0%AF%8D%E0%AE%AA%E0%AE%AE%E0%AF%8D+%E0%AE%86%E0%AE%A9%E0%AF%8D%E0%AE%B2%E0%AF%88%E0%AE%A9%E0%AF%8D+%E0%AE%AA%E0%AE%AF%E0%AE%A9%E0%AF%8D%E0%AE%AA%E0%AE%BE%E0%AE%9F%E0%AF%81%E0%AE%95%E0%AE%B3%E0%AF%8D+%E0%AE%87%E0%AE%B0%E0%AF%81%E0%AE%95%E0%AF%8D%E0%AE%95+%E0%AE%B5%E0%AF%87%E0%AE%A3%E0%AF%8D%E0%AE%9F%E0%AF%81%E0%AE%AE%E0%AF%8D+%E0%AE%A4%E0%AE%AF%E0%AE%BE%E0%AE%B0%E0%AE%BF%E0%AE%95%E0%AF%8D%E0%AE%95%E0%AF%81%E0%AE%AE%E0%AF%8D+%E0%AE%A4%E0%AE%B5%E0%AE%B1%E0%AF%81+%E0%AE%95%E0%AE%A3%E0%AF%8D%E0%AE%9F%E0%AF%81%E0%AE%AA%E0%AE%BF%E0%AE%9F%E0%AE%BF%E0%AE%95%E0%AF%8D%E0%AE%95+%E0%AE%B5%E0%AE%B0%E0%AF%81%E0%AE%AE%E0%AF%8D+%E0%AE%A8%E0%AE%BE%E0%AE%AE%E0%AF%8D+%E0%AE%A4%E0%AE%B1%E0%AF%8D%E0%AE%AA%E0%AF%87%E0%AE%BE%E0%AE%A4%E0%AF%81+%E0%AE%87%E0%AE%B0%E0%AF%81%E0%AE%95%E0%AF%8D%E0%AE%95%E0%AE%BF%E0%AE%B1%E0%AF%87%E0%AE%BE%E0%AE%AE%E0%AF%8D.+%E0%AE%87%E0%AE%A8%E0%AF%8D%E0%AE%A4+%E0%AE%A8%E0%AE%BF%E0%AE%95%E0%AE%B4%E0%AF%8D%E0%AE%B5%E0%AF%81%E0%AE%95%E0%AE%B3%E0%AE%BF%E0%AE%B2%E0%AF%8D+%E0%AE%9A%E0%AF%86%E0%AE%AF%E0%AF%8D%E0%AE%A4%E0%AE%AA%E0%AE%BF%E0%AE%A9%E0%AF%8D+%E0%AE%85%E0%AE%AE%E0%AF%88%E0%AE%AA%E0%AF%8D%E0%AE%AA%E0%AE%BF%E0%AE%A9%E0%AF%8D+%E0%AE%95%E0%AE%A3%E0%AE%95%E0%AF%8D%E0%AE%95%E0%AF%81%2C+%E0%AE%85%E0%AE%B5%E0%AE%B0%E0%AF%8D%E0%AE%95%E0%AE%B3%E0%AF%8D+%E0%AE%A4%E0%AE%B5%E0%AE%B1%E0%AF%81+%E0%AE%B5%E0%AE%BF%E0%AE%9F%E0%AF%8D%E0%AE%9F%E0%AF%81+quae+%E0%AE%AA%E0%AE%9F%E0%AF%8D%E0%AE%9F%E0%AE%B1%E0%AF%88+%E0%AE%A8%E0%AF%80%E0%AE%99%E0%AF%8D%E0%AE%95%E0%AE%B3%E0%AF%8D+%E0%AE%AA%E0%AE%B0%E0%AE%BF%E0%AE%A8%E0%AF%8D%E0%AE%A4%E0%AF%81%E0%AE%B0%E0%AF%88%E0%AE%95%E0%AF%8D%E0%AE%95%E0%AE%BF%E0%AE%B1%E0%AF%87%E0%AE%BE%E0%AE%AE%E0%AF%8D+%E0%AE%AE%E0%AF%86%E0%AE%A9%E0%AF%8D%E0%AE%AE%E0%AF%88%E0%AE%AF%E0%AE%BE%E0%AE%95+%E0%AE%AE%E0%AE%BE%E0%AE%B1%E0%AF%81%E0%AE%AE%E0%AF%8D";

    @Param Type type;

    Object a1;
    Object a2;
    Object b1;
    Object b2;

    Object c1;
    Object c2;

    @BeforeExperiment
    protected void setUp() throws Exception {
        a1 = type.newInstance("https://mail.google.com/mail/u/0/?shva=1#inbox");
        a2 = type.newInstance("https://mail.google.com/mail/u/0/?shva=1#inbox");
        b1 = type.newInstance("http://developer.android.com/reference/java/net/URI.html");
        b2 = type.newInstance("http://developer.android.com/reference/java/net/URI.html");

        c1 = type.newInstance("http://developer.android.com/query?q=" + QUERY);
        // Replace the very last char.
        c2 = type.newInstance("http://developer.android.com/query?q=" + QUERY.substring(0, QUERY.length() - 3) + "%AF");
    }

    public void timeEquals(int reps) {
        for (int i = 0; i < reps; i+=3) {
            a1.equals(b1);
            a1.equals(a2);
            b1.equals(b2);
        }
    }

    public void timeHashCode(int reps) {
        for (int i = 0; i < reps; i+=2) {
            a1.hashCode();
            b1.hashCode();
        }
    }

    public void timeEqualsWithHeavilyEscapedComponent(int reps) {
        for (int i = 0; i < reps; ++i) {
            c1.equals(c2);
        }
    }
}
