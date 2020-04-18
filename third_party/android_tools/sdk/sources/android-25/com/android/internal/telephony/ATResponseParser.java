/*
 * Copyright (C) 2006 The Android Open Source Project
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

/**
 * {@hide}
 */
public class ATResponseParser
{
    /*************************** Instance Variables **************************/

    private String mLine;
    private int mNext = 0;
    private int mTokStart, mTokEnd;

    /***************************** Class Methods *****************************/

    public
    ATResponseParser (String line)
    {
        mLine = line;
    }

    public boolean
    nextBoolean()
    {
        // "\s*(\d)(,|$)"
        // \d is '0' or '1'

        nextTok();

        if (mTokEnd - mTokStart > 1) {
            throw new ATParseEx();
        }
        char c = mLine.charAt(mTokStart);

        if (c == '0') return false;
        if (c ==  '1') return true;
        throw new ATParseEx();
    }


    /** positive int only */
    public int
    nextInt()
    {
        // "\s*(\d+)(,|$)"
        int ret = 0;

        nextTok();

        for (int i = mTokStart ; i < mTokEnd ; i++) {
            char c = mLine.charAt(i);

            // Yes, ASCII decimal digits only
            if (c < '0' || c > '9') {
                throw new ATParseEx();
            }

            ret *= 10;
            ret += c - '0';
        }

        return ret;
    }

    public String
    nextString()
    {
        nextTok();

        return mLine.substring(mTokStart, mTokEnd);
    }

    public boolean
    hasMore()
    {
        return mNext < mLine.length();
    }

    private void
    nextTok()
    {
        int len = mLine.length();

        if (mNext == 0) {
            skipPrefix();
        }

        if (mNext >= len) {
            throw new ATParseEx();
        }

        try {
            // \s*("([^"]*)"|(.*)\s*)(,|$)

            char c = mLine.charAt(mNext++);
            boolean hasQuote = false;

            c = skipWhiteSpace(c);

            if (c == '"') {
                if (mNext >= len) {
                    throw new ATParseEx();
                }
                c = mLine.charAt(mNext++);
                mTokStart = mNext - 1;
                while (c != '"' && mNext < len) {
                    c = mLine.charAt(mNext++);
                }
                if (c != '"') {
                    throw new ATParseEx();
                }
                mTokEnd = mNext - 1;
                if (mNext < len && mLine.charAt(mNext++) != ',') {
                    throw new ATParseEx();
                }
            } else {
                mTokStart = mNext - 1;
                mTokEnd = mTokStart;
                while (c != ',') {
                    if (!Character.isWhitespace(c)) {
                        mTokEnd = mNext;
                    }
                    if (mNext == len) {
                        break;
                    }
                    c = mLine.charAt(mNext++);
                }
            }
        } catch (StringIndexOutOfBoundsException ex) {
            throw new ATParseEx();
        }
    }


    /** Throws ATParseEx if whitespace extends to the end of string */
    private char
    skipWhiteSpace (char c)
    {
        int len;
        len = mLine.length();
        while (mNext < len && Character.isWhitespace(c)) {
            c = mLine.charAt(mNext++);
        }

        if (Character.isWhitespace(c)) {
            throw new ATParseEx();
        }
        return c;
    }


    private void
    skipPrefix()
    {
        // consume "^[^:]:"

        mNext = 0;
        int s = mLine.length();
        while (mNext < s){
            char c = mLine.charAt(mNext++);

            if (c == ':') {
                return;
            }
        }

        throw new ATParseEx("missing prefix");
    }

}
