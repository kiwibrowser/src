/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.common;

import android.text.TextUtils;
import android.text.util.Rfc822Token;
import android.text.util.Rfc822Tokenizer;
import android.util.Patterns;
import android.widget.AutoCompleteTextView;

import java.util.regex.Pattern;

/**
 * This class works as a Validator for AutoCompleteTextView for
 * email addresses.  If a token does not appear to be a valid address,
 * it is trimmed of characters that cannot legitimately appear in one
 * and has the specified domain name added.  It is meant for use with
 * {@link Rfc822Token} and {@link Rfc822Tokenizer}.
 *
 * @deprecated In the future make sure we don't quietly alter the user's
 *             text in ways they did not intend.  Meanwhile, hide this
 *             class from the public API because it does not even have
 *             a full understanding of the syntax it claims to correct.
 * @hide
 */
@Deprecated
public class Rfc822Validator implements AutoCompleteTextView.Validator {
    /**
     * Expression that matches the local part of an email address.
     * This expression does not follow the constraints of the RFC towards the dots, because the
     * de facto standard is to allow them anywhere.
     *
     * It is however a simplification and it will not validate the double-quote syntax.
     */
    private static final String EMAIL_ADDRESS_LOCALPART_REGEXP =
        "((?!\\s)[\\.\\w!#$%&'*+\\-/=?^`{|}~\u0080-\uFFFE])+";

    /**
     * Alias of characters that can be used in IRI, as per RFC 3987.
     */
    private static final String GOOD_IRI_CHAR = Patterns.GOOD_IRI_CHAR;

    /**
     * Regular expression for a domain label, as per RFC 3490.
     * Its total length must not exceed 63 octets, according to RFC 5890.
     */
    private static final String LABEL_REGEXP =
        "([" + GOOD_IRI_CHAR + "][" + GOOD_IRI_CHAR + "\\-]{0,61})?[" + GOOD_IRI_CHAR + "]";

    /**
     * Expression that matches a domain name, including international domain names in Punycode or
     * Unicode.
     */
    private static final String DOMAIN_REGEXP =
        "("+ LABEL_REGEXP + "\\.)+"                 // Subdomains and domain
        // Top-level domain must be at least 2 chars
        + "[" + GOOD_IRI_CHAR + "][" + GOOD_IRI_CHAR + "\\-]{0,61}[" + GOOD_IRI_CHAR + "]";

    /**
     * Pattern for an email address.
     *
     * It is similar to {@link android.util.Patterns#EMAIL_ADDRESS}, but also accepts Unicode
     * characters.
     */
    private static final Pattern EMAIL_ADDRESS_PATTERN =
            Pattern.compile(EMAIL_ADDRESS_LOCALPART_REGEXP + "@" + DOMAIN_REGEXP);

    private String mDomain;
    private boolean mRemoveInvalid = false;

    /**
     * Constructs a new validator that uses the specified domain name as
     * the default when none is specified.
     */
    public Rfc822Validator(String domain) {
        mDomain = domain;
    }

    /**
     * {@inheritDoc}
     */
    public boolean isValid(CharSequence text) {
        Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(text);
        return tokens.length == 1 &&
               EMAIL_ADDRESS_PATTERN.
                   matcher(tokens[0].getAddress()).matches();
    }

    /**
     * Specify if the validator should remove invalid tokens instead of trying
     * to fix them. This can be used to strip results of incorrectly formatted
     * tokens.
     *
     * @param remove true to remove tokens with the wrong format, false to
     *            attempt to fix them
     */
    public void setRemoveInvalid(boolean remove) {
        mRemoveInvalid = remove;
    }

    /**
     * @return a string in which all the characters that are illegal for the username
     * or the domain name part of the email address have been removed.
     */
    private String removeIllegalCharacters(String s) {
        StringBuilder result = new StringBuilder();
        int length = s.length();
        for (int i = 0; i < length; i++) {
            char c = s.charAt(i);

            /*
             * An RFC822 atom can contain any ASCII printing character
             * except for periods and any of the following punctuation.
             * A local-part can contain multiple atoms, concatenated by
             * periods, so do allow periods here.
             */

            if (c <= ' ' || c > '~') {
                continue;
            }

            if (c == '(' || c == ')' || c == '<' || c == '>' ||
                c == '@' || c == ',' || c == ';' || c == ':' ||
                c == '\\' || c == '"' || c == '[' || c == ']') {
                continue;
            }

            result.append(c);
        }
        return result.toString();
    }

    /**
     * {@inheritDoc}
     */
    public CharSequence fixText(CharSequence cs) {
        // Return an empty string if the email address only contains spaces, \n or \t
        if (TextUtils.getTrimmedLength(cs) == 0) return "";

        Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(cs);
        StringBuilder sb = new StringBuilder();

        for (int i = 0; i < tokens.length; i++) {
            String text = tokens[i].getAddress();

            if (mRemoveInvalid && !isValid(text)) {
                continue;
            }
            int index = text.indexOf('@');
            if (index < 0) {
                // append the domain of the account if it exists
                if (mDomain != null) {
                    tokens[i].setAddress(removeIllegalCharacters(text) + "@" + mDomain);
                }
            } else {
                // Otherwise, remove the illegal characters on both sides of the '@'
                String fix = removeIllegalCharacters(text.substring(0, index));
                if (TextUtils.isEmpty(fix)) {
                    // if the address is empty after removing invalid chars
                    // don't use it
                    continue;
                }
                String domain = removeIllegalCharacters(text.substring(index + 1));
                boolean emptyDomain = domain.length() == 0;
                if (!emptyDomain || mDomain != null) {
                    tokens[i].setAddress(fix + "@" + (!emptyDomain ? domain : mDomain));
                }
            }

            sb.append(tokens[i].toString());
            if (i + 1 < tokens.length) {
                sb.append(", ");
            }
        }

        return sb;
    }
}
