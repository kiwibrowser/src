/**
*******************************************************************************
* Copyright (C) 1996-2005, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
*******************************************************************************
*/

package java.nio.charset;

import libcore.icu.NativeConverter;

final class CharsetICU extends Charset {
    private final String icuCanonicalName;

    protected CharsetICU(String canonicalName, String icuCanonName, String[] aliases) {
         super(canonicalName, aliases);
         icuCanonicalName = icuCanonName;
    }

    public CharsetDecoder newDecoder() {
        return CharsetDecoderICU.newInstance(this, icuCanonicalName);
    }

    public CharsetEncoder newEncoder() {
        return CharsetEncoderICU.newInstance(this, icuCanonicalName);
    }

    public boolean contains(Charset cs) {
        if (cs == null) {
            return false;
        } else if (this.equals(cs)) {
            return true;
        }
        return NativeConverter.contains(this.name(), cs.name());
    }
}
