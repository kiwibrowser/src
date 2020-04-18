/*
 * Copyright 2017 The Netty Project
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
package io.netty.internal.tcnative;

/**
 * This class is necessary to break the following cyclic dependency:
 * <ol>
 * <li>JNI_OnLoad</li>
 * <li>JNI Calls FindClass because RegisterNatives (used to register JNI methods) requires a class</li>
 * <li>FindClass loads the class, but static members variables of that class attempt to call a JNI method which has not
 * yet been registered.</li>
 * <li>{@link java.lang.UnsatisfiedLinkError} is thrown because native method has not yet been registered.</li>
 * </ol>
 * <strong>Static members which call JNI methods must not be declared in this class!</strong>
 */
final class NativeStaticallyReferencedJniMethods {
    private NativeStaticallyReferencedJniMethods() {
    }

    /**
     * Options that may impact security and may be set by default as defined in:
     * <a href="https://www.openssl.org/docs/man1.0.1/ssl/SSL_CTX_set_options.html">SSL Docs</a>.
     */
    static native int sslOpCipherServerPreference();
    static native int sslOpNoSSLv2();
    static native int sslOpNoSSLv3();
    static native int sslOpNoTLSv1();
    static native int sslOpNoTLSv11();
    static native int sslOpNoTLSv12();
    static native int sslOpNoTicket();

    /**
     * Options not defined in the OpenSSL docs but may impact security.
     */
    static native int sslOpNoCompression();

    /* Only support OFF and SERVER for now */
    static native int sslSessCacheOff();
    static native int sslSessCacheServer();

    static native int sslStConnect();
    static native int sslStAccept();

    static native int sslModeEnablePartialWrite();
    static native int sslModeAcceptMovingWriteBuffer();
    static native int sslModeReleaseBuffers();

    static native int sslSendShutdown();
    static native int sslReceivedShutdown();
    static native int sslErrorNone();
    static native int sslErrorSSL();
    static native int sslErrorWantRead();
    static native int sslErrorWantWrite();
    static native int sslErrorWantX509Lookup();
    static native int sslErrorSyscall();
    static native int sslErrorZeroReturn();
    static native int sslErrorWantConnect();
    static native int sslErrorWantAccept();

    static native int x509CheckFlagAlwaysCheckSubject();
    static native int x509CheckFlagDisableWildCards();
    static native int x509CheckFlagNoPartialWildCards();
    static native int x509CheckFlagMultiLabelWildCards();

    /* x509 certificate verification errors */
    static native int x509vOK();
    static native int x509vErrUnspecified();
    static native int x509vErrUnableToGetIssuerCert();
    static native int x509vErrUnableToGetCrl();
    static native int x509vErrUnableToDecryptCertSignature();
    static native int x509vErrUnableToDecryptCrlSignature();
    static native int x509vErrUnableToDecodeIssuerPublicKey();
    static native int x509vErrCertSignatureFailure();
    static native int x509vErrCrlSignatureFailure();
    static native int x509vErrCertNotYetValid();
    static native int x509vErrCertHasExpired();
    static native int x509vErrCrlNotYetValid();
    static native int x509vErrCrlHasExpired();
    static native int x509vErrErrorInCertNotBeforeField();
    static native int x509vErrErrorInCertNotAfterField();
    static native int x509vErrErrorInCrlLastUpdateField();
    static native int x509vErrErrorInCrlNextUpdateField();
    static native int x509vErrOutOfMem();
    static native int x509vErrDepthZeroSelfSignedCert();
    static native int x509vErrSelfSignedCertInChain();
    static native int x509vErrUnableToGetIssuerCertLocally();
    static native int x509vErrUnableToVerifyLeafSignature();
    static native int x509vErrCertChainTooLong();
    static native int x509vErrCertRevoked();
    static native int x509vErrInvalidCa();
    static native int x509vErrPathLengthExceeded();
    static native int x509vErrInvalidPurpose();
    static native int x509vErrCertUntrusted();
    static native int x509vErrCertRejected();
    static native int x509vErrSubjectIssuerMismatch();
    static native int x509vErrAkidSkidMismatch();
    static native int x509vErrAkidIssuerSerialMismatch();
    static native int x509vErrKeyUsageNoCertSign();
    static native int x509vErrUnableToGetCrlIssuer();
    static native int x509vErrUnhandledCriticalExtension();
    static native int x509vErrKeyUsageNoCrlSign();
    static native int x509vErrUnhandledCriticalCrlExtension();
    static native int x509vErrInvalidNonCa();
    static native int x509vErrProxyPathLengthExceeded();
    static native int x509vErrKeyUsageNoDigitalSignature();
    static native int x509vErrProxyCertificatesNotAllowed();
    static native int x509vErrInvalidExtension();
    static native int x509vErrInvalidPolicyExtension();
    static native int x509vErrNoExplicitPolicy();
    static native int x509vErrDifferntCrlScope();
    static native int x509vErrUnsupportedExtensionFeature();
    static native int x509vErrUnnestedResource();
    static native int x509vErrPermittedViolation();
    static native int x509vErrExcludedViolation();
    static native int x509vErrSubtreeMinMax();
    static native int x509vErrApplicationVerification();
    static native int x509vErrUnsupportedConstraintType();
    static native int x509vErrUnsupportedConstraintSyntax();
    static native int x509vErrUnsupportedNameSyntax();
    static native int x509vErrCrlPathValidationError();
    static native int x509vErrPathLoop();
    static native int x509vErrSuiteBInvalidVersion();
    static native int x509vErrSuiteBInvalidAlgorithm();
    static native int x509vErrSuiteBInvalidCurve();
    static native int x509vErrSuiteBInvalidSignatureAlgorithm();
    static native int x509vErrSuiteBLosNotAllowed();
    static native int x509vErrSuiteBCannotSignP384WithP256();
    static native int x509vErrHostnameMismatch();
    static native int x509vErrEmailMismatch();
    static native int x509vErrIpAddressMismatch();
    static native int x509vErrDaneNoMatch();
}
