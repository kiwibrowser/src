/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.vcard;

import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;

import com.android.vcard.exception.VCardAgentNotSupportedException;
import com.android.vcard.exception.VCardException;
import com.android.vcard.exception.VCardInvalidCommentLineException;
import com.android.vcard.exception.VCardInvalidLineException;
import com.android.vcard.exception.VCardVersionException;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * <p>
 * Basic implementation achieving vCard parsing. Based on vCard 2.1.
 * </p>
 * @hide
 */
/* package */ class VCardParserImpl_V21 {
    private static final String LOG_TAG = VCardConstants.LOG_TAG;

    protected static final class CustomBufferedReader extends BufferedReader {
        private long mTime;

        /**
         * Needed since "next line" may be null due to end of line.
         */
        private boolean mNextLineIsValid;
        private String mNextLine;

        public CustomBufferedReader(Reader in) {
            super(in);
        }

        @Override
        public String readLine() throws IOException {
            if (mNextLineIsValid) {
                final String ret = mNextLine;
                mNextLine = null;
                mNextLineIsValid = false;
                return ret;
            }

            final long start = System.currentTimeMillis();
            final String line = super.readLine();
            final long end = System.currentTimeMillis();
            mTime += end - start;
            return line;
        }

        /**
         * Read one line, but make this object store it in its queue.
         */
        public String peekLine() throws IOException {
            if (!mNextLineIsValid) {
                final long start = System.currentTimeMillis();
                final String line = super.readLine();
                final long end = System.currentTimeMillis();
                mTime += end - start;

                mNextLine = line;
                mNextLineIsValid = true;
            }

            return mNextLine;
        }

        public long getTotalmillisecond() {
            return mTime;
        }
    }

    private static final String DEFAULT_ENCODING = "8BIT";
    private static final String DEFAULT_CHARSET = "UTF-8";

    protected final String mIntermediateCharset;

    private final List<VCardInterpreter> mInterpreterList = new ArrayList<VCardInterpreter>();
    private boolean mCanceled;

    /**
     * <p>
     * The encoding type for deconding byte streams. This member variable is
     * reset to a default encoding every time when a new item comes.
     * </p>
     * <p>
     * "Encoding" in vCard is different from "Charset". It is mainly used for
     * addresses, notes, images. "7BIT", "8BIT", "BASE64", and
     * "QUOTED-PRINTABLE" are known examples.
     * </p>
     */
    protected String mCurrentEncoding;

    protected String mCurrentCharset;

    /**
     * <p>
     * The reader object to be used internally.
     * </p>
     * <p>
     * Developers should not directly read a line from this object. Use
     * getLine() unless there some reason.
     * </p>
     */
    protected CustomBufferedReader mReader;

    /**
     * <p>
     * Set for storing unkonwn TYPE attributes, which is not acceptable in vCard
     * specification, but happens to be seen in real world vCard.
     * </p>
     * <p>
     * We just accept those invalid types after emitting a warning for each of it.
     * </p>
     */
    protected final Set<String> mUnknownTypeSet = new HashSet<String>();

    /**
     * <p>
     * Set for storing unkonwn VALUE attributes, which is not acceptable in
     * vCard specification, but happens to be seen in real world vCard.
     * </p>
     * <p>
     * We just accept those invalid types after emitting a warning for each of it.
     * </p>
     */
    protected final Set<String> mUnknownValueSet = new HashSet<String>();


    public VCardParserImpl_V21() {
        this(VCardConfig.VCARD_TYPE_DEFAULT);
    }

    public VCardParserImpl_V21(int vcardType) {
        mIntermediateCharset =  VCardConfig.DEFAULT_INTERMEDIATE_CHARSET;
    }

    /**
     * @return true when a given property name is a valid property name.
     */
    protected boolean isValidPropertyName(final String propertyName) {
        if (!(getKnownPropertyNameSet().contains(propertyName.toUpperCase()) ||
                propertyName.startsWith("X-"))
                && !mUnknownTypeSet.contains(propertyName)) {
            mUnknownTypeSet.add(propertyName);
            Log.w(LOG_TAG, "Property name unsupported by vCard 2.1: " + propertyName);
        }
        return true;
    }

    /**
     * @return String. It may be null, or its length may be 0
     * @throws IOException
     */
    protected String getLine() throws IOException {
        return mReader.readLine();
    }

    protected String peekLine() throws IOException {
        return mReader.peekLine();
    }

    /**
     * @return String with it's length > 0
     * @throws IOException
     * @throws VCardException when the stream reached end of line
     */
    protected String getNonEmptyLine() throws IOException, VCardException {
        String line;
        while (true) {
            line = getLine();
            if (line == null) {
                throw new VCardException("Reached end of buffer.");
            } else if (line.trim().length() > 0) {
                return line;
            }
        }
    }

    /**
     * <code>
     * vcard = "BEGIN" [ws] ":" [ws] "VCARD" [ws] 1*CRLF
     *         items *CRLF
     *         "END" [ws] ":" [ws] "VCARD"
     * </code>
     * @return False when reaching end of file.
     */
    private boolean parseOneVCard() throws IOException, VCardException {
        // reset for this entire vCard.
        mCurrentEncoding = DEFAULT_ENCODING;
        mCurrentCharset = DEFAULT_CHARSET;

        boolean allowGarbage = false;
        if (!readBeginVCard(allowGarbage)) {
            return false;
        }
        for (VCardInterpreter interpreter : mInterpreterList) {
            interpreter.onEntryStarted();
        }
        parseItems();
        for (VCardInterpreter interpreter : mInterpreterList) {
            interpreter.onEntryEnded();
        }
        return true;
    }

    /**
     * @return True when successful. False when reaching the end of line
     * @throws IOException
     * @throws VCardException
     */
    protected boolean readBeginVCard(boolean allowGarbage) throws IOException, VCardException {
        // TODO: use consructPropertyLine().
        String line;
        do {
            while (true) {
                line = getLine();
                if (line == null) {
                    return false;
                } else if (line.trim().length() > 0) {
                    break;
                }
            }
            final String[] strArray = line.split(":", 2);
            final int length = strArray.length;

            // Although vCard 2.1/3.0 specification does not allow lower cases,
            // we found vCard file emitted by some external vCard expoter have such
            // invalid Strings.
            // e.g. BEGIN:vCard
            if (length == 2 && strArray[0].trim().equalsIgnoreCase("BEGIN")
                    && strArray[1].trim().equalsIgnoreCase("VCARD")) {
                return true;
            } else if (!allowGarbage) {
                throw new VCardException("Expected String \"BEGIN:VCARD\" did not come "
                        + "(Instead, \"" + line + "\" came)");
            }
        } while (allowGarbage);

        throw new VCardException("Reached where must not be reached.");
    }

    /**
     * Parses lines other than the first "BEGIN:VCARD". Takes care of "END:VCARD"n and
     * "BEGIN:VCARD" in nested vCard.
     */
    /*
     * items = *CRLF item / item
     *
     * Note: BEGIN/END aren't include in the original spec while this method handles them.
     */
    protected void parseItems() throws IOException, VCardException {
        boolean ended = false;

        try {
            ended = parseItem();
        } catch (VCardInvalidCommentLineException e) {
            Log.e(LOG_TAG, "Invalid line which looks like some comment was found. Ignored.");
        }

        while (!ended) {
            try {
                ended = parseItem();
            } catch (VCardInvalidCommentLineException e) {
                Log.e(LOG_TAG, "Invalid line which looks like some comment was found. Ignored.");
            }
        }
    }

    /*
     * item = [groups "."] name [params] ":" value CRLF / [groups "."] "ADR"
     * [params] ":" addressparts CRLF / [groups "."] "ORG" [params] ":" orgparts
     * CRLF / [groups "."] "N" [params] ":" nameparts CRLF / [groups "."]
     * "AGENT" [params] ":" vcard CRLF
     */
    protected boolean parseItem() throws IOException, VCardException {
        // Reset for an item.
        mCurrentEncoding = DEFAULT_ENCODING;

        final String line = getNonEmptyLine();
        final VCardProperty propertyData = constructPropertyData(line);

        final String propertyNameUpper = propertyData.getName().toUpperCase();
        final String propertyRawValue = propertyData.getRawValue();

        if (propertyNameUpper.equals(VCardConstants.PROPERTY_BEGIN)) {
            if (propertyRawValue.equalsIgnoreCase("VCARD")) {
                handleNest();
            } else {
                throw new VCardException("Unknown BEGIN type: " + propertyRawValue);
            }
        } else if (propertyNameUpper.equals(VCardConstants.PROPERTY_END)) {
            if (propertyRawValue.equalsIgnoreCase("VCARD")) {
                return true;  // Ended.
            } else {
                throw new VCardException("Unknown END type: " + propertyRawValue);
            }
        } else {
            parseItemInter(propertyData, propertyNameUpper);
        }
        return false;
    }

    private void parseItemInter(VCardProperty property, String propertyNameUpper)
            throws IOException, VCardException {
        String propertyRawValue = property.getRawValue();
        if (propertyNameUpper.equals(VCardConstants.PROPERTY_AGENT)) {
            handleAgent(property);
        } else if (isValidPropertyName(propertyNameUpper)) {
            if (propertyNameUpper.equals(VCardConstants.PROPERTY_VERSION) &&
                    !propertyRawValue.equals(getVersionString())) {
                throw new VCardVersionException(
                        "Incompatible version: " + propertyRawValue + " != " + getVersionString());
            }
            handlePropertyValue(property, propertyNameUpper);
        } else {
            throw new VCardException("Unknown property name: \"" + propertyNameUpper + "\"");
        }
    }

    private void handleNest() throws IOException, VCardException {
        for (VCardInterpreter interpreter : mInterpreterList) {
            interpreter.onEntryStarted();
        }
        parseItems();
        for (VCardInterpreter interpreter : mInterpreterList) {
            interpreter.onEntryEnded();
        }
    }

    // For performance reason, the states for group and property name are merged into one.
    static private final int STATE_GROUP_OR_PROPERTY_NAME = 0;
    static private final int STATE_PARAMS = 1;
    // vCard 3.0 specification allows double-quoted parameters, while vCard 2.1 does not.
    static private final int STATE_PARAMS_IN_DQUOTE = 2;

    protected VCardProperty constructPropertyData(String line) throws VCardException {
        final VCardProperty propertyData = new VCardProperty();

        final int length = line.length();
        if (length > 0 && line.charAt(0) == '#') {
            throw new VCardInvalidCommentLineException();
        }

        int state = STATE_GROUP_OR_PROPERTY_NAME;
        int nameIndex = 0;

        // This loop is developed so that we don't have to take care of bottle neck here.
        // Refactor carefully when you need to do so.
        for (int i = 0; i < length; i++) {
            final char ch = line.charAt(i);
            switch (state) {
                case STATE_GROUP_OR_PROPERTY_NAME: {
                    if (ch == ':') {  // End of a property name.
                        final String propertyName = line.substring(nameIndex, i);
                        propertyData.setName(propertyName);
                        propertyData.setRawValue( i < length - 1 ? line.substring(i + 1) : "");
                        return propertyData;
                    } else if (ch == '.') {  // Each group is followed by the dot.
                        final String groupName = line.substring(nameIndex, i);
                        if (groupName.length() == 0) {
                            Log.w(LOG_TAG, "Empty group found. Ignoring.");
                        } else {
                            propertyData.addGroup(groupName);
                        }
                        nameIndex = i + 1;  // Next should be another group or a property name.
                    } else if (ch == ';') {  // End of property name and beginneng of parameters.
                        final String propertyName = line.substring(nameIndex, i);
                        propertyData.setName(propertyName);
                        nameIndex = i + 1;
                        state = STATE_PARAMS;  // Start parameter parsing.
                    }
                    // TODO: comma support (in vCard 3.0 and 4.0).
                    break;
                }
                case STATE_PARAMS: {
                    if (ch == '"') {
                        if (VCardConstants.VERSION_V21.equalsIgnoreCase(getVersionString())) {
                            Log.w(LOG_TAG, "Double-quoted params found in vCard 2.1. " +
                                    "Silently allow it");
                        }
                        state = STATE_PARAMS_IN_DQUOTE;
                    } else if (ch == ';') {  // Starts another param.
                        handleParams(propertyData, line.substring(nameIndex, i));
                        nameIndex = i + 1;
                    } else if (ch == ':') {  // End of param and beginenning of values.
                        handleParams(propertyData, line.substring(nameIndex, i));
                        propertyData.setRawValue(i < length - 1 ? line.substring(i + 1) : "");
                        return propertyData;
                    }
                    break;
                }
                case STATE_PARAMS_IN_DQUOTE: {
                    if (ch == '"') {
                        if (VCardConstants.VERSION_V21.equalsIgnoreCase(getVersionString())) {
                            Log.w(LOG_TAG, "Double-quoted params found in vCard 2.1. " +
                                    "Silently allow it");
                        }
                        state = STATE_PARAMS;
                    }
                    break;
                }
            }
        }

        throw new VCardInvalidLineException("Invalid line: \"" + line + "\"");
    }

    /*
     * params = ";" [ws] paramlist paramlist = paramlist [ws] ";" [ws] param /
     * param param = "TYPE" [ws] "=" [ws] ptypeval / "VALUE" [ws] "=" [ws]
     * pvalueval / "ENCODING" [ws] "=" [ws] pencodingval / "CHARSET" [ws] "="
     * [ws] charsetval / "LANGUAGE" [ws] "=" [ws] langval / "X-" word [ws] "="
     * [ws] word / knowntype
     */
    protected void handleParams(VCardProperty propertyData, String params)
            throws VCardException {
        final String[] strArray = params.split("=", 2);
        if (strArray.length == 2) {
            final String paramName = strArray[0].trim().toUpperCase();
            String paramValue = strArray[1].trim();
            if (paramName.equals("TYPE")) {
                handleType(propertyData, paramValue);
            } else if (paramName.equals("VALUE")) {
                handleValue(propertyData, paramValue);
            } else if (paramName.equals("ENCODING")) {
                handleEncoding(propertyData, paramValue.toUpperCase());
            } else if (paramName.equals("CHARSET")) {
                handleCharset(propertyData, paramValue);
            } else if (paramName.equals("LANGUAGE")) {
                handleLanguage(propertyData, paramValue);
            } else if (paramName.startsWith("X-")) {
                handleAnyParam(propertyData, paramName, paramValue);
            } else {
                throw new VCardException("Unknown type \"" + paramName + "\"");
            }
        } else {
            handleParamWithoutName(propertyData, strArray[0]);
        }
    }

    /**
     * vCard 3.0 parser implementation may throw VCardException.
     */
    protected void handleParamWithoutName(VCardProperty propertyData, final String paramValue) {
        handleType(propertyData, paramValue);
    }

    /*
     * ptypeval = knowntype / "X-" word
     */
    protected void handleType(VCardProperty propertyData, final String ptypeval) {
        if (!(getKnownTypeSet().contains(ptypeval.toUpperCase())
                || ptypeval.startsWith("X-"))
                && !mUnknownTypeSet.contains(ptypeval)) {
            mUnknownTypeSet.add(ptypeval);
            Log.w(LOG_TAG, String.format("TYPE unsupported by %s: ", getVersion(), ptypeval));
        }
        propertyData.addParameter(VCardConstants.PARAM_TYPE, ptypeval);
    }

    /*
     * pvalueval = "INLINE" / "URL" / "CONTENT-ID" / "CID" / "X-" word
     */
    protected void handleValue(VCardProperty propertyData, final String pvalueval) {
        if (!(getKnownValueSet().contains(pvalueval.toUpperCase())
                || pvalueval.startsWith("X-")
                || mUnknownValueSet.contains(pvalueval))) {
            mUnknownValueSet.add(pvalueval);
            Log.w(LOG_TAG, String.format(
                    "The value unsupported by TYPE of %s: ", getVersion(), pvalueval));
        }
        propertyData.addParameter(VCardConstants.PARAM_VALUE, pvalueval);
    }

    /*
     * pencodingval = "7BIT" / "8BIT" / "QUOTED-PRINTABLE" / "BASE64" / "X-" word
     */
    protected void handleEncoding(VCardProperty propertyData, String pencodingval)
            throws VCardException {
        if (getAvailableEncodingSet().contains(pencodingval) ||
                pencodingval.startsWith("X-")) {
            propertyData.addParameter(VCardConstants.PARAM_ENCODING, pencodingval);
            // Update encoding right away, as this is needed to understanding other params.
            mCurrentEncoding = pencodingval.toUpperCase();
        } else {
            throw new VCardException("Unknown encoding \"" + pencodingval + "\"");
        }
    }

    /**
     * <p>
     * vCard 2.1 specification only allows us-ascii and iso-8859-xxx (See RFC 1521),
     * but recent vCard files often contain other charset like UTF-8, SHIFT_JIS, etc.
     * We allow any charset.
     * </p>
     */
    protected void handleCharset(VCardProperty propertyData, String charsetval) {
        mCurrentCharset = charsetval;
        propertyData.addParameter(VCardConstants.PARAM_CHARSET, charsetval);
    }

    /**
     * See also Section 7.1 of RFC 1521
     */
    protected void handleLanguage(VCardProperty propertyData, String langval)
            throws VCardException {
        String[] strArray = langval.split("-");
        if (strArray.length != 2) {
            throw new VCardException("Invalid Language: \"" + langval + "\"");
        }
        String tmp = strArray[0];
        int length = tmp.length();
        for (int i = 0; i < length; i++) {
            if (!isAsciiLetter(tmp.charAt(i))) {
                throw new VCardException("Invalid Language: \"" + langval + "\"");
            }
        }
        tmp = strArray[1];
        length = tmp.length();
        for (int i = 0; i < length; i++) {
            if (!isAsciiLetter(tmp.charAt(i))) {
                throw new VCardException("Invalid Language: \"" + langval + "\"");
            }
        }
        propertyData.addParameter(VCardConstants.PARAM_LANGUAGE, langval);
    }

    private boolean isAsciiLetter(char ch) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            return true;
        }
        return false;
    }

    /**
     * Mainly for "X-" type. This accepts any kind of type without check.
     */
    protected void handleAnyParam(
            VCardProperty propertyData, String paramName, String paramValue) {
        propertyData.addParameter(paramName, paramValue);
    }

    protected void handlePropertyValue(VCardProperty property, String propertyName)
            throws IOException, VCardException {
        final String propertyNameUpper = property.getName().toUpperCase();
        String propertyRawValue = property.getRawValue();
        final String sourceCharset = VCardConfig.DEFAULT_INTERMEDIATE_CHARSET;
        final Collection<String> charsetCollection =
                property.getParameters(VCardConstants.PARAM_CHARSET);
        String targetCharset =
                ((charsetCollection != null) ? charsetCollection.iterator().next() : null);
        if (TextUtils.isEmpty(targetCharset)) {
            targetCharset = VCardConfig.DEFAULT_IMPORT_CHARSET;
        }

        // TODO: have "separableProperty" which reflects vCard spec..
        if (propertyNameUpper.equals(VCardConstants.PROPERTY_ADR)
                || propertyNameUpper.equals(VCardConstants.PROPERTY_ORG)
                || propertyNameUpper.equals(VCardConstants.PROPERTY_N)) {
            handleAdrOrgN(property, propertyRawValue, sourceCharset, targetCharset);
            return;
        }

        if (mCurrentEncoding.equals(VCardConstants.PARAM_ENCODING_QP) ||
                // If encoding attribute is missing, then attempt to detect QP encoding.
                // This is to handle a bug where the android exporter was creating FN properties
                // with missing encoding.  b/7292017
                (propertyNameUpper.equals(VCardConstants.PROPERTY_FN) &&
                        property.getParameters(VCardConstants.PARAM_ENCODING) == null &&
                        VCardUtils.appearsLikeAndroidVCardQuotedPrintable(propertyRawValue))
                ) {
            final String quotedPrintablePart = getQuotedPrintablePart(propertyRawValue);
            final String propertyEncodedValue =
                    VCardUtils.parseQuotedPrintable(quotedPrintablePart,
                            false, sourceCharset, targetCharset);
            property.setRawValue(quotedPrintablePart);
            property.setValues(propertyEncodedValue);
            for (VCardInterpreter interpreter : mInterpreterList) {
                interpreter.onPropertyCreated(property);
            }
        } else if (mCurrentEncoding.equals(VCardConstants.PARAM_ENCODING_BASE64)
                || mCurrentEncoding.equals(VCardConstants.PARAM_ENCODING_B)) {
            // It is very rare, but some BASE64 data may be so big that
            // OutOfMemoryError occurs. To ignore such cases, use try-catch.
            try {
                final String base64Property = getBase64(propertyRawValue);
                try {
                    property.setByteValue(Base64.decode(base64Property, Base64.DEFAULT));
                } catch (IllegalArgumentException e) {
                    throw new VCardException("Decode error on base64 photo: " + propertyRawValue);
                }
                for (VCardInterpreter interpreter : mInterpreterList) {
                    interpreter.onPropertyCreated(property);
                }
            } catch (OutOfMemoryError error) {
                Log.e(LOG_TAG, "OutOfMemoryError happened during parsing BASE64 data!");
                for (VCardInterpreter interpreter : mInterpreterList) {
                    interpreter.onPropertyCreated(property);
                }
            }
        } else {
            if (!(mCurrentEncoding.equals("7BIT") || mCurrentEncoding.equals("8BIT") ||
                    mCurrentEncoding.startsWith("X-"))) {
                Log.w(LOG_TAG,
                        String.format("The encoding \"%s\" is unsupported by vCard %s",
                                mCurrentEncoding, getVersionString()));
            }

            // Some device uses line folding defined in RFC 2425, which is not allowed
            // in vCard 2.1 (while needed in vCard 3.0).
            //
            // e.g.
            // BEGIN:VCARD
            // VERSION:2.1
            // N:;Omega;;;
            // EMAIL;INTERNET:"Omega"
            //   <omega@example.com>
            // FN:Omega
            // END:VCARD
            //
            // The vCard above assumes that email address should become:
            // "Omega" <omega@example.com>
            //
            // But vCard 2.1 requires Quote-Printable when a line contains line break(s).
            //
            // For more information about line folding,
            // see "5.8.1. Line delimiting and folding" in RFC 2425.
            //
            // We take care of this case more formally in vCard 3.0, so we only need to
            // do this in vCard 2.1.
            if (getVersion() == VCardConfig.VERSION_21) {
                StringBuilder builder = null;
                while (true) {
                    final String nextLine = peekLine();
                    // We don't need to care too much about this exceptional case,
                    // but we should not wrongly eat up "END:VCARD", since it critically
                    // breaks this parser's state machine.
                    // Thus we roughly look over the next line and confirm it is at least not
                    // "END:VCARD". This extra fee is worth paying. This is exceptional
                    // anyway.
                    if (!TextUtils.isEmpty(nextLine) &&
                            nextLine.charAt(0) == ' ' &&
                            !"END:VCARD".contains(nextLine.toUpperCase())) {
                        getLine();  // Drop the next line.

                        if (builder == null) {
                            builder = new StringBuilder();
                            builder.append(propertyRawValue);
                        }
                        builder.append(nextLine.substring(1));
                    } else {
                        break;
                    }
                }
                if (builder != null) {
                    propertyRawValue = builder.toString();
                }
            }

            ArrayList<String> propertyValueList = new ArrayList<String>();
            String value = maybeUnescapeText(VCardUtils.convertStringCharset(
                    propertyRawValue, sourceCharset, targetCharset));
            propertyValueList.add(value);
            property.setValues(propertyValueList);
            for (VCardInterpreter interpreter : mInterpreterList) {
                interpreter.onPropertyCreated(property);
            }
        }
    }

    private void handleAdrOrgN(VCardProperty property, String propertyRawValue,
            String sourceCharset, String targetCharset) throws VCardException, IOException {
        List<String> encodedValueList = new ArrayList<String>();

        // vCard 2.1 does not allow QUOTED-PRINTABLE here, but some softwares/devices emit
        // such data.
        if (mCurrentEncoding.equals(VCardConstants.PARAM_ENCODING_QP)) {
            // First we retrieve Quoted-Printable String from vCard entry, which may include
            // multiple lines.
            final String quotedPrintablePart = getQuotedPrintablePart(propertyRawValue);

            // "Raw value" from the view of users should contain all part of QP string.
            // TODO: add test for this handling
            property.setRawValue(quotedPrintablePart);

            // We split Quoted-Printable String using semi-colon before decoding it, as
            // the Quoted-Printable may have semi-colon, which confuses splitter.
            final List<String> quotedPrintableValueList =
                    VCardUtils.constructListFromValue(quotedPrintablePart, getVersion());
            for (String quotedPrintableValue : quotedPrintableValueList) {
                String encoded = VCardUtils.parseQuotedPrintable(quotedPrintableValue,
                        false, sourceCharset, targetCharset);
                encodedValueList.add(encoded);
            }
        } else {
            final String propertyValue = VCardUtils.convertStringCharset(
                    getPotentialMultiline(propertyRawValue), sourceCharset, targetCharset);
            final List<String> valueList =
                    VCardUtils.constructListFromValue(propertyValue, getVersion());
            for (String value : valueList) {
                encodedValueList.add(value);
            }
        }

        property.setValues(encodedValueList);
        for (VCardInterpreter interpreter : mInterpreterList) {
            interpreter.onPropertyCreated(property);
        }
    }

    /**
     * <p>
     * Parses and returns Quoted-Printable.
     * </p>
     *
     * @param firstString The string following a parameter name and attributes.
     *            Example: "string" in
     *            "ADR:ENCODING=QUOTED-PRINTABLE:string\n\r".
     * @return whole Quoted-Printable string, including a given argument and
     *         following lines. Excludes the last empty line following to Quoted
     *         Printable lines.
     * @throws IOException
     * @throws VCardException
     */
    private String getQuotedPrintablePart(String firstString)
            throws IOException, VCardException {
        // Specifically, there may be some padding between = and CRLF.
        // See the following:
        //
        // qp-line := *(qp-segment transport-padding CRLF)
        // qp-part transport-padding
        // qp-segment := qp-section *(SPACE / TAB) "="
        // ; Maximum length of 76 characters
        //
        // e.g. (from RFC 2045)
        // Now's the time =
        // for all folk to come=
        // to the aid of their country.
        if (firstString.trim().endsWith("=")) {
            // remove "transport-padding"
            int pos = firstString.length() - 1;
            while (firstString.charAt(pos) != '=') {
            }
            StringBuilder builder = new StringBuilder();
            builder.append(firstString.substring(0, pos + 1));
            builder.append("\r\n");
            String line;
            while (true) {
                line = getLine();
                if (line == null) {
                    throw new VCardException("File ended during parsing a Quoted-Printable String");
                }
                if (line.trim().endsWith("=")) {
                    // remove "transport-padding"
                    pos = line.length() - 1;
                    while (line.charAt(pos) != '=') {
                    }
                    builder.append(line.substring(0, pos + 1));
                    builder.append("\r\n");
                } else {
                    builder.append(line);
                    break;
                }
            }
            return builder.toString();
        } else {
            return firstString;
        }
    }

    /**
     * Given the first line of a property, checks consecutive lines after it and builds a new
     * multi-line value if it exists.
     *
     * @param firstString The first line of the property.
     * @return A new property, potentially built from multiple lines.
     * @throws IOException
     */
    private String getPotentialMultiline(String firstString) throws IOException {
        final StringBuilder builder = new StringBuilder();
        builder.append(firstString);

        while (true) {
            final String line = peekLine();
            if (line == null || line.length() == 0) {
                break;
            }

            final String propertyName = getPropertyNameUpperCase(line);
            if (propertyName != null) {
                break;
            }

            // vCard 2.1 does not allow multi-line of adr but microsoft vcards may have it.
            // We will consider the next line to be a part of a multi-line value if it does not
            // contain a property name (i.e. a colon or semi-colon).
            // Consume the line.
            getLine();
            builder.append(" ").append(line);
        }

        return builder.toString();
    }

    protected String getBase64(String firstString) throws IOException, VCardException {
        final StringBuilder builder = new StringBuilder();
        builder.append(firstString);

        while (true) {
            final String line = peekLine();
            if (line == null) {
                throw new VCardException("File ended during parsing BASE64 binary");
            }

            // vCard 2.1 requires two spaces at the end of BASE64 strings, but some vCard doesn't
            // have them. We try to detect those cases using colon and semi-colon, given BASE64
            // does not contain it.
            // E.g.
            //      TEL;TYPE=WORK:+5555555
            // or
            //      END:VCARD
            String propertyName = getPropertyNameUpperCase(line);
            if (getKnownPropertyNameSet().contains(propertyName) ||
                    VCardConstants.PROPERTY_X_ANDROID_CUSTOM.equals(propertyName)) {
                Log.w(LOG_TAG, "Found a next property during parsing a BASE64 string, " +
                        "which must not contain semi-colon or colon. Treat the line as next "
                        + "property.");
                Log.w(LOG_TAG, "Problematic line: " + line.trim());
                break;
            }

            // Consume the line.
            getLine();

            if (line.length() == 0) {
                break;
            }
            // Trim off any extraneous whitespace to handle 2.1 implementations
            // that use 3.0 style line continuations. This is safe because space
            // isn't a Base64 encoding value.
            builder.append(line.trim());
        }

        return builder.toString();
    }

    /**
     * Extracts the property name portion of a given vCard line.
     * <p>
     * Properties must contain a colon.
     * <p>
     * E.g.
     *      TEL;TYPE=WORK:+5555555  // returns "TEL"
     *      END:VCARD // returns "END"
     *      TEL; // returns null
     *
     * @param line The vCard line.
     * @return The property name portion. {@literal null} if no property name found.
     */
    private String getPropertyNameUpperCase(String line) {
        final int colonIndex = line.indexOf(":");
        if (colonIndex > -1) {
            final int semiColonIndex = line.indexOf(";");

            // Find the minimum index that is greater than -1.
            final int minIndex;
            if (colonIndex == -1) {
                minIndex = semiColonIndex;
            } else if (semiColonIndex == -1) {
                minIndex = colonIndex;
            } else {
                minIndex = Math.min(colonIndex, semiColonIndex);
            }
            return line.substring(0, minIndex).toUpperCase();
        }
        return null;
    }

    /*
     * vCard 2.1 specifies AGENT allows one vcard entry. Currently we emit an
     * error toward the AGENT property.
     * // TODO: Support AGENT property.
     * item =
     * ... / [groups "."] "AGENT" [params] ":" vcard CRLF vcard = "BEGIN" [ws]
     * ":" [ws] "VCARD" [ws] 1*CRLF items *CRLF "END" [ws] ":" [ws] "VCARD"
     */
    protected void handleAgent(final VCardProperty property) throws VCardException {
        if (!property.getRawValue().toUpperCase().contains("BEGIN:VCARD")) {
            // Apparently invalid line seen in Windows Mobile 6.5. Ignore them.
            for (VCardInterpreter interpreter : mInterpreterList) {
                interpreter.onPropertyCreated(property);
            }
            return;
        } else {
            throw new VCardAgentNotSupportedException("AGENT Property is not supported now.");
        }
    }

    /**
     * For vCard 3.0.
     */
    protected String maybeUnescapeText(final String text) {
        return text;
    }

    /**
     * Returns unescaped String if the character should be unescaped. Return
     * null otherwise. e.g. In vCard 2.1, "\;" should be unescaped into ";"
     * while "\x" should not be.
     */
    protected String maybeUnescapeCharacter(final char ch) {
        return unescapeCharacter(ch);
    }

    /* package */ static String unescapeCharacter(final char ch) {
        // Original vCard 2.1 specification does not allow transformation
        // "\:" -> ":", "\," -> ",", and "\\" -> "\", but previous
        // implementation of
        // this class allowed them, so keep it as is.
        if (ch == '\\' || ch == ';' || ch == ':' || ch == ',') {
            return String.valueOf(ch);
        } else {
            return null;
        }
    }

    /**
     * @return {@link VCardConfig#VERSION_21}
     */
    protected int getVersion() {
        return VCardConfig.VERSION_21;
    }

    /**
     * @return {@link VCardConfig#VERSION_30}
     */
    protected String getVersionString() {
        return VCardConstants.VERSION_V21;
    }

    protected Set<String> getKnownPropertyNameSet() {
        return VCardParser_V21.sKnownPropertyNameSet;
    }

    protected Set<String> getKnownTypeSet() {
        return VCardParser_V21.sKnownTypeSet;
    }

    protected Set<String> getKnownValueSet() {
        return VCardParser_V21.sKnownValueSet;
    }

    protected Set<String> getAvailableEncodingSet() {
        return VCardParser_V21.sAvailableEncoding;
    }

    protected String getDefaultEncoding() {
        return DEFAULT_ENCODING;
    }

    protected String getDefaultCharset() {
        return DEFAULT_CHARSET;
    }

    protected String getCurrentCharset() {
        return mCurrentCharset;
    }

    public void addInterpreter(VCardInterpreter interpreter) {
        mInterpreterList.add(interpreter);
    }

    public void parse(InputStream is) throws IOException, VCardException {
        if (is == null) {
            throw new NullPointerException("InputStream must not be null.");
        }

        final InputStreamReader tmpReader = new InputStreamReader(is, mIntermediateCharset);
        mReader = new CustomBufferedReader(tmpReader);

        final long start = System.currentTimeMillis();
        for (VCardInterpreter interpreter : mInterpreterList) {
            interpreter.onVCardStarted();
        }

        // vcard_file = [wsls] vcard [wsls]
        while (true) {
            synchronized (this) {
                if (mCanceled) {
                    Log.i(LOG_TAG, "Cancel request has come. exitting parse operation.");
                    break;
                }
            }
            if (!parseOneVCard()) {
                break;
            }
        }

        for (VCardInterpreter interpreter : mInterpreterList) {
            interpreter.onVCardEnded();
        }
    }

    public void parseOne(InputStream is) throws IOException, VCardException {
        if (is == null) {
            throw new NullPointerException("InputStream must not be null.");
        }

        final InputStreamReader tmpReader = new InputStreamReader(is, mIntermediateCharset);
        mReader = new CustomBufferedReader(tmpReader);

        final long start = System.currentTimeMillis();
        for (VCardInterpreter interpreter : mInterpreterList) {
            interpreter.onVCardStarted();
        }
        parseOneVCard();
        for (VCardInterpreter interpreter : mInterpreterList) {
            interpreter.onVCardEnded();
        }
    }

    public final synchronized void cancel() {
        Log.i(LOG_TAG, "ParserImpl received cancel operation.");
        mCanceled = true;
    }
}
