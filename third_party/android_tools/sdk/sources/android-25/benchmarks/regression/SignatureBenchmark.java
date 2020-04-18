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

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Param;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Signature;
import java.util.HashMap;
import java.util.Map;

/**
 * Tests RSA and DSA signature creation and verification.
 */
public class SignatureBenchmark {

    private static final int DATA_SIZE = 8192;
    private static final byte[] DATA = new byte[DATA_SIZE];
    static {
        for (int i = 0; i < DATA_SIZE; i++) {
            DATA[i] = (byte)i;
        }
    }
    @Param private Algorithm algorithm;

    public enum Algorithm {
        MD5WithRSA,
        SHA1WithRSA,
        SHA256WithRSA,
        SHA384WithRSA,
        SHA512WithRSA,
        SHA1withDSA
    };

    @Param private Implementation implementation;

    public enum Implementation { OpenSSL, BouncyCastle };

    // Key generation and signing aren't part of the benchmark for verification
    // so cache the results
    private static Map<String,KeyPair> KEY_PAIRS = new HashMap<String,KeyPair>();
    private static Map<String,byte[]> SIGNATURES = new HashMap<String,byte[]>();

    private String signatureAlgorithm;
    private byte[] signature;
    private PrivateKey privateKey;
    private PublicKey publicKey;

    @BeforeExperiment
    protected void setUp() throws Exception {
        this.signatureAlgorithm = algorithm.toString();

        String keyAlgorithm = signatureAlgorithm.substring(signatureAlgorithm.length() - 3 ,
                                                           signatureAlgorithm.length());
        KeyPair keyPair = KEY_PAIRS.get(keyAlgorithm);
        if (keyPair == null) {
            KeyPairGenerator generator = KeyPairGenerator.getInstance(keyAlgorithm);
            keyPair = generator.generateKeyPair();
            KEY_PAIRS.put(keyAlgorithm, keyPair);
        }
        this.privateKey = keyPair.getPrivate();
        this.publicKey = keyPair.getPublic();

        this.signature = SIGNATURES.get(signatureAlgorithm);
        if (this.signature == null) {
            Signature signer = Signature.getInstance(signatureAlgorithm);
            signer.initSign(keyPair.getPrivate());
            signer.update(DATA);
            this.signature = signer.sign();
            SIGNATURES.put(signatureAlgorithm, signature);
        }
    }

    public void timeSign(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            Signature signer;
            switch (implementation) {
                case OpenSSL:
                    signer = Signature.getInstance(signatureAlgorithm, "AndroidOpenSSL");
                    break;
                case BouncyCastle:
                    signer = Signature.getInstance(signatureAlgorithm, "BC");
                    break;
                default:
                    throw new RuntimeException(implementation.toString());
            }
            signer.initSign(privateKey);
            signer.update(DATA);
            signer.sign();
        }
    }

    public void timeVerify(int reps) throws Exception {
        for (int i = 0; i < reps; ++i) {
            Signature verifier;
            switch (implementation) {
                case OpenSSL:
                    verifier = Signature.getInstance(signatureAlgorithm, "AndroidOpenSSL");
                    break;
                case BouncyCastle:
                    verifier = Signature.getInstance(signatureAlgorithm, "BC");
                    break;
                default:
                    throw new RuntimeException(implementation.toString());
            }
            verifier.initVerify(publicKey);
            verifier.update(DATA);
            verifier.verify(signature);
        }
    }
}
