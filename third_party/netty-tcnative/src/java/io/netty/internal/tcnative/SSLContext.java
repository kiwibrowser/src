/*
 * Copyright 2016 The Netty Project
 *
 * The Netty Project licenses this file to you under the Apache License,
 * version 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package io.netty.internal.tcnative;

public final class SSLContext {

    private SSLContext() { }

    /**
     * Initialize new SSL context
     * @param protocol The SSL protocol to use. It can be any combination of
     * the following:
     * <PRE>
     * {@link SSL#SSL_PROTOCOL_SSLV2}
     * {@link SSL#SSL_PROTOCOL_SSLV3}
     * {@link SSL#SSL_PROTOCOL_TLSV1}
     * {@link SSL#SSL_PROTOCOL_TLSV1_1}
     * {@link SSL#SSL_PROTOCOL_TLSV1_2}
     * {@link SSL#SSL_PROTOCOL_ALL} ( == all TLS versions, no SSL)
     * </PRE>
     * @param mode SSL mode to use
     * <PRE>
     * SSL_MODE_CLIENT
     * SSL_MODE_SERVER
     * SSL_MODE_COMBINED
     * </PRE>
     * @return the SSLContext struct
     * @throws Exception if an error happened
     */
    public static native long make(int protocol, int mode)
        throws Exception;

    /**
     * Free the resources used by the Context
     * @param ctx Server or Client context to free.
     * @return APR Status code.
     */
    public static native int free(long ctx);

    /**
     * Set Session context id. Usually host:port combination.
     * @param ctx Context to use.
     * @param id  String that uniquely identifies this context.
     */
    public static native void setContextId(long ctx, String id);

    /**
     * Set OpenSSL Option.
     * @param ctx Server or Client context to use.
     * @param options  See SSL.SSL_OP_* for option flags.
     */
    public static native void setOptions(long ctx, int options);

    /**
     * Get OpenSSL Option.
     * @param ctx Server or Client context to use.
     * @return options  See SSL.SSL_OP_* for option flags.
     */
    public static native int getOptions(long ctx);

    /**
     * Clears OpenSSL Options.
     * @param ctx Server or Client context to use.
     * @param options  See SSL.SSL_OP_* for option flags.
     */
    public static native void clearOptions(long ctx, int options);

    /**
     * Cipher Suite available for negotiation in SSL handshake.
     * <br>
     * This complex directive uses a colon-separated cipher-spec string consisting
     * of OpenSSL cipher specifications to configure the Cipher Suite the client
     * is permitted to negotiate in the SSL handshake phase. Notice that this
     * directive can be used both in per-server and per-directory context.
     * In per-server context it applies to the standard SSL handshake when a
     * connection is established. In per-directory context it forces a SSL
     * renegotiation with the reconfigured Cipher Suite after the HTTP request
     * was read but before the HTTP response is sent.
     * @param ctx Server or Client context to use.
     * @param ciphers An SSL cipher specification.
     * @return {@code true} if successful
     * @throws Exception if an error happened
     */
    public static native boolean setCipherSuite(long ctx, String ciphers) throws Exception;

    /**
     * Set File of PEM-encoded Server CA Certificates
     * <br>
     * This directive sets the optional all-in-one file where you can assemble the
     * certificates of Certification Authorities (CA) which form the certificate
     * chain of the server certificate. This starts with the issuing CA certificate
     * of of the server certificate and can range up to the root CA certificate.
     * Such a file is simply the concatenation of the various PEM-encoded CA
     * Certificate files, usually in certificate chain order.
     * <br>
     * But be careful: Providing the certificate chain works only if you are using
     * a single (either RSA or DSA) based server certificate. If you are using a
     * coupled RSA+DSA certificate pair, this will work only if actually both
     * certificates use the same certificate chain. Else the browsers will be
     * confused in this situation.
     * @param ctx Server or Client context to use.
     * @param file File of PEM-encoded Server CA Certificates.
     * @param skipfirst Skip first certificate if chain file is inside
     *                  certificate file.
     * @return {@code true} if successful
     */
    public static native boolean setCertificateChainFile(long ctx, String file, boolean skipfirst);
    /**
     * Set BIO of PEM-encoded Server CA Certificates
     * <p>
     * This directive sets the optional all-in-one file where you can assemble the
     * certificates of Certification Authorities (CA) which form the certificate
     * chain of the server certificate. This starts with the issuing CA certificate
     * of of the server certificate and can range up to the root CA certificate.
     * Such a file is simply the concatenation of the various PEM-encoded CA
     * Certificate files, usually in certificate chain order.
     * <p>
     * But be careful: Providing the certificate chain works only if you are using
     * a single (either RSA or DSA) based server certificate. If you are using a
     * coupled RSA+DSA certificate pair, this will work only if actually both
     * certificates use the same certificate chain. Otherwsie the browsers will be
     * confused in this situation.
     * @param ctx Server or Client context to use.
     * @param bio BIO of PEM-encoded Server CA Certificates.
     * @param skipfirst Skip first certificate if chain file is inside
     *                  certificate file.
     * @return {@code true} if successful
     */
    public static native boolean setCertificateChainBio(long ctx, long bio, boolean skipfirst);

    /**
     * Set Certificate
     * <p>
     * Point setCertificateFile at a PEM encoded certificate.  If
     * the certificate is encrypted, then you will be prompted for a
     * pass phrase.  Note that a kill -HUP will prompt again. A test
     * certificate can be generated with `make certificate' under
     * built time. Keep in mind that if you've both a RSA and a DSA
     * certificate you can configure both in parallel (to also allow
     * the use of DSA ciphers, etc.)
     * <p>
     * If the key is not combined with the certificate, use key param
     * to point at the key file.  Keep in mind that if
     * you've both a RSA and a DSA private key you can configure
     * both in parallel (to also allow the use of DSA ciphers, etc.)
     * @param ctx Server or Client context to use.
     * @param cert Certificate file.
     * @param key Private Key file to use if not in cert.
     * @param password Certificate password. If null and certificate
     *                 is encrypted, password prompt will be displayed.
     * @return {@code true} if successful
     * @throws Exception if an error happened
     */
    public static native boolean setCertificate(long ctx, String cert, String key, String password) throws Exception;

    /**
     * Set Certificate
     * <p>
     * Point setCertificate at a PEM encoded certificate stored in a BIO. If
     * the certificate is encrypted, then you will be prompted for a
     * pass phrase.  Note that a kill -HUP will prompt again. A test
     * certificate can be generated with `make certificate' under
     * built time. Keep in mind that if you've both a RSA and a DSA
     * certificate you can configure both in parallel (to also allow
     * the use of DSA ciphers, etc.)
     * <p>
     * If the key is not combined with the certificate, use key param
     * to point at the key file.  Keep in mind that if
     * you've both a RSA and a DSA private key you can configure
     * both in parallel (to also allow the use of DSA ciphers, etc.)
     * @param ctx Server or Client context to use.
     * @param certBio Certificate BIO.
     * @param keyBio Private Key BIO to use if not in cert.
     * @param password Certificate password. If null and certificate
     *                 is encrypted, password prompt will be displayed.
     * @return {@code true} if successful
     * @throws Exception if an error happened
     */
    public static native boolean setCertificateBio(long ctx, long certBio, long keyBio, String password) throws Exception;

    /**
     * Set the size of the internal session cache.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_set_cache_size.html">man SSL_CTX_sess_set_cache_size</a>
     * @param ctx Server or Client context to use.
     * @param size the size of the cache
     * @return the previous set value
     */
    public static native long setSessionCacheSize(long ctx, long size);

    /**
     * Get the size of the internal session cache.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_get_cache_size.html">man SSL_CTX_sess_get_cache_size</a>
     * @param ctx Server or Client context to use.
     * @return the current value
     */
    public static native long getSessionCacheSize(long ctx);

    /**
     * Set the timeout for the internal session cache in seconds.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_set_timeout.html">man SSL_CTX_set_timeout</a>
     * @param ctx Server or Client context to use.
     * @param timeoutSeconds the timeout of the cache
     * @return the previous set value
     */
    public static native long setSessionCacheTimeout(long ctx, long timeoutSeconds);

    /**
     * Get the timeout for the internal session cache in seconds.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_get_timeout.html">man SSL_CTX_get_timeout</a>
     * @param ctx Server or Client context to use
     * @return the current value
     */
    public static native long getSessionCacheTimeout(long ctx);

    /**
     * Set the mode of the internal session cache and return the previous used mode.
     * @param ctx Server or Client context to use
     * @param mode the mode of the cache
     * @return the previous set value
     */
    public static native long setSessionCacheMode(long ctx, long mode);

    /**
     * Get the mode of the current used internal session cache.
     *
     * @param ctx Server or Client context to use
     * @return the current mode
     */
    public static native long getSessionCacheMode(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionAccept(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionAcceptGood(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionAcceptRenegotiate(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionCacheFull(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionCbHits(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionConnect(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionConnectGood(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionConnectRenegotiate(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionHits(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionMisses(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionNumber(long ctx);

    /**
     * Session resumption statistics methods.
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_sess_number.html">man SSL_CTX_sess_number</a>
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionTimeouts(long ctx);

    /**
     * TLS session ticket key resumption statistics.
     *
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionTicketKeyNew(long ctx);

    /**
     * TLS session ticket key resumption statistics.
     *
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionTicketKeyResume(long ctx);

    /**
     * TLS session ticket key resumption statistics.
     *
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionTicketKeyRenew(long ctx);

    /**
     * TLS session ticket key resumption statistics.
     *
     * @param ctx Server or Client context to use
     * @return the current number
     */
    public static native long sessionTicketKeyFail(long ctx);

    /**
     * Set TLS session ticket keys.
     *
     * <p> The first key in the list is the primary key. Tickets dervied from the other keys
     * in the list will be accepted but updated to a new ticket using the primary key. This
     * is useful for implementing ticket key rotation.
     * See <a href="https://tools.ietf.org/html/rfc5077">RFC 5077</a>
     *
     * @param ctx Server or Client context to use
     * @param keys the {@link SessionTicketKey}s
     */
    public static void setSessionTicketKeys(long ctx, SessionTicketKey[] keys) {
        if (keys == null || keys.length == 0) {
            throw new IllegalArgumentException("Length of the keys should be longer than 0.");
        }
        byte[] binaryKeys = new byte[keys.length * SessionTicketKey.TICKET_KEY_SIZE];
        for (int i = 0; i < keys.length; i++) {
            SessionTicketKey key = keys[i];
            int dstCurPos = SessionTicketKey.TICKET_KEY_SIZE * i;
            System.arraycopy(key.name, 0, binaryKeys, dstCurPos, SessionTicketKey.NAME_SIZE);
            dstCurPos += SessionTicketKey.NAME_SIZE;
            System.arraycopy(key.hmacKey, 0, binaryKeys, dstCurPos, SessionTicketKey.HMAC_KEY_SIZE);
            dstCurPos += SessionTicketKey.HMAC_KEY_SIZE;
            System.arraycopy(key.aesKey, 0, binaryKeys, dstCurPos, SessionTicketKey.AES_KEY_SIZE);
        }
        setSessionTicketKeys0(ctx, binaryKeys);
    }

    /**
     * Set TLS session keys.
     */
    private static native void setSessionTicketKeys0(long ctx, byte[] keys);

    /**
     * Set concatenated PEM-encoded CA Certificates for Client Auth
     * <br>
     * This directive sets the all-in-one BIO where you can assemble the
     * Certificates of Certification Authorities (CA) whose clients you deal with.
     * These are used for Client Authentication. Such a BIO is simply the
     * concatenation of the various PEM-encoded Certificate files, in order of
     * preference. This can be used alternatively and/or additionally to
     * path.
     * <br>
     * @param ctx Server context to use.
     * @param certBio Directory of PEM-encoded CA Certificates for Client Auth.
     * @return {@code true} if successful, {@code false} otherwise.
     */
    public static native boolean setCACertificateBio(long ctx, long certBio);

    /**
     * Set Type of Client Certificate verification and Maximum depth of CA Certificates
     * in Client Certificate verification.
     * <br>
     * This directive sets the Certificate verification level for the Client
     * Authentication. Notice that this directive can be used both in per-server
     * and per-directory context. In per-server context it applies to the client
     * authentication process used in the standard SSL handshake when a connection
     * is established. In per-directory context it forces a SSL renegotiation with
     * the reconfigured client verification level after the HTTP request was read
     * but before the HTTP response is sent.
     * <br>
     * The following levels are available for level:
     * <ul>
     * <li>{@link SSL#SSL_CVERIFY_IGNORED} - The level is ignored. Only depth will change.</li>
     * <li>{@link SSL#SSL_CVERIFY_NONE} - No client Certificate is required at all</li>
     * <li>{@link SSL#SSL_CVERIFY_OPTIONAL} - The client may present a valid Certificate</li>
     * <li>{@link SSL#SSL_CVERIFY_REQUIRED} - The client has to present a valid Certificate</li>
     * </ul>
     * The depth actually is the maximum number of intermediate certificate issuers,
     * i.e. the number of CA certificates which are max allowed to be followed while
     * verifying the client certificate. A depth of 0 means that self-signed client
     * certificates are accepted only, the default depth of 1 means the client
     * certificate can be self-signed or has to be signed by a CA which is directly
     * known to the server (i.e. the CA's certificate is under
     * <code>setCACertificatePath</code>), etc.
     * @param ctx Server or Client context to use.
     * @param level Type of Client Certificate verification.
     * @param depth Maximum depth of CA Certificates in Client Certificate
     *              verification.
     */
    public static native void setVerify(long ctx, int level, int depth);

    /**
     * Allow to hook {@link CertificateVerifier} into the handshake processing.
     * This will call {@code SSL_CTX_set_cert_verify_callback} and so replace the default verification
     * callback used by openssl
     * @param ctx Server or Client context to use.
     * @param verifier the verifier to call during handshake.
     */
    public static native void setCertVerifyCallback(long ctx, CertificateVerifier verifier);

    /**
     * Allow to hook {@link CertificateRequestedCallback} into the certificate choosing process.
     * This will call {@code SSL_CTX_set_client_cert_cb} and so replace the default verification
     * callback used by openssl
     * @param ctx Server or Client context to use.
     * @param callback the callback to call during certificate selection.
     */
    public static native void setCertRequestedCallback(long ctx, CertificateRequestedCallback callback);

    /**
     * Set next protocol for next protocol negotiation extension
     * @param ctx Server context to use.
     * @param nextProtos protocols in priority order
     * @param selectorFailureBehavior see {@link SSL#SSL_SELECTOR_FAILURE_NO_ADVERTISE}
     *                                and {@link SSL#SSL_SELECTOR_FAILURE_CHOOSE_MY_LAST_PROTOCOL}
     */
    public static native void setNpnProtos(long ctx, String[] nextProtos, int selectorFailureBehavior);

    /**
     * Set application layer protocol for application layer protocol negotiation extension
     * @param ctx Server context to use.
     * @param alpnProtos protocols in priority order
     * @param selectorFailureBehavior see {@link SSL#SSL_SELECTOR_FAILURE_NO_ADVERTISE}
     *                                and {@link SSL#SSL_SELECTOR_FAILURE_CHOOSE_MY_LAST_PROTOCOL}
     */
    public static native void setAlpnProtos(long ctx, String[] alpnProtos, int selectorFailureBehavior);

    /**
     * Set length of the DH to use.
     *
     * @param ctx Server context to use.
     * @param length the length.
     */
    public static native void setTmpDHLength(long ctx, int length);

    /**
     * Set the context within which session be reused (server side only).
     * See <a href="https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_set_session_id_context.html">man SSL_CTX_set_session_id_context</a>
     *
     * @param ctx Server context to use.
     * @param sidCtx can be any kind of binary data, it is therefore possible to use e.g. the name
     *               of the application and/or the hostname and/or service name
     * @return {@code true} if success, {@code false} otherwise.
     */
    public static native boolean setSessionIdContext(long ctx, byte[] sidCtx);

    /**
     * Call SSL_CTX_set_mode
     *
     * @param ctx context to use
     * @param mode the mode
     * @return the set mode.
     */
    public static native int setMode(long ctx, int mode);

    /**
     * Call SSL_CTX_get_mode
     *
     * @param ctx context to use
     * @return the mode.
     */
    public static native int getMode(long ctx);
}
