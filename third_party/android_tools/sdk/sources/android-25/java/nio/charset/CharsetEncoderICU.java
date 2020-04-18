/**
*******************************************************************************
* Copyright (C) 1996-2006, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                  *
*******************************************************************************
*
*******************************************************************************
*/
/**
 * A JNI interface for ICU converters.
 *
 *
 * @author Ram Viswanadha, IBM
 */
package java.nio.charset;

import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.util.HashMap;
import java.util.Map;
import libcore.icu.ICU;
import libcore.icu.NativeConverter;
import libcore.util.EmptyArray;
import libcore.util.NativeAllocationRegistry;

final class CharsetEncoderICU extends CharsetEncoder {
    private static final Map<String, byte[]> DEFAULT_REPLACEMENTS = new HashMap<String, byte[]>();
    static {
        // ICU has different default replacements to the RI in some cases. There are many
        // additional cases, but this covers all the charsets that Java guarantees will be
        // available, which is where compatibility seems most important. (The RI even uses
        // the byte corresponding to '?' in ASCII as the replacement byte for charsets where that
        // byte corresponds to an entirely different character.)
        // It's odd that UTF-8 doesn't use U+FFFD, given that (unlike ISO-8859-1 and US-ASCII) it
        // can represent it, but this is what the RI does...
        byte[] questionMark = new byte[] { (byte) '?' };
        DEFAULT_REPLACEMENTS.put("UTF-8",      questionMark);
        DEFAULT_REPLACEMENTS.put("ISO-8859-1", questionMark);
        DEFAULT_REPLACEMENTS.put("US-ASCII",   questionMark);
    }

    private static final int INPUT_OFFSET = 0;
    private static final int OUTPUT_OFFSET = 1;
    private static final int INVALID_CHAR_COUNT = 2;
    /*
     * data[INPUT_OFFSET]   = on input contains the start of input and on output the number of input chars consumed
     * data[OUTPUT_OFFSET]  = on input contains the start of output and on output the number of output bytes written
     * data[INVALID_CHARS]  = number of invalid chars
     */
    private int[] data = new int[3];

    /* handle to the ICU converter that is opened */
    private final long converterHandle;

    private char[] input = null;
    private byte[] output = null;

    private char[] allocatedInput = null;
    private byte[] allocatedOutput = null;

    // These instance variables are always assigned in the methods before being used. This class
    // is inherently thread-unsafe so we don't have to worry about synchronization.
    private int inEnd;
    private int outEnd;

    public static CharsetEncoderICU newInstance(Charset cs, String icuCanonicalName) {
        // This complexity is necessary to ensure that even if the constructor, superclass
        // constructor, or call to updateCallback throw, we still free the native peer.
        long address = 0;
        try {
            address = NativeConverter.openConverter(icuCanonicalName);
            float averageBytesPerChar = NativeConverter.getAveBytesPerChar(address);
            float maxBytesPerChar = NativeConverter.getMaxBytesPerChar(address);
            byte[] replacement = makeReplacement(icuCanonicalName, address);
            CharsetEncoderICU result = new CharsetEncoderICU(cs, averageBytesPerChar, maxBytesPerChar, replacement, address);
            address = 0; // CharsetEncoderICU has taken ownership; its finalizer will do the free.
            return result;
        } finally {
            if (address != 0) {
                NativeConverter.closeConverter(address);
            }
        }
    }

    private static byte[] makeReplacement(String icuCanonicalName, long address) {
        // We have our own map of RI-compatible default replacements (where ICU disagrees)...
        byte[] replacement = DEFAULT_REPLACEMENTS.get(icuCanonicalName);
        if (replacement != null) {
            return replacement.clone();
        }
        // ...but fall back to asking ICU.
        return NativeConverter.getSubstitutionBytes(address);
    }

    private CharsetEncoderICU(Charset cs, float averageBytesPerChar, float maxBytesPerChar, byte[] replacement, long address) {
        super(cs, averageBytesPerChar, maxBytesPerChar, replacement, true);
        // Our native peer needs to know what just happened...
        this.converterHandle = address;
        NativeConverter.registerConverter(this, converterHandle);
        updateCallback();
    }

    @Override protected void implReplaceWith(byte[] newReplacement) {
        updateCallback();
    }

    @Override protected void implOnMalformedInput(CodingErrorAction newAction) {
        updateCallback();
    }

    @Override protected void implOnUnmappableCharacter(CodingErrorAction newAction) {
        updateCallback();
    }

    private void updateCallback() {
        NativeConverter.setCallbackEncode(converterHandle, this);
    }

    @Override protected void implReset() {
        NativeConverter.resetCharToByte(converterHandle);
        data[INPUT_OFFSET] = 0;
        data[OUTPUT_OFFSET] = 0;
        data[INVALID_CHAR_COUNT] = 0;
        output = null;
        input = null;
        allocatedInput = null;
        allocatedOutput = null;
        inEnd = 0;
        outEnd = 0;
    }

    @Override protected CoderResult implFlush(ByteBuffer out) {
        try {
            // ICU needs to see an empty input.
            input = EmptyArray.CHAR;
            inEnd = 0;
            data[INPUT_OFFSET] = 0;

            data[OUTPUT_OFFSET] = getArray(out);
            data[INVALID_CHAR_COUNT] = 0; // Make sure we don't see earlier errors.

            int error = NativeConverter.encode(converterHandle, input, inEnd, output, outEnd, data, true);
            if (ICU.U_FAILURE(error)) {
                if (error == ICU.U_BUFFER_OVERFLOW_ERROR) {
                    return CoderResult.OVERFLOW;
                } else if (error == ICU.U_TRUNCATED_CHAR_FOUND) {
                    if (data[INVALID_CHAR_COUNT] > 0) {
                        return CoderResult.malformedForLength(data[INVALID_CHAR_COUNT]);
                    }
                }
            }
            return CoderResult.UNDERFLOW;
        } finally {
            setPosition(out);
            implReset();
        }
    }

    @Override protected CoderResult encodeLoop(CharBuffer in, ByteBuffer out) {
        if (!in.hasRemaining()) {
            return CoderResult.UNDERFLOW;
        }

        data[INPUT_OFFSET] = getArray(in);
        data[OUTPUT_OFFSET]= getArray(out);
        data[INVALID_CHAR_COUNT] = 0; // Make sure we don't see earlier errors.

        try {
            int error = NativeConverter.encode(converterHandle, input, inEnd, output, outEnd, data, false);
            if (ICU.U_FAILURE(error)) {
                if (error == ICU.U_BUFFER_OVERFLOW_ERROR) {
                    return CoderResult.OVERFLOW;
                } else if (error == ICU.U_INVALID_CHAR_FOUND) {
                    return CoderResult.unmappableForLength(data[INVALID_CHAR_COUNT]);
                } else if (error == ICU.U_ILLEGAL_CHAR_FOUND) {
                    return CoderResult.malformedForLength(data[INVALID_CHAR_COUNT]);
                } else {
                    throw new AssertionError(error);
                }
            }
            // Decoding succeeded: give us more data.
            return CoderResult.UNDERFLOW;
        } finally {
            setPosition(in);
            setPosition(out);
        }
    }

    private int getArray(ByteBuffer out) {
        if (out.hasArray()) {
            output = out.array();
            outEnd = out.arrayOffset() + out.limit();
            return out.arrayOffset() + out.position();
        } else {
            outEnd = out.remaining();
            if (allocatedOutput == null || outEnd > allocatedOutput.length) {
                allocatedOutput = new byte[outEnd];
            }
            // The array's start position is 0
            output = allocatedOutput;
            return 0;
        }
    }

    private int getArray(CharBuffer in) {
        if (in.hasArray()) {
            input = in.array();
            inEnd = in.arrayOffset() + in.limit();
            return in.arrayOffset() + in.position();
        } else {
            inEnd = in.remaining();
            if (allocatedInput == null || inEnd > allocatedInput.length) {
                allocatedInput = new char[inEnd];
            }
            // Copy the input buffer into the allocated array.
            int pos = in.position();
            in.get(allocatedInput, 0, inEnd);
            in.position(pos);
            // The array's start position is 0
            input = allocatedInput;
            return 0;
        }
    }

    private void setPosition(ByteBuffer out) {
        if (out.hasArray()) {
            out.position(data[OUTPUT_OFFSET] - out.arrayOffset());
        } else {
            out.put(output, 0, data[OUTPUT_OFFSET]);
        }
        // release reference to output array, which may not be ours
        output = null;
    }

    private void setPosition(CharBuffer in) {
        int position = in.position() + data[INPUT_OFFSET] - data[INVALID_CHAR_COUNT];
        if (position < 0) {
            // The calculated position might be negative if we encountered an
            // invalid char that spanned input buffers. We adjust it to 0 in this case.
            //
            // NOTE: The API doesn't allow us to adjust the position of the previous
            // input buffer. (Doing that wouldn't serve any useful purpose anyway.)
            position = 0;
        }

        in.position(position);
        // release reference to input array, which may not be ours
        input = null;
    }
}
