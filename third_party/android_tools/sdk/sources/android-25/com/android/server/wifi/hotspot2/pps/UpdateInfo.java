package com.android.server.wifi.hotspot2.pps;

import android.util.Base64;

import com.android.server.wifi.hotspot2.Utils;
import com.android.server.wifi.hotspot2.omadm.OMAException;
import com.android.server.wifi.hotspot2.omadm.OMANode;
import com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager;

import java.nio.charset.StandardCharsets;

import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_CertSHA256Fingerprint;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_CertURL;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_Password;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_Restriction;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_TrustRoot;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_URI;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_UpdateInterval;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_UpdateMethod;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_Username;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_UsernamePassword;

public class UpdateInfo {
    public enum UpdateRestriction {HomeSP, RoamingPartner, Unrestricted}

    private final long mInterval;
    private final boolean mSPPClientInitiated;
    private final UpdateRestriction mUpdateRestriction;
    private final String mURI;
    private final String mUsername;
    private final String mPassword;
    private final String mCertURL;
    private final String mCertFP;

    public UpdateInfo(OMANode policyUpdate) throws OMAException {
        mInterval = PasspointManagementObjectManager.getLong(policyUpdate, TAG_UpdateInterval, null)
                * PasspointManagementObjectManager.IntervalFactor;
        mSPPClientInitiated = PasspointManagementObjectManager.getSelection(policyUpdate,
                TAG_UpdateMethod);
        mUpdateRestriction =
                PasspointManagementObjectManager.getSelection(policyUpdate, TAG_Restriction);
        mURI = PasspointManagementObjectManager.getString(policyUpdate, TAG_URI);

        OMANode unp = policyUpdate.getChild(TAG_UsernamePassword);
        if (unp != null) {
            mUsername = PasspointManagementObjectManager.getString(unp.getChild(TAG_Username));
            String pw = PasspointManagementObjectManager.getString(unp.getChild(TAG_Password));
            mPassword = new String(Base64.decode(pw.getBytes(StandardCharsets.US_ASCII),
                    Base64.DEFAULT), StandardCharsets.UTF_8);
        }
        else {
            mUsername = null;
            mPassword = null;
        }

        OMANode trustRoot = PasspointManagementObjectManager.getChild(policyUpdate, TAG_TrustRoot);
        mCertURL = PasspointManagementObjectManager.getString(trustRoot, TAG_CertURL);
        mCertFP = PasspointManagementObjectManager.getString(trustRoot, TAG_CertSHA256Fingerprint);
    }

    public long getInterval() {
        return mInterval;
    }

    public boolean isSPPClientInitiated() {
        return mSPPClientInitiated;
    }

    public UpdateRestriction getUpdateRestriction() {
        return mUpdateRestriction;
    }

    public String getURI() {
        return mURI;
    }

    public String getUsername() {
        return mUsername;
    }

    public String getPassword() {
        return mPassword;
    }

    public String getCertURL() {
        return mCertURL;
    }

    public String getCertFP() {
        return mCertFP;
    }

    @Override
    public String toString() {
        return "UpdateInfo{" +
                "interval=" + Utils.toHMS(mInterval) +
                ", SPPClientInitiated=" + mSPPClientInitiated +
                ", updateRestriction=" + mUpdateRestriction +
                ", URI='" + mURI + '\'' +
                ", username='" + mUsername + '\'' +
                ", password=" + mPassword +
                ", certURL='" + mCertURL + '\'' +
                ", certFP='" + mCertFP + '\'' +
                '}';
    }
}
