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
import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.security.spec.AlgorithmParameterSpec;
import javax.crypto.Cipher;
import javax.crypto.CipherInputStream;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;

/**
 * CipherInputStream benchmark.
 */
public class CipherInputStreamBenchmark {

    private static final int DATA_SIZE = 1024 * 1024;
    private static final byte[] DATA = new byte[DATA_SIZE];

    private static final int IV_SIZE = 16;
    private static final byte[] IV = new byte[IV_SIZE];

    static {
        for (int i = 0; i < DATA_SIZE; i++) {
            DATA[i] = (byte) i;
        }
        for (int i = 0; i < IV_SIZE; i++) {
            IV[i] = (byte) i;
        }
    }

    private SecretKey key;

    private byte[] output = new byte[8192];

    private Cipher cipherEncrypt;

    private AlgorithmParameterSpec spec;

    @BeforeExperiment
    protected void setUp() throws Exception {
        KeyGenerator generator = KeyGenerator.getInstance("AES");
        generator.init(128);
        key = generator.generateKey();

        spec = new IvParameterSpec(IV);

        cipherEncrypt = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipherEncrypt.init(Cipher.ENCRYPT_MODE, key, spec);
    }

    public void timeEncrypt(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            cipherEncrypt.init(Cipher.ENCRYPT_MODE, key, spec);
            InputStream is = new CipherInputStream(new ByteArrayInputStream(DATA), cipherEncrypt);
            while (is.read(output) != -1) {
            }
        }
    }
}
