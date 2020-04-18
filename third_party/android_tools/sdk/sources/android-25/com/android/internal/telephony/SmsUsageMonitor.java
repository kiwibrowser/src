/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony;

import android.app.AppGlobals;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.res.XmlResourceParser;
import android.database.ContentObserver;
import android.os.Binder;
import android.os.Handler;
import android.os.Process;
import android.os.RemoteException;
import android.os.UserHandle;
import android.provider.Settings;
import android.telephony.PhoneNumberUtils;
import android.util.AtomicFile;
import android.telephony.Rlog;
import android.util.Xml;

import com.android.internal.util.FastXmlSerializer;
import com.android.internal.util.XmlUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.regex.Pattern;

/**
 * Implement the per-application based SMS control, which limits the number of
 * SMS/MMS messages an app can send in the checking period.
 *
 * This code was formerly part of {@link SMSDispatcher}, and has been moved
 * into a separate class to support instantiation of multiple SMSDispatchers on
 * dual-mode devices that require support for both 3GPP and 3GPP2 format messages.
 */
public class SmsUsageMonitor {
    private static final String TAG = "SmsUsageMonitor";
    private static final boolean DBG = false;
    private static final boolean VDBG = false;

    private static final String SHORT_CODE_PATH = "/data/misc/sms/codes";

    /** Default checking period for SMS sent without user permission. */
    private static final int DEFAULT_SMS_CHECK_PERIOD = 60000;      // 1 minute

    /** Default number of SMS sent in checking period without user permission. */
    private static final int DEFAULT_SMS_MAX_COUNT = 30;

    /** Return value from {@link #checkDestination} for regular phone numbers. */
    static final int CATEGORY_NOT_SHORT_CODE = 0;

    /** Return value from {@link #checkDestination} for free (no cost) short codes. */
    static final int CATEGORY_FREE_SHORT_CODE = 1;

    /** Return value from {@link #checkDestination} for standard rate (non-premium) short codes. */
    static final int CATEGORY_STANDARD_SHORT_CODE = 2;

    /** Return value from {@link #checkDestination} for possible premium short codes. */
    static final int CATEGORY_POSSIBLE_PREMIUM_SHORT_CODE = 3;

    /** Return value from {@link #checkDestination} for premium short codes. */
    static final int CATEGORY_PREMIUM_SHORT_CODE = 4;

    /** @hide */
    public static int mergeShortCodeCategories(int type1, int type2) {
        if (type1 > type2) return type1;
        return type2;
    }

    /** Premium SMS permission for a new package (ask user when first premium SMS sent). */
    public static final int PREMIUM_SMS_PERMISSION_UNKNOWN = 0;

    /** Default premium SMS permission (ask user for each premium SMS sent). */
    public static final int PREMIUM_SMS_PERMISSION_ASK_USER = 1;

    /** Premium SMS permission when the owner has denied the app from sending premium SMS. */
    public static final int PREMIUM_SMS_PERMISSION_NEVER_ALLOW = 2;

    /** Premium SMS permission when the owner has allowed the app to send premium SMS. */
    public static final int PREMIUM_SMS_PERMISSION_ALWAYS_ALLOW = 3;

    private final int mCheckPeriod;
    private final int mMaxAllowed;

    private final HashMap<String, ArrayList<Long>> mSmsStamp =
            new HashMap<String, ArrayList<Long>>();

    /** Context for retrieving regexes from XML resource. */
    private final Context mContext;

    /** Country code for the cached short code pattern matcher. */
    private String mCurrentCountry;

    /** Cached short code pattern matcher for {@link #mCurrentCountry}. */
    private ShortCodePatternMatcher mCurrentPatternMatcher;

    /** Notice when the enabled setting changes - can be changed through gservices */
    private final AtomicBoolean mCheckEnabled = new AtomicBoolean(true);

    /** Handler for responding to content observer updates. */
    private final SettingsObserverHandler mSettingsObserverHandler;

    /** File holding the patterns */
    private final File mPatternFile = new File(SHORT_CODE_PATH);

    /** Last modified time for pattern file */
    private long mPatternFileLastModified = 0;

    /** Directory for per-app SMS permission XML file. */
    private static final String SMS_POLICY_FILE_DIRECTORY = "/data/misc/sms";

    /** Per-app SMS permission XML filename. */
    private static final String SMS_POLICY_FILE_NAME = "premium_sms_policy.xml";

    /** XML tag for root element. */
    private static final String TAG_SHORTCODES = "shortcodes";

    /** XML tag for short code patterns for a specific country. */
    private static final String TAG_SHORTCODE = "shortcode";

    /** XML attribute for the country code. */
    private static final String ATTR_COUNTRY = "country";

    /** XML attribute for the short code regex pattern. */
    private static final String ATTR_PATTERN = "pattern";

    /** XML attribute for the premium short code regex pattern. */
    private static final String ATTR_PREMIUM = "premium";

    /** XML attribute for the free short code regex pattern. */
    private static final String ATTR_FREE = "free";

    /** XML attribute for the standard rate short code regex pattern. */
    private static final String ATTR_STANDARD = "standard";

    /** Stored copy of premium SMS package permissions. */
    private AtomicFile mPolicyFile;

    /** Loaded copy of premium SMS package permissions. */
    private final HashMap<String, Integer> mPremiumSmsPolicy = new HashMap<String, Integer>();

    /** XML tag for root element of premium SMS permissions. */
    private static final String TAG_SMS_POLICY_BODY = "premium-sms-policy";

    /** XML tag for a package. */
    private static final String TAG_PACKAGE = "package";

    /** XML attribute for the package name. */
    private static final String ATTR_PACKAGE_NAME = "name";

    /** XML attribute for the package's premium SMS permission (integer type). */
    private static final String ATTR_PACKAGE_SMS_POLICY = "sms-policy";

    /**
     * SMS short code regex pattern matcher for a specific country.
     */
    private static final class ShortCodePatternMatcher {
        private final Pattern mShortCodePattern;
        private final Pattern mPremiumShortCodePattern;
        private final Pattern mFreeShortCodePattern;
        private final Pattern mStandardShortCodePattern;

        ShortCodePatternMatcher(String shortCodeRegex, String premiumShortCodeRegex,
                String freeShortCodeRegex, String standardShortCodeRegex) {
            mShortCodePattern = (shortCodeRegex != null ? Pattern.compile(shortCodeRegex) : null);
            mPremiumShortCodePattern = (premiumShortCodeRegex != null ?
                    Pattern.compile(premiumShortCodeRegex) : null);
            mFreeShortCodePattern = (freeShortCodeRegex != null ?
                    Pattern.compile(freeShortCodeRegex) : null);
            mStandardShortCodePattern = (standardShortCodeRegex != null ?
                    Pattern.compile(standardShortCodeRegex) : null);
        }

        int getNumberCategory(String phoneNumber) {
            if (mFreeShortCodePattern != null && mFreeShortCodePattern.matcher(phoneNumber)
                    .matches()) {
                return CATEGORY_FREE_SHORT_CODE;
            }
            if (mStandardShortCodePattern != null && mStandardShortCodePattern.matcher(phoneNumber)
                    .matches()) {
                return CATEGORY_STANDARD_SHORT_CODE;
            }
            if (mPremiumShortCodePattern != null && mPremiumShortCodePattern.matcher(phoneNumber)
                    .matches()) {
                return CATEGORY_PREMIUM_SHORT_CODE;
            }
            if (mShortCodePattern != null && mShortCodePattern.matcher(phoneNumber).matches()) {
                return CATEGORY_POSSIBLE_PREMIUM_SHORT_CODE;
            }
            return CATEGORY_NOT_SHORT_CODE;
        }
    }

    /**
     * Observe the secure setting for enable flag
     */
    private static class SettingsObserver extends ContentObserver {
        private final Context mContext;
        private final AtomicBoolean mEnabled;

        SettingsObserver(Handler handler, Context context, AtomicBoolean enabled) {
            super(handler);
            mContext = context;
            mEnabled = enabled;
            onChange(false);
        }

        @Override
        public void onChange(boolean selfChange) {
            mEnabled.set(Settings.Global.getInt(mContext.getContentResolver(),
                    Settings.Global.SMS_SHORT_CODE_CONFIRMATION, 1) != 0);
        }
    }

    private static class SettingsObserverHandler extends Handler {
        SettingsObserverHandler(Context context, AtomicBoolean enabled) {
            ContentResolver resolver = context.getContentResolver();
            ContentObserver globalObserver = new SettingsObserver(this, context, enabled);
            resolver.registerContentObserver(Settings.Global.getUriFor(
                    Settings.Global.SMS_SHORT_CODE_CONFIRMATION), false, globalObserver);
        }
    }

    /**
     * Create SMS usage monitor.
     * @param context the context to use to load resources and get TelephonyManager service
     */
    public SmsUsageMonitor(Context context) {
        mContext = context;
        ContentResolver resolver = context.getContentResolver();

        mMaxAllowed = Settings.Global.getInt(resolver,
                Settings.Global.SMS_OUTGOING_CHECK_MAX_COUNT,
                DEFAULT_SMS_MAX_COUNT);

        mCheckPeriod = Settings.Global.getInt(resolver,
                Settings.Global.SMS_OUTGOING_CHECK_INTERVAL_MS,
                DEFAULT_SMS_CHECK_PERIOD);

        mSettingsObserverHandler = new SettingsObserverHandler(mContext, mCheckEnabled);

        loadPremiumSmsPolicyDb();
    }

    /**
     * Return a pattern matcher object for the specified country.
     * @param country the country to search for
     * @return a {@link ShortCodePatternMatcher} for the specified country, or null if not found
     */
    private ShortCodePatternMatcher getPatternMatcherFromFile(String country) {
        FileReader patternReader = null;
        XmlPullParser parser = null;
        try {
            patternReader = new FileReader(mPatternFile);
            parser = Xml.newPullParser();
            parser.setInput(patternReader);
            return getPatternMatcherFromXmlParser(parser, country);
        } catch (FileNotFoundException e) {
            Rlog.e(TAG, "Short Code Pattern File not found");
        } catch (XmlPullParserException e) {
            Rlog.e(TAG, "XML parser exception reading short code pattern file", e);
        } finally {
            mPatternFileLastModified = mPatternFile.lastModified();
            if (patternReader != null) {
                try {
                    patternReader.close();
                } catch (IOException e) {}
            }
        }
        return null;
    }

    private ShortCodePatternMatcher getPatternMatcherFromResource(String country) {
        int id = com.android.internal.R.xml.sms_short_codes;
        XmlResourceParser parser = null;
        try {
            parser = mContext.getResources().getXml(id);
            return getPatternMatcherFromXmlParser(parser, country);
        } finally {
            if (parser != null) parser.close();
        }
    }

    private ShortCodePatternMatcher getPatternMatcherFromXmlParser(XmlPullParser parser,
            String country) {
        try {
            XmlUtils.beginDocument(parser, TAG_SHORTCODES);

            while (true) {
                XmlUtils.nextElement(parser);
                String element = parser.getName();
                if (element == null) {
                    Rlog.e(TAG, "Parsing pattern data found null");
                    break;
                }

                if (element.equals(TAG_SHORTCODE)) {
                    String currentCountry = parser.getAttributeValue(null, ATTR_COUNTRY);
                    if (VDBG) Rlog.d(TAG, "Found country " + currentCountry);
                    if (country.equals(currentCountry)) {
                        String pattern = parser.getAttributeValue(null, ATTR_PATTERN);
                        String premium = parser.getAttributeValue(null, ATTR_PREMIUM);
                        String free = parser.getAttributeValue(null, ATTR_FREE);
                        String standard = parser.getAttributeValue(null, ATTR_STANDARD);
                        return new ShortCodePatternMatcher(pattern, premium, free, standard);
                    }
                } else {
                    Rlog.e(TAG, "Error: skipping unknown XML tag " + element);
                }
            }
        } catch (XmlPullParserException e) {
            Rlog.e(TAG, "XML parser exception reading short code patterns", e);
        } catch (IOException e) {
            Rlog.e(TAG, "I/O exception reading short code patterns", e);
        }
        if (DBG) Rlog.d(TAG, "Country (" + country + ") not found");
        return null;    // country not found
    }

    /** Clear the SMS application list for disposal. */
    void dispose() {
        mSmsStamp.clear();
    }

    /**
     * Check to see if an application is allowed to send new SMS messages, and confirm with
     * user if the send limit was reached or if a non-system app is potentially sending to a
     * premium SMS short code or number.
     *
     * @param appName the package name of the app requesting to send an SMS
     * @param smsWaiting the number of new messages desired to send
     * @return true if application is allowed to send the requested number
     *  of new sms messages
     */
    public boolean check(String appName, int smsWaiting) {
        synchronized (mSmsStamp) {
            removeExpiredTimestamps();

            ArrayList<Long> sentList = mSmsStamp.get(appName);
            if (sentList == null) {
                sentList = new ArrayList<Long>();
                mSmsStamp.put(appName, sentList);
            }

            return isUnderLimit(sentList, smsWaiting);
        }
    }

    /**
     * Check if the destination is a possible premium short code.
     * NOTE: the caller is expected to strip non-digits from the destination number with
     * {@link PhoneNumberUtils#extractNetworkPortion} before calling this method.
     * This happens in {@link SMSDispatcher#sendRawPdu} so that we use the same phone number
     * for testing and in the user confirmation dialog if the user needs to confirm the number.
     * This makes it difficult for malware to fool the user or the short code pattern matcher
     * by using non-ASCII characters to make the number appear to be different from the real
     * destination phone number.
     *
     * @param destAddress the destination address to test for possible short code
     * @return {@link #CATEGORY_NOT_SHORT_CODE}, {@link #CATEGORY_FREE_SHORT_CODE},
     *  {@link #CATEGORY_POSSIBLE_PREMIUM_SHORT_CODE}, or {@link #CATEGORY_PREMIUM_SHORT_CODE}.
     */
    public int checkDestination(String destAddress, String countryIso) {
        synchronized (mSettingsObserverHandler) {
            // always allow emergency numbers
            if (PhoneNumberUtils.isEmergencyNumber(destAddress, countryIso)) {
                if (DBG) Rlog.d(TAG, "isEmergencyNumber");
                return CATEGORY_NOT_SHORT_CODE;
            }
            // always allow if the feature is disabled
            if (!mCheckEnabled.get()) {
                if (DBG) Rlog.e(TAG, "check disabled");
                return CATEGORY_NOT_SHORT_CODE;
            }

            if (countryIso != null) {
                if (mCurrentCountry == null || !countryIso.equals(mCurrentCountry) ||
                        mPatternFile.lastModified() != mPatternFileLastModified) {
                    if (mPatternFile.exists()) {
                        if (DBG) Rlog.d(TAG, "Loading SMS Short Code patterns from file");
                        mCurrentPatternMatcher = getPatternMatcherFromFile(countryIso);
                    } else {
                        if (DBG) Rlog.d(TAG, "Loading SMS Short Code patterns from resource");
                        mCurrentPatternMatcher = getPatternMatcherFromResource(countryIso);
                    }
                    mCurrentCountry = countryIso;
                }
            }

            if (mCurrentPatternMatcher != null) {
                return mCurrentPatternMatcher.getNumberCategory(destAddress);
            } else {
                // Generic rule: numbers of 5 digits or less are considered potential short codes
                Rlog.e(TAG, "No patterns for \"" + countryIso + "\": using generic short code rule");
                if (destAddress.length() <= 5) {
                    return CATEGORY_POSSIBLE_PREMIUM_SHORT_CODE;
                } else {
                    return CATEGORY_NOT_SHORT_CODE;
                }
            }
        }
    }

    /**
     * Load the premium SMS policy from an XML file.
     * Based on code from NotificationManagerService.
     */
    private void loadPremiumSmsPolicyDb() {
        synchronized (mPremiumSmsPolicy) {
            if (mPolicyFile == null) {
                File dir = new File(SMS_POLICY_FILE_DIRECTORY);
                mPolicyFile = new AtomicFile(new File(dir, SMS_POLICY_FILE_NAME));

                mPremiumSmsPolicy.clear();

                FileInputStream infile = null;
                try {
                    infile = mPolicyFile.openRead();
                    final XmlPullParser parser = Xml.newPullParser();
                    parser.setInput(infile, StandardCharsets.UTF_8.name());

                    XmlUtils.beginDocument(parser, TAG_SMS_POLICY_BODY);

                    while (true) {
                        XmlUtils.nextElement(parser);

                        String element = parser.getName();
                        if (element == null) break;

                        if (element.equals(TAG_PACKAGE)) {
                            String packageName = parser.getAttributeValue(null, ATTR_PACKAGE_NAME);
                            String policy = parser.getAttributeValue(null, ATTR_PACKAGE_SMS_POLICY);
                            if (packageName == null) {
                                Rlog.e(TAG, "Error: missing package name attribute");
                            } else if (policy == null) {
                                Rlog.e(TAG, "Error: missing package policy attribute");
                            } else try {
                                mPremiumSmsPolicy.put(packageName, Integer.parseInt(policy));
                            } catch (NumberFormatException e) {
                                Rlog.e(TAG, "Error: non-numeric policy type " + policy);
                            }
                        } else {
                            Rlog.e(TAG, "Error: skipping unknown XML tag " + element);
                        }
                    }
                } catch (FileNotFoundException e) {
                    // No data yet
                } catch (IOException e) {
                    Rlog.e(TAG, "Unable to read premium SMS policy database", e);
                } catch (NumberFormatException e) {
                    Rlog.e(TAG, "Unable to parse premium SMS policy database", e);
                } catch (XmlPullParserException e) {
                    Rlog.e(TAG, "Unable to parse premium SMS policy database", e);
                } finally {
                    if (infile != null) {
                        try {
                            infile.close();
                        } catch (IOException ignored) {
                        }
                    }
                }
            }
        }
    }

    /**
     * Persist the premium SMS policy to an XML file.
     * Based on code from NotificationManagerService.
     */
    private void writePremiumSmsPolicyDb() {
        synchronized (mPremiumSmsPolicy) {
            FileOutputStream outfile = null;
            try {
                outfile = mPolicyFile.startWrite();

                XmlSerializer out = new FastXmlSerializer();
                out.setOutput(outfile, StandardCharsets.UTF_8.name());

                out.startDocument(null, true);

                out.startTag(null, TAG_SMS_POLICY_BODY);

                for (Map.Entry<String, Integer> policy : mPremiumSmsPolicy.entrySet()) {
                    out.startTag(null, TAG_PACKAGE);
                    out.attribute(null, ATTR_PACKAGE_NAME, policy.getKey());
                    out.attribute(null, ATTR_PACKAGE_SMS_POLICY, policy.getValue().toString());
                    out.endTag(null, TAG_PACKAGE);
                }

                out.endTag(null, TAG_SMS_POLICY_BODY);
                out.endDocument();

                mPolicyFile.finishWrite(outfile);
            } catch (IOException e) {
                Rlog.e(TAG, "Unable to write premium SMS policy database", e);
                if (outfile != null) {
                    mPolicyFile.failWrite(outfile);
                }
            }
        }
    }

    /**
     * Returns the premium SMS permission for the specified package. If the package has never
     * been seen before, the default {@link #PREMIUM_SMS_PERMISSION_ASK_USER}
     * will be returned.
     * @param packageName the name of the package to query permission
     * @return one of {@link #PREMIUM_SMS_PERMISSION_UNKNOWN},
     *  {@link #PREMIUM_SMS_PERMISSION_ASK_USER},
     *  {@link #PREMIUM_SMS_PERMISSION_NEVER_ALLOW}, or
     *  {@link #PREMIUM_SMS_PERMISSION_ALWAYS_ALLOW}
     * @throws SecurityException if the caller is not a system process
     */
    public int getPremiumSmsPermission(String packageName) {
        checkCallerIsSystemOrPhoneOrSameApp(packageName);
        synchronized (mPremiumSmsPolicy) {
            Integer policy = mPremiumSmsPolicy.get(packageName);
            if (policy == null) {
                return PREMIUM_SMS_PERMISSION_UNKNOWN;
            } else {
                return policy;
            }
        }
    }

    /**
     * Sets the premium SMS permission for the specified package and save the value asynchronously
     * to persistent storage.
     * @param packageName the name of the package to set permission
     * @param permission one of {@link #PREMIUM_SMS_PERMISSION_ASK_USER},
     *  {@link #PREMIUM_SMS_PERMISSION_NEVER_ALLOW}, or
     *  {@link #PREMIUM_SMS_PERMISSION_ALWAYS_ALLOW}
     * @throws SecurityException if the caller is not a system process
     */
    public void setPremiumSmsPermission(String packageName, int permission) {
        checkCallerIsSystemOrPhoneApp();
        if (permission < PREMIUM_SMS_PERMISSION_ASK_USER
                || permission > PREMIUM_SMS_PERMISSION_ALWAYS_ALLOW) {
            throw new IllegalArgumentException("invalid SMS permission type " + permission);
        }
        synchronized (mPremiumSmsPolicy) {
            mPremiumSmsPolicy.put(packageName, permission);
        }
        // write policy file in the background
        new Thread(new Runnable() {
            @Override
            public void run() {
                writePremiumSmsPolicyDb();
            }
        }).start();
    }

    private static void checkCallerIsSystemOrPhoneOrSameApp(String pkg) {
        int uid = Binder.getCallingUid();
        int appId = UserHandle.getAppId(uid);
        if (appId == Process.SYSTEM_UID || appId == Process.PHONE_UID || uid == 0) {
            return;
        }
        try {
            ApplicationInfo ai = AppGlobals.getPackageManager().getApplicationInfo(
                    pkg, 0, UserHandle.getCallingUserId());
            if (!UserHandle.isSameApp(ai.uid, uid)) {
                throw new SecurityException("Calling uid " + uid + " gave package"
                        + pkg + " which is owned by uid " + ai.uid);
            }
        } catch (RemoteException re) {
            throw new SecurityException("Unknown package " + pkg + "\n" + re);
        }
    }

    private static void checkCallerIsSystemOrPhoneApp() {
        int uid = Binder.getCallingUid();
        int appId = UserHandle.getAppId(uid);
        if (appId == Process.SYSTEM_UID || appId == Process.PHONE_UID || uid == 0) {
            return;
        }
        throw new SecurityException("Disallowed call for uid " + uid);
    }

    /**
     * Remove keys containing only old timestamps. This can happen if an SMS app is used
     * to send messages and then uninstalled.
     */
    private void removeExpiredTimestamps() {
        long beginCheckPeriod = System.currentTimeMillis() - mCheckPeriod;

        synchronized (mSmsStamp) {
            Iterator<Map.Entry<String, ArrayList<Long>>> iter = mSmsStamp.entrySet().iterator();
            while (iter.hasNext()) {
                Map.Entry<String, ArrayList<Long>> entry = iter.next();
                ArrayList<Long> oldList = entry.getValue();
                if (oldList.isEmpty() || oldList.get(oldList.size() - 1) < beginCheckPeriod) {
                    iter.remove();
                }
            }
        }
    }

    private boolean isUnderLimit(ArrayList<Long> sent, int smsWaiting) {
        Long ct = System.currentTimeMillis();
        long beginCheckPeriod = ct - mCheckPeriod;

        if (VDBG) log("SMS send size=" + sent.size() + " time=" + ct);

        while (!sent.isEmpty() && sent.get(0) < beginCheckPeriod) {
            sent.remove(0);
        }

        if ((sent.size() + smsWaiting) <= mMaxAllowed) {
            for (int i = 0; i < smsWaiting; i++ ) {
                sent.add(ct);
            }
            return true;
        }
        return false;
    }

    private static void log(String msg) {
        Rlog.d(TAG, msg);
    }
}
