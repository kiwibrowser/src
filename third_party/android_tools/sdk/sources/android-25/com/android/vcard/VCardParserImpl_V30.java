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

import android.util.Log;

import com.android.vcard.exception.VCardException;

import java.io.IOException;
import java.util.Set;

/**
 * <p>
 * Basic implementation achieving vCard 3.0 parsing.
 * </p>
 * <p>
 * This class inherits vCard 2.1 implementation since technically they are similar,
 * while specifically there's logical no relevance between them.
 * So that developers are not confused with the inheritance,
 * {@link VCardParser_V30} does not inherit {@link VCardParser_V21}, while
 * {@link VCardParserImpl_V30} inherits {@link VCardParserImpl_V21}.
 * </p>
 * @hide
 */
/* package */ class VCardParserImpl_V30 extends VCardParserImpl_V21 {
    private static final String LOG_TAG = VCardConstants.LOG_TAG;

    private String mPreviousLine;
    private boolean mEmittedAgentWarning = false;

    public VCardParserImpl_V30() {
        super();
    }

    public VCardParserImpl_V30(int vcardType) {
        super(vcardType);
    }

    @Override
    protected int getVersion() {
        return VCardConfig.VERSION_30;
    }

    @Override
    protected String getVersionString() {
        return VCardConstants.VERSION_V30;
    }

    @Override
    protected String getLine() throws IOException {
        if (mPreviousLine != null) {
            String ret = mPreviousLine;
            mPreviousLine = null;
            return ret;
        } else {
            return mReader.readLine();
        }
    }

    /**
     * vCard 3.0 requires that the line with space at the beginning of the line
     * must be combined with previous line.
     */
    @Override
    protected String getNonEmptyLine() throws IOException, VCardException {
        String line;
        StringBuilder builder = null;
        while ((line = mReader.readLine()) != null) {
            // Skip empty lines in order to accomodate implementations that
            // send line termination variations such as \r\r\n.
            if (line.length() == 0) {
                continue;
            } else if (line.charAt(0) == ' ' || line.charAt(0) == '\t') {
                // RFC 2425 describes line continuation as \r\n followed by
                // a single ' ' or '\t' whitespace character.
                if (builder == null) {
                    builder = new StringBuilder();
                }
                if (mPreviousLine != null) {
                    builder.append(mPreviousLine);
                    mPreviousLine = null;
                }
                builder.append(line.substring(1));
            } else {
                if (builder != null || mPreviousLine != null) {
                    break;
                }
                mPreviousLine = line;
            }
        }

        String ret = null;
        if (builder != null) {
            ret = builder.toString();
        } else if (mPreviousLine != null) {
            ret = mPreviousLine;
        }
        mPreviousLine = line;
        if (ret == null) {
            throw new VCardException("Reached end of buffer.");
        }
        return ret;
    }

    /*
     * vcard = [group "."] "BEGIN" ":" "VCARD" 1 * CRLF
     *         1 * (contentline)
     *         ;A vCard object MUST include the VERSION, FN and N types.
     *         [group "."] "END" ":" "VCARD" 1 * CRLF
     */
    @Override
    protected boolean readBeginVCard(boolean allowGarbage) throws IOException, VCardException {
        // TODO: vCard 3.0 supports group.
        return super.readBeginVCard(allowGarbage);
    }

    /**
     * vCard 3.0 allows iana-token as paramType, while vCard 2.1 does not.
     */
    @Override
    protected void handleParams(VCardProperty propertyData, final String params)
            throws VCardException {
        try {
            super.handleParams(propertyData, params);
        } catch (VCardException e) {
            // maybe IANA type
            String[] strArray = params.split("=", 2);
            if (strArray.length == 2) {
                handleAnyParam(propertyData, strArray[0], strArray[1]);
            } else {
                // Must not come here in the current implementation.
                throw new VCardException(
                        "Unknown params value: " + params);
            }
        }
    }

    @Override
    protected void handleAnyParam(
            VCardProperty propertyData, final String paramName, final String paramValue) {
        splitAndPutParam(propertyData, paramName, paramValue);
    }

    @Override
    protected void handleParamWithoutName(VCardProperty property, final String paramValue) {
        handleType(property, paramValue);
    }

    /*
     *  vCard 3.0 defines
     *
     *  param         = param-name "=" param-value *("," param-value)
     *  param-name    = iana-token / x-name
     *  param-value   = ptext / quoted-string
     *  quoted-string = DQUOTE QSAFE-CHAR DQUOTE
     *  QSAFE-CHAR    = WSP / %x21 / %x23-7E / NON-ASCII
     *                ; Any character except CTLs, DQUOTE
     *
     *  QSAFE-CHAR must not contain DQUOTE, including escaped one (\").
     */
    @Override
    protected void handleType(VCardProperty property, final String paramValue) {
        splitAndPutParam(property, VCardConstants.PARAM_TYPE, paramValue);
    }

    /**
     * Splits parameter values into pieces in accordance with vCard 3.0 specification and
     * puts pieces into mInterpreter.
     */
    /*
     *  param-value   = ptext / quoted-string
     *  quoted-string = DQUOTE QSAFE-CHAR DQUOTE
     *  QSAFE-CHAR    = WSP / %x21 / %x23-7E / NON-ASCII
     *                ; Any character except CTLs, DQUOTE
     *
     *  QSAFE-CHAR must not contain DQUOTE, including escaped one (\")
     */
    private void splitAndPutParam(VCardProperty property, String paramName, String paramValue) {
        // "comma,separated:inside.dquote",pref
        //   -->
        // - comma,separated:inside.dquote
        // - pref
        //
        // Note: Though there's a code, we don't need to take much care of
        // wrongly-added quotes like the example above, as they induce
        // parse errors at the top level (when splitting a line into parts).
        StringBuilder builder = null;  // Delay initialization.
        boolean insideDquote = false;
        final int length = paramValue.length();
        for (int i = 0; i < length; i++) {
            final char ch = paramValue.charAt(i);
            if (ch == '"') {
                if (insideDquote) {
                    // End of Dquote.
                    property.addParameter(paramName, encodeParamValue(builder.toString()));
                    builder = null;
                    insideDquote = false;
                } else {
                    if (builder != null) {
                        if (builder.length() > 0) {
                            // e.g.
                            // pref"quoted"
                            Log.w(LOG_TAG, "Unexpected Dquote inside property.");
                        } else {
                            // e.g.
                            // pref,"quoted"
                            // "quoted",pref
                            property.addParameter(paramName, encodeParamValue(builder.toString()));
                        }
                    }
                    insideDquote = true;
                }
            } else if (ch == ',' && !insideDquote) {
                if (builder == null) {
                    Log.w(LOG_TAG, "Comma is used before actual string comes. (" +
                            paramValue + ")");
                } else {
                    property.addParameter(paramName, encodeParamValue(builder.toString()));
                    builder = null;
                }
            } else {
                // To stop creating empty StringBuffer at the end of parameter,
                // we delay creating this object until this point.
                if (builder == null) {
                    builder = new StringBuilder();
                }
                builder.append(ch);
            }
        }
        if (insideDquote) {
            // e.g.
            // "non-quote-at-end
            Log.d(LOG_TAG, "Dangling Dquote.");
        }
        if (builder != null) {
            if (builder.length() == 0) {
                Log.w(LOG_TAG, "Unintended behavior. We must not see empty StringBuilder " +
                        "at the end of parameter value parsing.");
            } else {
                property.addParameter(paramName, encodeParamValue(builder.toString()));
            }
        }
    }

    /**
     * Encode a param value using UTF-8.
     */
    protected String encodeParamValue(String paramValue) {
        return VCardUtils.convertStringCharset(
                paramValue, VCardConfig.DEFAULT_INTERMEDIATE_CHARSET, "UTF-8");
    }

    @Override
    protected void handleAgent(VCardProperty property) {
        // The way how vCard 3.0 supports "AGENT" is completely different from vCard 2.1.
        //
        // e.g.
        // AGENT:BEGIN:VCARD\nFN:Joe Friday\nTEL:+1-919-555-7878\n
        //  TITLE:Area Administrator\, Assistant\n EMAIL\;TYPE=INTERN\n
        //  ET:jfriday@host.com\nEND:VCARD\n
        //
        // TODO: fix this.
        //
        // issue:
        //  vCard 3.0 also allows this as an example.
        //
        // AGENT;VALUE=uri:
        //  CID:JQPUBLIC.part3.960129T083020.xyzMail@host3.com
        //
        // This is not vCard. Should we support this?
        //
        // Just ignore the line for now, since we cannot know how to handle it...
        if (!mEmittedAgentWarning) {
            Log.w(LOG_TAG, "AGENT in vCard 3.0 is not supported yet. Ignore it");
            mEmittedAgentWarning = true;
        }
    }

    /**
     * This is only called from handlePropertyValue(), which has already
     * read the first line of this property. With v3.0, the getNonEmptyLine()
     * routine has already concatenated all following continuation lines.
     * The routine is implemented in the V21 parser to concatenate v2.1 style
     * data blocks, but is unnecessary here.
     */
    @Override
    protected String getBase64(final String firstString)
            throws IOException, VCardException {
        return firstString;
    }

    /**
     * ESCAPED-CHAR = "\\" / "\;" / "\," / "\n" / "\N")
     *              ; \\ encodes \, \n or \N encodes newline
     *              ; \; encodes ;, \, encodes ,
     *
     * Note: Apple escapes ':' into '\:' while does not escape '\'
     */
    @Override
    protected String maybeUnescapeText(final String text) {
        return unescapeText(text);
    }

    public static String unescapeText(final String text) {
        StringBuilder builder = new StringBuilder();
        final int length = text.length();
        for (int i = 0; i < length; i++) {
            char ch = text.charAt(i);
            if (ch == '\\' && i < length - 1) {
                final char next_ch = text.charAt(++i);
                if (next_ch == 'n' || next_ch == 'N') {
                    builder.append("\n");
                } else {
                    builder.append(next_ch);
                }
            } else {
                builder.append(ch);
            }
        }
        return builder.toString();
    }

    @Override
    protected String maybeUnescapeCharacter(final char ch) {
        return unescapeCharacter(ch);
    }

    public static String unescapeCharacter(final char ch) {
        if (ch == 'n' || ch == 'N') {
            return "\n";
        } else {
            return String.valueOf(ch);
        }
    }

    @Override
    protected Set<String> getKnownPropertyNameSet() {
        return VCardParser_V30.sKnownPropertyNameSet;
    }
}
