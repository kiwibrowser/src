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
import java.security.spec.AlgorithmParameterSpec;
import java.util.HashMap;
import java.util.Map;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;

/**
 * Cipher benchmarks. Only runs on AES currently because of the combinatorial
 * explosion of the test as it stands.
 */
public class CipherBenchmark {

    private static final int DATA_SIZE = 8192;
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

    @Param private Algorithm algorithm;

    public enum Algorithm {
        AES,
    };

    @Param private Mode mode;

    public enum Mode {
        CBC,
        CFB,
        CTR,
        ECB,
        OFB,
    };

    @Param private Padding padding;

    public enum Padding {
        NOPADDING,
        PKCS1PADDING,
    };

    @Param({"128", "192", "256"}) private int keySize;

    @Param({"16", "32", "64", "128", "1024", "8192"}) private int inputSize;

    @Param private Implementation implementation;

    public enum Implementation { OpenSSL, BouncyCastle };

    private String providerName;

    // Key generation isn't part of the benchmark so cache the results
    private static Map<Integer, SecretKey> KEY_SIZES = new HashMap<Integer, SecretKey>();

    private String cipherAlgorithm;
    private SecretKey key;

    private byte[] output = new byte[DATA.length];

    private Cipher cipherEncrypt;

    private Cipher cipherDecrypt;

    private AlgorithmParameterSpec spec;

    @BeforeExperiment
    protected void setUp() throws Exception {
        cipherAlgorithm = algorithm.toString() + "/" + mode.toString() + "/"
                + padding.toString();

        String keyAlgorithm = algorithm.toString();
        key = KEY_SIZES.get(keySize);
        if (key == null) {
            KeyGenerator generator = KeyGenerator.getInstance(keyAlgorithm);
            generator.init(keySize);
            key = generator.generateKey();
            KEY_SIZES.put(keySize, key);
        }

        switch (implementation) {
            case OpenSSL:
                providerName = "AndroidOpenSSL";
                break;
            case BouncyCastle:
                providerName = "BC";
                break;
            default:
                throw new RuntimeException(implementation.toString());
        }

        if (mode != Mode.ECB) {
            spec = new IvParameterSpec(IV);
        }

        cipherEncrypt = Cipher.getInstance(cipherAlgorithm, providerName);
        cipherEncrypt.init(Cipher.ENCRYPT_MODE, key, spec);

        cipherDecrypt = Cipher.getInstance(cipherAlgorithm, providerName);
        cipherDecrypt.init(Cipher.DECRYPT_MODE, key, spec);
    }

    public void timeEncrypt(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            cipherEncrypt.doFinal(DATA, 0, inputSize, output);
        }
    }

    public void timeDecrypt(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            cipherDecrypt.doFinal(DATA, 0, inputSize, output);
        }
    }
}
