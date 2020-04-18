/*
 * Copyright 2014 The Netty Project
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

import static io.netty.internal.tcnative.NativeStaticallyReferencedJniMethods.*;

/**
 * Is called during handshake and hooked into openssl via {@code SSL_CTX_set_cert_verify_callback}.
 *
 * IMPORTANT: Implementations of this interface should be static as it is stored as a global reference via JNI. This
 *            means if you use an inner / anonymous class to implement this and also depend on the finalizer of the
 *            class to free up the SSLContext the finalizer will never run as the object is never GC, due the hard
 *            reference to the enclosing class. This will most likely result in a memory leak.
 */
public interface CertificateVerifier {
    int X509_V_OK = x509vOK();
    int X509_V_ERR_UNSPECIFIED = x509vErrUnspecified();
    int X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT = x509vErrUnableToGetIssuerCert();
    int X509_V_ERR_UNABLE_TO_GET_CRL = x509vErrUnableToGetCrl();
    int X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE = x509vErrUnableToDecryptCertSignature();
    int X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE = x509vErrUnableToDecryptCrlSignature();
    int X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY = x509vErrUnableToDecodeIssuerPublicKey();
    int X509_V_ERR_CERT_SIGNATURE_FAILURE = x509vErrCertSignatureFailure();
    int X509_V_ERR_CRL_SIGNATURE_FAILURE = x509vErrCrlSignatureFailure();
    int X509_V_ERR_CERT_NOT_YET_VALID = x509vErrCertNotYetValid();
    int X509_V_ERR_CERT_HAS_EXPIRED = x509vErrCertHasExpired();
    int X509_V_ERR_CRL_NOT_YET_VALID = x509vErrCrlNotYetValid();
    int X509_V_ERR_CRL_HAS_EXPIRED = x509vErrCrlHasExpired();
    int X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD = x509vErrErrorInCertNotBeforeField();
    int X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD = x509vErrErrorInCertNotAfterField();
    int X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD = x509vErrErrorInCrlLastUpdateField();
    int X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD = x509vErrErrorInCrlNextUpdateField();
    int X509_V_ERR_OUT_OF_MEM = x509vErrOutOfMem();
    int X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT = x509vErrDepthZeroSelfSignedCert();
    int X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN = x509vErrSelfSignedCertInChain();
    int X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY = x509vErrUnableToGetIssuerCertLocally();
    int X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE  = x509vErrUnableToVerifyLeafSignature();
    int X509_V_ERR_CERT_CHAIN_TOO_LONG = x509vErrCertChainTooLong();
    int X509_V_ERR_CERT_REVOKED = x509vErrCertRevoked();
    int X509_V_ERR_INVALID_CA = x509vErrInvalidCa();
    int X509_V_ERR_PATH_LENGTH_EXCEEDED = x509vErrPathLengthExceeded();
    int X509_V_ERR_INVALID_PURPOSE = x509vErrInvalidPurpose();
    int X509_V_ERR_CERT_UNTRUSTED = x509vErrCertUntrusted();
    int X509_V_ERR_CERT_REJECTED = x509vErrCertRejected();
    int X509_V_ERR_SUBJECT_ISSUER_MISMATCH = x509vErrSubjectIssuerMismatch();
    int X509_V_ERR_AKID_SKID_MISMATCH = x509vErrAkidSkidMismatch();
    int X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH = x509vErrAkidIssuerSerialMismatch();
    int X509_V_ERR_KEYUSAGE_NO_CERTSIGN = x509vErrKeyUsageNoCertSign();
    int X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER = x509vErrUnableToGetCrlIssuer();
    int X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION = x509vErrUnhandledCriticalExtension();
    int X509_V_ERR_KEYUSAGE_NO_CRL_SIGN = x509vErrKeyUsageNoCrlSign();
    int X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION = x509vErrUnhandledCriticalCrlExtension();
    int X509_V_ERR_INVALID_NON_CA = x509vErrInvalidNonCa();
    int X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED = x509vErrProxyPathLengthExceeded();
    int X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE = x509vErrKeyUsageNoDigitalSignature();
    int X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED = x509vErrProxyCertificatesNotAllowed();
    int X509_V_ERR_INVALID_EXTENSION = x509vErrInvalidExtension();
    int X509_V_ERR_INVALID_POLICY_EXTENSION = x509vErrInvalidPolicyExtension();
    int X509_V_ERR_NO_EXPLICIT_POLICY = x509vErrNoExplicitPolicy();
    int X509_V_ERR_DIFFERENT_CRL_SCOPE = x509vErrDifferntCrlScope();
    int X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE = x509vErrUnsupportedExtensionFeature();
    int X509_V_ERR_UNNESTED_RESOURCE = x509vErrUnnestedResource();
    int X509_V_ERR_PERMITTED_VIOLATION = x509vErrPermittedViolation();
    int X509_V_ERR_EXCLUDED_VIOLATION  = x509vErrExcludedViolation();
    int X509_V_ERR_SUBTREE_MINMAX = x509vErrSubtreeMinMax();
    int X509_V_ERR_APPLICATION_VERIFICATION = x509vErrApplicationVerification();
    int X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE = x509vErrUnsupportedConstraintType();
    int X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX = x509vErrUnsupportedConstraintSyntax();
    int X509_V_ERR_UNSUPPORTED_NAME_SYNTAX = x509vErrUnsupportedNameSyntax();
    int X509_V_ERR_CRL_PATH_VALIDATION_ERROR = x509vErrCrlPathValidationError();
    int X509_V_ERR_PATH_LOOP = x509vErrPathLoop();
    int X509_V_ERR_SUITE_B_INVALID_VERSION = x509vErrSuiteBInvalidVersion();
    int X509_V_ERR_SUITE_B_INVALID_ALGORITHM = x509vErrSuiteBInvalidAlgorithm();
    int X509_V_ERR_SUITE_B_INVALID_CURVE = x509vErrSuiteBInvalidCurve();
    int X509_V_ERR_SUITE_B_INVALID_SIGNATURE_ALGORITHM = x509vErrSuiteBInvalidSignatureAlgorithm();
    int X509_V_ERR_SUITE_B_LOS_NOT_ALLOWED = x509vErrSuiteBLosNotAllowed();
    int X509_V_ERR_SUITE_B_CANNOT_SIGN_P_384_WITH_P_256 = x509vErrSuiteBCannotSignP384WithP256();
    int X509_V_ERR_HOSTNAME_MISMATCH = x509vErrHostnameMismatch();
    int X509_V_ERR_EMAIL_MISMATCH = x509vErrEmailMismatch();
    int X509_V_ERR_IP_ADDRESS_MISMATCH = x509vErrIpAddressMismatch();
    int X509_V_ERR_DANE_NO_MATCH = x509vErrDaneNoMatch();

    /**
     * Returns {@code true} if the passed in certificate chain could be verified and so the handshake
     * should be successful, {@code false} otherwise.
     *
     * @param ssl               the SSL instance
     * @param x509              the {@code X509} certificate chain
     * @param authAlgorithm     the auth algorithm
     * @return verified         {@code true} if verified successful, {@code false} otherwise
     */
    int verify(long ssl, byte[][] x509, String authAlgorithm);
}
