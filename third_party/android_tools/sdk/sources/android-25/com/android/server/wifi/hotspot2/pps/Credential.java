package com.android.server.wifi.hotspot2.pps;

import android.net.wifi.WifiEnterpriseConfig;
import android.security.Credentials;
import android.security.KeyStore;
import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;

import com.android.server.wifi.IMSIParameter;
import com.android.server.wifi.anqp.eap.EAP;
import com.android.server.wifi.anqp.eap.EAPMethod;
import com.android.server.wifi.anqp.eap.NonEAPInnerAuth;
import com.android.server.wifi.hotspot2.Utils;
import com.android.server.wifi.hotspot2.omadm.OMAException;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.util.Arrays;

public class Credential {
    public enum CertType {IEEE, x509v3}

    public static final String CertTypeX509 = "x509v3";
    public static final String CertTypeIEEE = "802.1ar";

    private final long mCtime;
    private final long mExpTime;
    private final String mRealm;
    private final boolean mCheckAAACert;

    private final String mUserName;
    private final String mPassword;
    private final boolean mDisregardPassword;
    private final boolean mMachineManaged;
    private final String mSTokenApp;
    private final boolean mShare;
    private final EAPMethod mEAPMethod;

    private final CertType mCertType;
    private final byte[] mFingerPrint;

    private final IMSIParameter mImsi;

    public Credential(long ctime, long expTime, String realm, boolean checkAAACert,
                      EAPMethod eapMethod, String userName, String password,
                      boolean machineManaged, String stApp, boolean share) {
        mCtime = ctime;
        mExpTime = expTime;
        mRealm = realm;
        mCheckAAACert = checkAAACert;
        mEAPMethod = eapMethod;
        mUserName = userName;

        if (!TextUtils.isEmpty(password)) {
            byte[] pwOctets = Base64.decode(password, Base64.DEFAULT);
            mPassword = new String(pwOctets, StandardCharsets.UTF_8);
        } else {
            mPassword = null;
        }
        mDisregardPassword = false;

        mMachineManaged = machineManaged;
        mSTokenApp = stApp;
        mShare = share;

        mCertType = null;
        mFingerPrint = null;

        mImsi = null;
    }

    public Credential(long ctime, long expTime, String realm, boolean checkAAACert,
                      EAPMethod eapMethod, Credential.CertType certType, byte[] fingerPrint) {
        mCtime = ctime;
        mExpTime = expTime;
        mRealm = realm;
        mCheckAAACert = checkAAACert;
        mEAPMethod = eapMethod;
        mCertType = certType;
        mFingerPrint = fingerPrint;

        mUserName = null;
        mPassword = null;
        mDisregardPassword = false;
        mMachineManaged = false;
        mSTokenApp = null;
        mShare = false;

        mImsi = null;
    }

    public Credential(long ctime, long expTime, String realm, boolean checkAAACert,
                      EAPMethod eapMethod, IMSIParameter imsi) {
        mCtime = ctime;
        mExpTime = expTime;
        mRealm = realm;
        mCheckAAACert = checkAAACert;
        mEAPMethod = eapMethod;
        mImsi = imsi;

        mCertType = null;
        mFingerPrint = null;

        mUserName = null;
        mPassword = null;
        mDisregardPassword = false;
        mMachineManaged = false;
        mSTokenApp = null;
        mShare = false;
    }

    public Credential(Credential other, String password) {
        mCtime = other.mCtime;
        mExpTime = other.mExpTime;
        mRealm = other.mRealm;
        mCheckAAACert = other.mCheckAAACert;
        mUserName = other.mUserName;
        mPassword = password;
        mDisregardPassword = other.mDisregardPassword;
        mMachineManaged = other.mMachineManaged;
        mSTokenApp = other.mSTokenApp;
        mShare = other.mShare;
        mEAPMethod = other.mEAPMethod;
        mCertType = other.mCertType;
        mFingerPrint = other.mFingerPrint;
        mImsi = other.mImsi;
    }

    public Credential(WifiEnterpriseConfig enterpriseConfig, KeyStore keyStore, boolean update)
            throws IOException {
        mCtime = Utils.UNSET_TIME;
        mExpTime = Utils.UNSET_TIME;
        mRealm = enterpriseConfig.getRealm();
        mCheckAAACert = false;
        mEAPMethod = mapEapMethod(enterpriseConfig.getEapMethod(),
                enterpriseConfig.getPhase2Method());
        mCertType = mEAPMethod.getEAPMethodID() == EAP.EAPMethodID.EAP_TLS ? CertType.x509v3 : null;
        byte[] fingerPrint;

        if (enterpriseConfig.getClientCertificate() != null) {
            // !!! Not sure this will be true in any practical instances:
            try {
                MessageDigest digester = MessageDigest.getInstance("SHA-256");
                fingerPrint = digester.digest(enterpriseConfig.getClientCertificate().getEncoded());
            } catch (GeneralSecurityException gse) {
                Log.e(Utils.hs2LogTag(getClass()),
                        "Failed to generate certificate fingerprint: " + gse);
                fingerPrint = null;
            }
        } else if (enterpriseConfig.getClientCertificateAlias() != null) {
            String alias = enterpriseConfig.getClientCertificateAlias();
            byte[] octets = keyStore.get(Credentials.USER_CERTIFICATE + alias);
            if (octets != null) {
                try {
                    MessageDigest digester = MessageDigest.getInstance("SHA-256");
                    fingerPrint = digester.digest(octets);
                } catch (GeneralSecurityException gse) {
                    Log.e(Utils.hs2LogTag(getClass()), "Failed to construct digest: " + gse);
                    fingerPrint = null;
                }
            } else // !!! The current alias is *not* derived from the fingerprint...
            {
                try {
                    fingerPrint = Base64.decode(enterpriseConfig.getClientCertificateAlias(),
                            Base64.DEFAULT);
                } catch (IllegalArgumentException ie) {
                    Log.e(Utils.hs2LogTag(getClass()), "Bad base 64 alias");
                    fingerPrint = null;
                }
            }
        } else {
            fingerPrint = null;
        }
        mFingerPrint = fingerPrint;
        String imsi = enterpriseConfig.getPlmn();
        mImsi = imsi == null || imsi.length() == 0 ? null : new IMSIParameter(imsi);
        mUserName = enterpriseConfig.getIdentity();
        mPassword = enterpriseConfig.getPassword();
        mDisregardPassword = update && mPassword.length() < 2;
        mMachineManaged = false;
        mSTokenApp = null;
        mShare = false;
    }

    public static CertType mapCertType(String certType) throws OMAException {
        if (certType.equalsIgnoreCase(CertTypeX509)) {
            return CertType.x509v3;
        } else if (certType.equalsIgnoreCase(CertTypeIEEE)) {
            return CertType.IEEE;
        } else {
            throw new OMAException("Invalid cert type: '" + certType + "'");
        }
    }

    private static EAPMethod mapEapMethod(int eapMethod, int phase2Method) throws IOException {
        switch (eapMethod) {
            case WifiEnterpriseConfig.Eap.TLS:
                return new EAPMethod(EAP.EAPMethodID.EAP_TLS, null);
            case WifiEnterpriseConfig.Eap.TTLS:
            /* keep this table in sync with WifiEnterpriseConfig.Phase2 enum */
                NonEAPInnerAuth inner;
                switch (phase2Method) {
                    case WifiEnterpriseConfig.Phase2.PAP:
                        inner = new NonEAPInnerAuth(NonEAPInnerAuth.NonEAPType.PAP);
                        break;
                    case WifiEnterpriseConfig.Phase2.MSCHAP:
                        inner = new NonEAPInnerAuth(NonEAPInnerAuth.NonEAPType.MSCHAP);
                        break;
                    case WifiEnterpriseConfig.Phase2.MSCHAPV2:
                        inner = new NonEAPInnerAuth(NonEAPInnerAuth.NonEAPType.MSCHAPv2);
                        break;
                    default:
                        throw new IOException("TTLS phase2 method " +
                                phase2Method + " not valid for Passpoint");
                }
                return new EAPMethod(EAP.EAPMethodID.EAP_TTLS, inner);
            case WifiEnterpriseConfig.Eap.SIM:
                return new EAPMethod(EAP.EAPMethodID.EAP_SIM, null);
            case WifiEnterpriseConfig.Eap.AKA:
                return new EAPMethod(EAP.EAPMethodID.EAP_AKA, null);
            case WifiEnterpriseConfig.Eap.AKA_PRIME:
                return new EAPMethod(EAP.EAPMethodID.EAP_AKAPrim, null);
            default:
                String methodName;
                if (eapMethod >= 0 && eapMethod < WifiEnterpriseConfig.Eap.strings.length) {
                    methodName = WifiEnterpriseConfig.Eap.strings[eapMethod];
                } else {
                    methodName = Integer.toString(eapMethod);
                }
                throw new IOException("EAP method id " + methodName + " is not valid for Passpoint");
        }
    }

    public EAPMethod getEAPMethod() {
        return mEAPMethod;
    }

    public String getRealm() {
        return mRealm;
    }

    public IMSIParameter getImsi() {
        return mImsi;
    }

    public String getUserName() {
        return mUserName;
    }

    public String getPassword() {
        return mPassword;
    }

    public boolean hasDisregardPassword() {
        return mDisregardPassword;
    }

    public CertType getCertType() {
        return mCertType;
    }

    public byte[] getFingerPrint() {
        return mFingerPrint;
    }

    public long getCtime() {
        return mCtime;
    }

    public long getExpTime() {
        return mExpTime;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        Credential that = (Credential) o;

        if (mCheckAAACert != that.mCheckAAACert) return false;
        if (mCtime != that.mCtime) return false;
        if (mExpTime != that.mExpTime) return false;
        if (mMachineManaged != that.mMachineManaged) return false;
        if (mShare != that.mShare) return false;
        if (mCertType != that.mCertType) return false;
        if (!mEAPMethod.equals(that.mEAPMethod)) return false;
        if (!Arrays.equals(mFingerPrint, that.mFingerPrint)) return false;
        if (!safeEquals(mImsi, that.mImsi)) {
            return false;
        }

        if (!mDisregardPassword && !safeEquals(mPassword, that.mPassword)) {
            return false;
        }

        if (!mRealm.equals(that.mRealm)) return false;
        if (!safeEquals(mSTokenApp, that.mSTokenApp)) {
            return false;
        }
        if (!safeEquals(mUserName, that.mUserName)) {
            return false;
        }

        return true;
    }

    private static boolean safeEquals(Object s1, Object s2) {
        if (s1 == null) {
            return s2 == null;
        }
        else {
            return s2 != null && s1.equals(s2);
        }
    }

    @Override
    public int hashCode() {
        int result = (int) (mCtime ^ (mCtime >>> 32));
        result = 31 * result + (int) (mExpTime ^ (mExpTime >>> 32));
        result = 31 * result + mRealm.hashCode();
        result = 31 * result + (mCheckAAACert ? 1 : 0);
        result = 31 * result + (mUserName != null ? mUserName.hashCode() : 0);
        result = 31 * result + (mPassword != null ? mPassword.hashCode() : 0);
        result = 31 * result + (mMachineManaged ? 1 : 0);
        result = 31 * result + (mSTokenApp != null ? mSTokenApp.hashCode() : 0);
        result = 31 * result + (mShare ? 1 : 0);
        result = 31 * result + mEAPMethod.hashCode();
        result = 31 * result + (mCertType != null ? mCertType.hashCode() : 0);
        result = 31 * result + (mFingerPrint != null ? Arrays.hashCode(mFingerPrint) : 0);
        result = 31 * result + (mImsi != null ? mImsi.hashCode() : 0);
        return result;
    }

    @Override
    public String toString() {
        return "Credential{" +
                "mCtime=" + Utils.toUTCString(mCtime) +
                ", mExpTime=" + Utils.toUTCString(mExpTime) +
                ", mRealm='" + mRealm + '\'' +
                ", mCheckAAACert=" + mCheckAAACert +
                ", mUserName='" + mUserName + '\'' +
                ", mPassword='" + mPassword + '\'' +
                ", mDisregardPassword=" + mDisregardPassword +
                ", mMachineManaged=" + mMachineManaged +
                ", mSTokenApp='" + mSTokenApp + '\'' +
                ", mShare=" + mShare +
                ", mEAPMethod=" + mEAPMethod +
                ", mCertType=" + mCertType +
                ", mFingerPrint=" + Utils.toHexString(mFingerPrint) +
                ", mImsi='" + mImsi + '\'' +
                '}';
    }
}
