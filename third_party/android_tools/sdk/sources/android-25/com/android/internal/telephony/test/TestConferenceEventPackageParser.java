/*
 * Copyright (C) 2014 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.internal.telephony.test;

import com.android.ims.ImsConferenceState;
import com.android.internal.util.XmlUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.os.Bundle;
import android.util.Log;
import android.util.Xml;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Implements a basic XML parser used to parse test IMS conference event packages which can be
 * injected into the IMS framework via the {@link com.android.internal.telephony.TelephonyTester}.
 * <pre>
 * {@code
 * <xml>
 *     <participant>
 *         <user>tel:+16505551212</user>
 *         <display-text>Joe Q. Public</display-text>
 *         <endpoint>sip:+16505551212@ims-test-provider.com</endpoint>
 *         <status>connected</status>
 *     </participant>
 * </xml>
 * }
 * </pre>
 * <p>
 * Note: This XML format is similar to the information stored in the
 * {@link com.android.ims.ImsConferenceState} parcelable.  The {@code status} values expected in the
 * XML are those found in the {@code ImsConferenceState} class (e.g.
 * {@link com.android.ims.ImsConferenceState#STATUS_CONNECTED}).
 * <p>
 * Place a file formatted similar to above in /data/data/com.android.phone/files/ and invoke the
 * following command while you have an ongoing IMS call:
 * <pre>
 *     adb shell am broadcast
 *          -a com.android.internal.telephony.TestConferenceEventPackage
 *          -e filename test.xml
 * </pre>
 */
public class TestConferenceEventPackageParser {
    private static final String LOG_TAG = "TestConferenceEventPackageParser";
    private static final String PARTICIPANT_TAG = "participant";

    /**
     * The XML input stream to parse.
     */
    private InputStream mInputStream;

    /**
     * Constructs an input of the conference event package parser for the given input stream.
     *
     * @param inputStream The input stream.
     */
    public TestConferenceEventPackageParser(InputStream inputStream) {
        mInputStream = inputStream;
    }

    /**
     * Parses the conference event package XML file and returns an
     * {@link com.android.ims.ImsConferenceState} instance containing the participants described in
     * the XML file.
     *
     * @return The {@link com.android.ims.ImsConferenceState} instance.
     */
    public ImsConferenceState parse() {
        ImsConferenceState conferenceState = new ImsConferenceState();

        XmlPullParser parser;
        try {
            parser = Xml.newPullParser();
            parser.setInput(mInputStream, null);
            parser.nextTag();

            int outerDepth = parser.getDepth();
            while (XmlUtils.nextElementWithin(parser, outerDepth)) {
                if (parser.getName().equals(PARTICIPANT_TAG)) {
                    Log.v(LOG_TAG, "Found participant.");
                    Bundle participant = parseParticipant(parser);
                    conferenceState.mParticipants.put(participant.getString(
                            ImsConferenceState.ENDPOINT), participant);
                }
            }
        } catch (IOException | XmlPullParserException e) {
            Log.e(LOG_TAG, "Failed to read test conference event package from XML file", e);
            return null;
        } finally {
            try {
                mInputStream.close();
            } catch (IOException e) {
                Log.e(LOG_TAG, "Failed to close test conference event package InputStream", e);
                return null;
            }
        }

        return conferenceState;
    }

    /**
     * Parses a participant record from a conference event package XML file.
     *
     * @param parser The XML parser.
     * @return {@link Bundle} containing the participant information.
     */
    private Bundle parseParticipant(XmlPullParser parser)
            throws IOException, XmlPullParserException {
        Bundle bundle = new Bundle();

        String user = "";
        String displayText = "";
        String endpoint = "";
        String status = "";

        int outerDepth = parser.getDepth();
        while (XmlUtils.nextElementWithin(parser, outerDepth)) {
            if (parser.getName().equals(ImsConferenceState.USER)) {
                parser.next();
                user = parser.getText();
            } else if (parser.getName().equals(ImsConferenceState.DISPLAY_TEXT)) {
                parser.next();
                displayText = parser.getText();
            }  else if (parser.getName().equals(ImsConferenceState.ENDPOINT)) {
                parser.next();
                endpoint = parser.getText();
            }  else if (parser.getName().equals(ImsConferenceState.STATUS)) {
                parser.next();
                status = parser.getText();
            }
        }

        Log.v(LOG_TAG, "User: "+user);
        Log.v(LOG_TAG, "DisplayText: "+displayText);
        Log.v(LOG_TAG, "Endpoint: "+endpoint);
        Log.v(LOG_TAG, "Status: "+status);

        bundle.putString(ImsConferenceState.USER, user);
        bundle.putString(ImsConferenceState.DISPLAY_TEXT, displayText);
        bundle.putString(ImsConferenceState.ENDPOINT, endpoint);
        bundle.putString(ImsConferenceState.STATUS, status);

        return bundle;
    }
}
