/*
 * Copyright (C) 2012 Google Inc.
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
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.SecureRandom;

public class KeyPairGeneratorBenchmark {
    @Param private Algorithm algorithm;

    public enum Algorithm {
        RSA,
        DSA,
    };

    @Param private Implementation implementation;

    public enum Implementation { OpenSSL, BouncyCastle };

    private String generatorAlgorithm;
    private KeyPairGenerator generator;
    private SecureRandom random;

    @BeforeExperiment
    protected void setUp() throws Exception {
        this.generatorAlgorithm = algorithm.toString();
        
        final String provider;
        if (implementation == Implementation.BouncyCastle) {
            provider = "BC";
        } else {
            provider = "AndroidOpenSSL";
        }

        this.generator = KeyPairGenerator.getInstance(generatorAlgorithm, provider);
        this.random = SecureRandom.getInstance("SHA1PRNG");
        this.generator.initialize(1024);
    }

    public void time(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            KeyPair keyPair = generator.generateKeyPair();
        }
    }
}
