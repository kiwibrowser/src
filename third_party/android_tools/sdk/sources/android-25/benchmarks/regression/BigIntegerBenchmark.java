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

import java.math.BigInteger;
import java.util.Random;

public class BigIntegerBenchmark {
    public void timeRandomDivision(int reps) throws Exception {
        Random r = new Random();
        BigInteger x = new BigInteger(1024, r);
        BigInteger y = new BigInteger(1024, r);
        for (int i = 0; i < reps; ++i) {
            x.divide(y);
        }
    }

    public void timeRandomGcd(int reps) throws Exception {
        Random r = new Random();
        BigInteger x = new BigInteger(1024, r);
        BigInteger y = new BigInteger(1024, r);
        for (int i = 0; i < reps; ++i) {
            x.gcd(y);
        }
    }

    public void timeRandomMultiplication(int reps) throws Exception {
        Random r = new Random();
        BigInteger x = new BigInteger(1024, r);
        BigInteger y = new BigInteger(1024, r);
        for (int i = 0; i < reps; ++i) {
            x.multiply(y);
        }
    }
}
