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

#include "tcn.h"
#include "ssl_private.h"

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslVerifyNone)(TCN_STDARGS) {
    return SSL_VERIFY_NONE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslVerifyPeer)(TCN_STDARGS) {
    return SSL_VERIFY_PEER;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslVerifyFailIfNoPeerCert)(TCN_STDARGS) {
    return SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslVerifyClientOnce)(TCN_STDARGS) {
    return SSL_VERIFY_CLIENT_ONCE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslOpCipherServerPreference)(TCN_STDARGS) {
    return SSL_OP_CIPHER_SERVER_PREFERENCE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslOpNoSSLv2)(TCN_STDARGS) {
    return SSL_OP_NO_SSLv2;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslOpNoSSLv3)(TCN_STDARGS) {
    return SSL_OP_NO_SSLv3;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslOpNoTLSv1)(TCN_STDARGS) {
    return SSL_OP_NO_TLSv1;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslOpNoTLSv11)(TCN_STDARGS) {
    return SSL_OP_NO_TLSv1_1;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslOpNoTLSv12)(TCN_STDARGS) {
    return SSL_OP_NO_TLSv1_2;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslOpNoTicket)(TCN_STDARGS) {
    return SSL_OP_NO_TICKET;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslOpNoCompression)(TCN_STDARGS) {
    return SSL_OP_NO_COMPRESSION;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslSessCacheOff)(TCN_STDARGS) {
    return SSL_SESS_CACHE_OFF;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslSessCacheServer)(TCN_STDARGS) {
    return SSL_SESS_CACHE_SERVER;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslStConnect)(TCN_STDARGS) {
    return SSL_ST_CONNECT;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslStAccept)(TCN_STDARGS) {
    return SSL_ST_ACCEPT;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslModeEnablePartialWrite)(TCN_STDARGS) {
    return SSL_MODE_ENABLE_PARTIAL_WRITE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslModeAcceptMovingWriteBuffer)(TCN_STDARGS) {
    return SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslModeReleaseBuffers)(TCN_STDARGS) {
    return SSL_MODE_RELEASE_BUFFERS;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslSendShutdown)(TCN_STDARGS) {
    return SSL_SENT_SHUTDOWN;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslReceivedShutdown)(TCN_STDARGS) {
    return SSL_RECEIVED_SHUTDOWN;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslErrorNone)(TCN_STDARGS) {
    return SSL_ERROR_NONE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslErrorSSL)(TCN_STDARGS) {
    return SSL_ERROR_SSL;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslErrorWantRead)(TCN_STDARGS) {
    return SSL_ERROR_WANT_READ;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslErrorWantWrite)(TCN_STDARGS) {
    return SSL_ERROR_WANT_WRITE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslErrorWantX509Lookup)(TCN_STDARGS) {
    return SSL_ERROR_WANT_X509_LOOKUP;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslErrorSyscall)(TCN_STDARGS) {
    return SSL_ERROR_SYSCALL;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslErrorZeroReturn)(TCN_STDARGS) {
    return SSL_ERROR_ZERO_RETURN;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslErrorWantConnect)(TCN_STDARGS) {
    return SSL_ERROR_WANT_CONNECT;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, sslErrorWantAccept)(TCN_STDARGS) {
    return SSL_ERROR_WANT_ACCEPT;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509CheckFlagAlwaysCheckSubject)(TCN_STDARGS) {
#ifdef X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT
    return X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT;
#else
    return 0;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509CheckFlagDisableWildCards)(TCN_STDARGS) {
#ifdef X509_CHECK_FLAG_NO_WILD_CARDS
    return X509_CHECK_FLAG_NO_WILD_CARDS;
#else
    return 0;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509CheckFlagNoPartialWildCards)(TCN_STDARGS) {
#ifdef X509_CHECK_FLAG_NO_PARTIAL_WILD_CARDS
    return X509_CHECK_FLAG_NO_PARTIAL_WILD_CARDS;
#else
    return 0;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509CheckFlagMultiLabelWildCards)(TCN_STDARGS) {
#ifdef X509_CHECK_FLAG_MULTI_LABEL_WILDCARDS
    return X509_CHECK_FLAG_MULTI_LABEL_WILDCARDS;
#else
    return 0;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vOK)(TCN_STDARGS) {
    return X509_V_OK;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnspecified)(TCN_STDARGS) {
#ifdef X509_V_ERR_UNSPECIFIED
    return X509_V_ERR_UNSPECIFIED;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnableToGetIssuerCert)(TCN_STDARGS) {
    return X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnableToGetCrl)(TCN_STDARGS) {
    return X509_V_ERR_UNABLE_TO_GET_CRL;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnableToDecryptCertSignature)(TCN_STDARGS) {
    return X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnableToDecryptCrlSignature)(TCN_STDARGS) {
    return X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnableToDecodeIssuerPublicKey)(TCN_STDARGS) {
    return X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCertSignatureFailure)(TCN_STDARGS) {
    return X509_V_ERR_CERT_SIGNATURE_FAILURE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCrlSignatureFailure)(TCN_STDARGS) {
    return X509_V_ERR_CRL_SIGNATURE_FAILURE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCertNotYetValid)(TCN_STDARGS) {
    return X509_V_ERR_CERT_NOT_YET_VALID;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCertHasExpired)(TCN_STDARGS) {
    return X509_V_ERR_CERT_HAS_EXPIRED;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCrlNotYetValid)(TCN_STDARGS) {
    return X509_V_ERR_CRL_NOT_YET_VALID;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCrlHasExpired)(TCN_STDARGS) {
    return X509_V_ERR_CRL_HAS_EXPIRED;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrErrorInCertNotBeforeField)(TCN_STDARGS) {
    return X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrErrorInCertNotAfterField)(TCN_STDARGS) {
    return X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrErrorInCrlLastUpdateField)(TCN_STDARGS) {
    return X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrErrorInCrlNextUpdateField)(TCN_STDARGS) {
    return X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrOutOfMem)(TCN_STDARGS) {
    return X509_V_ERR_OUT_OF_MEM;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrDepthZeroSelfSignedCert)(TCN_STDARGS) {
    return X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrSelfSignedCertInChain)(TCN_STDARGS) {
    return X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnableToGetIssuerCertLocally)(TCN_STDARGS) {
    return X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnableToVerifyLeafSignature)(TCN_STDARGS) {
    return X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCertChainTooLong)(TCN_STDARGS) {
    return X509_V_ERR_CERT_CHAIN_TOO_LONG;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCertRevoked)(TCN_STDARGS) {
    return X509_V_ERR_CERT_REVOKED;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrInvalidCa)(TCN_STDARGS) {
    return X509_V_ERR_INVALID_CA;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrPathLengthExceeded)(TCN_STDARGS) {
    return X509_V_ERR_PATH_LENGTH_EXCEEDED;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrInvalidPurpose)(TCN_STDARGS) {
    return X509_V_ERR_INVALID_PURPOSE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCertUntrusted)(TCN_STDARGS) {
    return X509_V_ERR_CERT_UNTRUSTED;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCertRejected)(TCN_STDARGS) {
    return X509_V_ERR_CERT_REJECTED;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrSubjectIssuerMismatch)(TCN_STDARGS) {
    return X509_V_ERR_SUBJECT_ISSUER_MISMATCH;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrAkidSkidMismatch)(TCN_STDARGS) {
    return X509_V_ERR_AKID_SKID_MISMATCH;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrAkidIssuerSerialMismatch)(TCN_STDARGS) {
    return X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrKeyUsageNoCertSign)(TCN_STDARGS) {
    return X509_V_ERR_KEYUSAGE_NO_CERTSIGN;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnableToGetCrlIssuer)(TCN_STDARGS) {
    return X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnhandledCriticalExtension)(TCN_STDARGS) {
    return X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrKeyUsageNoCrlSign)(TCN_STDARGS) {
    return X509_V_ERR_KEYUSAGE_NO_CRL_SIGN;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnhandledCriticalCrlExtension)(TCN_STDARGS) {
    return X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrInvalidNonCa)(TCN_STDARGS) {
    return X509_V_ERR_INVALID_NON_CA;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrProxyPathLengthExceeded)(TCN_STDARGS) {
    return X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrKeyUsageNoDigitalSignature)(TCN_STDARGS) {
    return X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrProxyCertificatesNotAllowed)(TCN_STDARGS) {
    return X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrInvalidExtension)(TCN_STDARGS) {
    return X509_V_ERR_INVALID_EXTENSION;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrInvalidPolicyExtension)(TCN_STDARGS) {
    return X509_V_ERR_INVALID_POLICY_EXTENSION;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrNoExplicitPolicy)(TCN_STDARGS) {
    return X509_V_ERR_NO_EXPLICIT_POLICY;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrDifferntCrlScope)(TCN_STDARGS) {
    return X509_V_ERR_DIFFERENT_CRL_SCOPE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnsupportedExtensionFeature)(TCN_STDARGS) {
    return X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnnestedResource)(TCN_STDARGS) {
    return X509_V_ERR_UNNESTED_RESOURCE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrPermittedViolation)(TCN_STDARGS) {
    return X509_V_ERR_PERMITTED_VIOLATION;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrExcludedViolation)(TCN_STDARGS) {
    return X509_V_ERR_EXCLUDED_VIOLATION;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrSubtreeMinMax)(TCN_STDARGS) {
    return X509_V_ERR_SUBTREE_MINMAX;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrApplicationVerification)(TCN_STDARGS) {
    return X509_V_ERR_APPLICATION_VERIFICATION;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnsupportedConstraintType)(TCN_STDARGS) {
    return X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnsupportedConstraintSyntax)(TCN_STDARGS) {
    return X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrUnsupportedNameSyntax)(TCN_STDARGS) {
    return X509_V_ERR_UNSUPPORTED_NAME_SYNTAX;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrCrlPathValidationError)(TCN_STDARGS) {
    return X509_V_ERR_CRL_PATH_VALIDATION_ERROR;
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrPathLoop)(TCN_STDARGS) {
#ifdef X509_V_ERR_PATH_LOOP
    return X509_V_ERR_PATH_LOOP;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrSuiteBInvalidVersion)(TCN_STDARGS) {
#ifdef X509_V_ERR_SUITE_B_INVALID_VERSION
    return X509_V_ERR_SUITE_B_INVALID_VERSION;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrSuiteBInvalidAlgorithm)(TCN_STDARGS) {
#ifdef X509_V_ERR_SUITE_B_INVALID_ALGORITHM
    return X509_V_ERR_SUITE_B_INVALID_ALGORITHM;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrSuiteBInvalidCurve)(TCN_STDARGS) {
#ifdef X509_V_ERR_SUITE_B_INVALID_CURVE
    return X509_V_ERR_SUITE_B_INVALID_CURVE;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrSuiteBInvalidSignatureAlgorithm)(TCN_STDARGS) {
#ifdef X509_V_ERR_SUITE_B_INVALID_SIGNATURE_ALGORITHM
    return X509_V_ERR_SUITE_B_INVALID_SIGNATURE_ALGORITHM;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrSuiteBLosNotAllowed)(TCN_STDARGS) {
#ifdef X509_V_ERR_SUITE_B_LOS_NOT_ALLOWED
    return X509_V_ERR_SUITE_B_LOS_NOT_ALLOWED;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrSuiteBCannotSignP384WithP256)(TCN_STDARGS) {
#ifdef X509_V_ERR_SUITE_B_CANNOT_SIGN_P_384_WITH_P_256
    return X509_V_ERR_SUITE_B_CANNOT_SIGN_P_384_WITH_P_256;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrHostnameMismatch)(TCN_STDARGS) {
#ifdef X509_V_ERR_HOSTNAME_MISMATCH
    return X509_V_ERR_HOSTNAME_MISMATCH;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrEmailMismatch)(TCN_STDARGS) {
#ifdef X509_V_ERR_EMAIL_MISMATCH
    return X509_V_ERR_EMAIL_MISMATCH;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrIpAddressMismatch)(TCN_STDARGS) {
#ifdef X509_V_ERR_IP_ADDRESS_MISMATCH
    return X509_V_ERR_IP_ADDRESS_MISMATCH;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}

TCN_IMPLEMENT_CALL(jint, NativeStaticallyReferencedJniMethods, x509vErrDaneNoMatch)(TCN_STDARGS) {
#ifdef X509_V_ERR_DANE_NO_MATCH
    return X509_V_ERR_DANE_NO_MATCH;
#else
    return TCN_X509_V_ERR_UNSPECIFIED;
#endif
}
