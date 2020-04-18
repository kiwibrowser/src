package com.android.server.wifi.configparse;

import android.content.Context;
import android.net.Uri;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiEnterpriseConfig;
import android.provider.DocumentsContract;
import android.util.Base64;
import android.util.Log;

import com.android.server.wifi.IMSIParameter;
import com.android.server.wifi.anqp.eap.AuthParam;
import com.android.server.wifi.anqp.eap.EAP;
import com.android.server.wifi.anqp.eap.EAPMethod;
import com.android.server.wifi.anqp.eap.NonEAPInnerAuth;
import com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager;
import com.android.server.wifi.hotspot2.pps.Credential;
import com.android.server.wifi.hotspot2.pps.HomeSP;

import org.xml.sax.SAXException;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.LineNumberReader;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.security.KeyStore;
import java.security.MessageDigest;
import java.security.PrivateKey;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;

public class ConfigBuilder {
    public static final String WifiConfigType = "application/x-wifi-config";
    private static final String ProfileTag = "application/x-passpoint-profile";
    private static final String KeyTag = "application/x-pkcs12";
    private static final String CATag = "application/x-x509-ca-cert";

    private static final String X509 = "X.509";

    private static final String TAG = "WCFG";

    public static WifiConfiguration buildConfig(String uriString, byte[] data, Context context)
            throws IOException, GeneralSecurityException, SAXException {
        Log.d(TAG, "Content: " + (data != null ? data.length : -1));

        byte[] b64 = Base64.decode(new String(data, StandardCharsets.ISO_8859_1), Base64.DEFAULT);
        Log.d(TAG, "Decoded: " + b64.length + " bytes.");

        dropFile(Uri.parse(uriString), context);

        MIMEContainer mimeContainer = new
                MIMEContainer(new LineNumberReader(
                new InputStreamReader(new ByteArrayInputStream(b64), StandardCharsets.ISO_8859_1)),
                null);
        if (!mimeContainer.isBase64()) {
            throw new IOException("Encoding for " +
                    mimeContainer.getContentType() + " is not base64");
        }
        MIMEContainer inner;
        if (mimeContainer.getContentType().equals(WifiConfigType)) {
            byte[] wrappedContent = Base64.decode(mimeContainer.getText(), Base64.DEFAULT);
            Log.d(TAG, "Building container from '" +
                    new String(wrappedContent, StandardCharsets.ISO_8859_1) + "'");
            inner = new MIMEContainer(new LineNumberReader(
                    new InputStreamReader(new ByteArrayInputStream(wrappedContent),
                            StandardCharsets.ISO_8859_1)), null);
        }
        else {
            inner = mimeContainer;
        }
        return parse(inner);
    }

    private static void dropFile(Uri uri, Context context) {
        if (DocumentsContract.isDocumentUri(context, uri)) {
            DocumentsContract.deleteDocument(context.getContentResolver(), uri);
        } else {
            context.getContentResolver().delete(uri, null, null);
        }
    }

    private static WifiConfiguration parse(MIMEContainer root)
            throws IOException, GeneralSecurityException, SAXException {

        if (root.getMimeContainers() == null) {
            throw new IOException("Malformed MIME content: not multipart");
        }

        String moText = null;
        X509Certificate caCert = null;
        PrivateKey clientKey = null;
        List<X509Certificate> clientChain = null;

        for (MIMEContainer subContainer : root.getMimeContainers()) {
            Log.d(TAG, " + Content Type: " + subContainer.getContentType());
            switch (subContainer.getContentType()) {
                case ProfileTag:
                    if (subContainer.isBase64()) {
                        byte[] octets = Base64.decode(subContainer.getText(), Base64.DEFAULT);
                        moText = new String(octets, StandardCharsets.UTF_8);
                    } else {
                        moText = subContainer.getText();
                    }
                    Log.d(TAG, "OMA: " + moText);
                    break;
                case CATag: {
                    if (!subContainer.isBase64()) {
                        throw new IOException("Can't read non base64 encoded cert");
                    }

                    byte[] octets = Base64.decode(subContainer.getText(), Base64.DEFAULT);
                    CertificateFactory factory = CertificateFactory.getInstance(X509);
                    caCert = (X509Certificate) factory.generateCertificate(
                            new ByteArrayInputStream(octets));
                    Log.d(TAG, "Cert subject " + caCert.getSubjectX500Principal());
                    Log.d(TAG, "Full Cert: " + caCert);
                    break;
                }
                case KeyTag: {
                    if (!subContainer.isBase64()) {
                        throw new IOException("Can't read non base64 encoded key");
                    }

                    byte[] octets = Base64.decode(subContainer.getText(), Base64.DEFAULT);

                    KeyStore ks = KeyStore.getInstance("PKCS12");
                    ByteArrayInputStream in = new ByteArrayInputStream(octets);
                    ks.load(in, new char[0]);
                    in.close();
                    Log.d(TAG, "---- Start PKCS12 info " + octets.length + ", size " + ks.size());
                    Enumeration<String> aliases = ks.aliases();
                    while (aliases.hasMoreElements()) {
                        String alias = aliases.nextElement();
                        clientKey = (PrivateKey) ks.getKey(alias, null);
                        Log.d(TAG, "Key: " + clientKey.getFormat());
                        Certificate[] chain = ks.getCertificateChain(alias);
                        if (chain != null) {
                            clientChain = new ArrayList<>();
                            for (Certificate certificate : chain) {
                                if (!(certificate instanceof X509Certificate)) {
                                    Log.w(TAG, "Element in cert chain is not an X509Certificate: " +
                                            certificate.getClass());
                                }
                                clientChain.add((X509Certificate) certificate);
                            }
                            Log.d(TAG, "Chain: " + clientChain.size());
                        }
                    }
                    Log.d(TAG, "---- End PKCS12 info.");
                    break;
                }
            }
        }

        if (moText == null) {
            throw new IOException("Missing profile");
        }

        HomeSP homeSP = PasspointManagementObjectManager.buildSP(moText);

        return buildConfig(homeSP, caCert, clientChain, clientKey);
    }

    private static WifiConfiguration buildConfig(HomeSP homeSP, X509Certificate caCert,
                                                 List<X509Certificate> clientChain, PrivateKey key)
            throws IOException, GeneralSecurityException {

        WifiConfiguration config;

        EAP.EAPMethodID eapMethodID = homeSP.getCredential().getEAPMethod().getEAPMethodID();
        switch (eapMethodID) {
            case EAP_TTLS:
                if (key != null || clientChain != null) {
                    Log.w(TAG, "Client cert and/or key unnecessarily included with EAP-TTLS "+
                            "profile");
                }
                config = buildTTLSConfig(homeSP, caCert);
                break;
            case EAP_TLS:
                config = buildTLSConfig(homeSP, clientChain, key, caCert);
                break;
            case EAP_AKA:
            case EAP_AKAPrim:
            case EAP_SIM:
                if (key != null || clientChain != null || caCert != null) {
                    Log.i(TAG, "Client/CA cert and/or key unnecessarily included with " +
                            eapMethodID + " profile");
                }
                config = buildSIMConfig(homeSP);
                break;
            default:
                throw new IOException("Unsupported EAP Method: " + eapMethodID);
        }

        return config;
    }

    // Retain for debugging purposes
    /*
    private static void xIterateCerts(KeyStore ks, X509Certificate caCert)
            throws GeneralSecurityException {
        Enumeration<String> aliases = ks.aliases();
        while (aliases.hasMoreElements()) {
            String alias = aliases.nextElement();
            Certificate cert = ks.getCertificate(alias);
            Log.d("HS2J", "Checking " + alias);
            if (cert instanceof X509Certificate) {
                X509Certificate x509Certificate = (X509Certificate) cert;
                boolean sm = x509Certificate.getSubjectX500Principal().equals(
                        caCert.getSubjectX500Principal());
                boolean eq = false;
                if (sm) {
                    eq = Arrays.equals(x509Certificate.getEncoded(), caCert.getEncoded());
                }
                Log.d("HS2J", "Subject: " + x509Certificate.getSubjectX500Principal() +
                        ": " + sm + "/" + eq);
            }
        }
    }
    */

    private static void setAnonymousIdentityToNaiRealm(
            WifiConfiguration config, Credential credential) {
        /**
         * Set WPA supplicant's anonymous identity field to a string containing the NAI realm, so
         * that this value will be sent to the EAP server as part of the EAP-Response/ Identity
         * packet. WPA supplicant will reset this field after using it for the EAP-Response/Identity
         * packet, and revert to using the (real) identity field for subsequent transactions that
         * request an identity (e.g. in EAP-TTLS).
         *
         * This NAI realm value (the portion of the identity after the '@') is used to tell the
         * AAA server which AAA/H to forward packets to. The hardcoded username, "anonymous", is a
         * placeholder that is not used--it is set to this value by convention. See Section 5.1 of
         * RFC3748 for more details.
         *
         * NOTE: we do not set this value for EAP-SIM/AKA/AKA', since the EAP server expects the
         * EAP-Response/Identity packet to contain an actual, IMSI-based identity, in order to
         * identify the device.
         */
        config.enterpriseConfig.setAnonymousIdentity("anonymous@" + credential.getRealm());
    }

    private static WifiConfiguration buildTTLSConfig(HomeSP homeSP, X509Certificate caCert)
            throws IOException {
        Credential credential = homeSP.getCredential();

        if (credential.getUserName() == null || credential.getPassword() == null) {
            throw new IOException("EAP-TTLS provisioned without user name or password");
        }

        EAPMethod eapMethod = credential.getEAPMethod();

        AuthParam authParam = eapMethod.getAuthParam();
        if (authParam == null ||
                authParam.getAuthInfoID() != EAP.AuthInfoID.NonEAPInnerAuthType) {
            throw new IOException("Bad auth parameter for EAP-TTLS: " + authParam);
        }

        WifiConfiguration config = buildBaseConfiguration(homeSP);
        NonEAPInnerAuth ttlsParam = (NonEAPInnerAuth) authParam;
        WifiEnterpriseConfig enterpriseConfig = config.enterpriseConfig;
        enterpriseConfig.setPhase2Method(remapInnerMethod(ttlsParam.getType()));
        enterpriseConfig.setIdentity(credential.getUserName());
        enterpriseConfig.setPassword(credential.getPassword());
        enterpriseConfig.setCaCertificate(caCert);

        setAnonymousIdentityToNaiRealm(config, credential);

        return config;
    }

    private static WifiConfiguration buildTLSConfig(HomeSP homeSP,
                                                    List<X509Certificate> clientChain,
                                                    PrivateKey clientKey,
                                                    X509Certificate caCert)
            throws IOException, GeneralSecurityException {

        Credential credential = homeSP.getCredential();

        X509Certificate clientCertificate = null;

        if (clientKey == null || clientChain == null) {
            throw new IOException("No key and/or cert passed for EAP-TLS");
        }
        if (credential.getCertType() != Credential.CertType.x509v3) {
            throw new IOException("Invalid certificate type for TLS: " +
                    credential.getCertType());
        }

        byte[] reference = credential.getFingerPrint();
        MessageDigest digester = MessageDigest.getInstance("SHA-256");
        for (X509Certificate certificate : clientChain) {
            digester.reset();
            byte[] fingerprint = digester.digest(certificate.getEncoded());
            if (Arrays.equals(reference, fingerprint)) {
                clientCertificate = certificate;
                break;
            }
        }
        if (clientCertificate == null) {
            throw new IOException("No certificate in chain matches supplied fingerprint");
        }

        String alias = Base64.encodeToString(reference, Base64.DEFAULT);

        WifiConfiguration config = buildBaseConfiguration(homeSP);
        WifiEnterpriseConfig enterpriseConfig = config.enterpriseConfig;
        enterpriseConfig.setClientCertificateAlias(alias);
        enterpriseConfig.setClientKeyEntry(clientKey, clientCertificate);
        enterpriseConfig.setCaCertificate(caCert);

        setAnonymousIdentityToNaiRealm(config, credential);

        return config;
    }

    private static WifiConfiguration buildSIMConfig(HomeSP homeSP)
            throws IOException {

        Credential credential = homeSP.getCredential();
        IMSIParameter credImsi = credential.getImsi();

        /*
         * Uncomment to enforce strict IMSI matching with currently installed SIM cards.
         *
        TelephonyManager tm = TelephonyManager.from(context);
        SubscriptionManager sub = SubscriptionManager.from(context);
        boolean match = false;

        for (int subId : sub.getActiveSubscriptionIdList()) {
            String imsi = tm.getSubscriberId(subId);
            if (credImsi.matches(imsi)) {
                match = true;
                break;
            }
        }
        if (!match) {
            throw new IOException("Supplied IMSI does not match any SIM card");
        }
        */

        WifiConfiguration config = buildBaseConfiguration(homeSP);
        config.enterpriseConfig.setPlmn(credImsi.toString());
        return config;
    }

    private static WifiConfiguration buildBaseConfiguration(HomeSP homeSP) throws IOException {
        EAP.EAPMethodID eapMethodID = homeSP.getCredential().getEAPMethod().getEAPMethodID();

        WifiConfiguration config = new WifiConfiguration();

        config.FQDN = homeSP.getFQDN();

        HashSet<Long> roamingConsortiumIds = homeSP.getRoamingConsortiums();
        config.roamingConsortiumIds = new long[roamingConsortiumIds.size()];
        int i = 0;
        for (long id : roamingConsortiumIds) {
            config.roamingConsortiumIds[i] = id;
            i++;
        }
        config.providerFriendlyName = homeSP.getFriendlyName();

        config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_EAP);
        config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.IEEE8021X);

        WifiEnterpriseConfig enterpriseConfig = new WifiEnterpriseConfig();
        enterpriseConfig.setEapMethod(remapEAPMethod(eapMethodID));
        enterpriseConfig.setRealm(homeSP.getCredential().getRealm());
        config.enterpriseConfig = enterpriseConfig;
        // The framework based config builder only ever builds r1 configs:
        config.updateIdentifier = null;

        return config;
    }

    private static int remapEAPMethod(EAP.EAPMethodID eapMethodID) throws IOException {
        switch (eapMethodID) {
            case EAP_TTLS:
                return WifiEnterpriseConfig.Eap.TTLS;
            case EAP_TLS:
                return WifiEnterpriseConfig.Eap.TLS;
            case EAP_SIM:
                return WifiEnterpriseConfig.Eap.SIM;
            case EAP_AKA:
                return WifiEnterpriseConfig.Eap.AKA;
            case EAP_AKAPrim:
                return WifiEnterpriseConfig.Eap.AKA_PRIME;
            default:
                throw new IOException("Bad EAP method: " + eapMethodID);
        }
    }

    private static int remapInnerMethod(NonEAPInnerAuth.NonEAPType type) throws IOException {
        switch (type) {
            case PAP:
                return WifiEnterpriseConfig.Phase2.PAP;
            case MSCHAP:
                return WifiEnterpriseConfig.Phase2.MSCHAP;
            case MSCHAPv2:
                return WifiEnterpriseConfig.Phase2.MSCHAPV2;
            case CHAP:
            default:
                throw new IOException("Inner method " + type + " not supported");
        }
    }
}
