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
import java.io.ByteArrayInputStream;
import java.net.URL;
import java.security.Principal;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSessionContext;

/**
 * This benchmark makes a real HTTP connection to a handful of hosts and
 * captures the served certificates as a byte array. It then verifies each
 * certificate in the benchmark loop, being careful to convert from the
 * byte[] to the certificate each time. Otherwise the certificate class
 * caches previous results which skews the results of the benchmark: In practice
 * each certificate instance is verified once and then released.
 */
public final class HostnameVerifierBenchmark {

    @Param({"android.clients.google.com",
            "m.google.com",
            "www.google.com",
            "www.amazon.com",
            "www.ubs.com"}) String host;

    private String hostname;
    private HostnameVerifier hostnameVerifier;
    private byte[][] encodedCertificates;

    @BeforeExperiment
    protected void setUp() throws Exception {
        URL url = new URL("https", host, "/");
        hostnameVerifier = HttpsURLConnection.getDefaultHostnameVerifier();
        HttpsURLConnection connection = (HttpsURLConnection) url.openConnection();
        connection.setHostnameVerifier(new HostnameVerifier() {
            public boolean verify(String hostname, SSLSession sslSession) {
                try {
                    encodedCertificates = certificatesToBytes(sslSession.getPeerCertificates());
                } catch (Exception e) {
                    throw new RuntimeException(e);
                }
                HostnameVerifierBenchmark.this.hostname = hostname;
                return true;
            }
        });
        connection.getInputStream();
        connection.disconnect();
    }

    public void timeVerify(int reps) throws Exception {
        for (int i = 0; i < reps; i++) {
            final Certificate[] certificates = bytesToCertificates(encodedCertificates);
            FakeSSLSession sslSession = new FakeSSLSession() {
                @Override public Certificate[] getPeerCertificates() {
                    return certificates;
                }
            };
            hostnameVerifier.verify(hostname, sslSession);
        }
    }

    private byte[][] certificatesToBytes(Certificate[] certificates) throws Exception {
        byte[][] result = new byte[certificates.length][];
        for (int i = 0, certificatesLength = certificates.length; i < certificatesLength; i++) {
            result[i] = certificates[i].getEncoded();
        }
        return result;
    }

    private Certificate[] bytesToCertificates(byte[][] encodedCertificates) throws Exception {
        CertificateFactory certificateFactory = CertificateFactory.getInstance("X.509");
        Certificate[] result = new Certificate[encodedCertificates.length];
        for (int i = 0; i < encodedCertificates.length; i++) {
            result[i] = certificateFactory.generateCertificate(
                new ByteArrayInputStream(encodedCertificates[i]));
        }
        return result;
    }

    private static class FakeSSLSession implements SSLSession {
        public int getApplicationBufferSize() {
            throw new UnsupportedOperationException();
        }
        public String getCipherSuite() {
            throw new UnsupportedOperationException();
        }
        public long getCreationTime() {
            throw new UnsupportedOperationException();
        }
        public byte[] getId() {
            throw new UnsupportedOperationException();
        }
        public long getLastAccessedTime() {
            throw new UnsupportedOperationException();
        }
        public Certificate[] getLocalCertificates() {
            throw new UnsupportedOperationException();
        }
        public Principal getLocalPrincipal() {
            throw new UnsupportedOperationException();
        }
        public int getPacketBufferSize() {
            throw new UnsupportedOperationException();
        }
        public javax.security.cert.X509Certificate[] getPeerCertificateChain() {
            throw new UnsupportedOperationException();
        }
        public Certificate[] getPeerCertificates() {
            throw new UnsupportedOperationException();
        }
        public String getPeerHost() {
            throw new UnsupportedOperationException();
        }
        public int getPeerPort() {
            throw new UnsupportedOperationException();
        }
        public Principal getPeerPrincipal() {
            throw new UnsupportedOperationException();
        }
        public String getProtocol() {
            throw new UnsupportedOperationException();
        }
        public SSLSessionContext getSessionContext() {
            throw new UnsupportedOperationException();
        }
        public Object getValue(String name) {
            throw new UnsupportedOperationException();
        }
        public String[] getValueNames() {
            throw new UnsupportedOperationException();
        }
        public void invalidate() {
            throw new UnsupportedOperationException();
        }
        public boolean isValid() {
            throw new UnsupportedOperationException();
        }
        public void putValue(String name, Object value) {
            throw new UnsupportedOperationException();
        }
        public void removeValue(String name) {
            throw new UnsupportedOperationException();
        }
    }
}
