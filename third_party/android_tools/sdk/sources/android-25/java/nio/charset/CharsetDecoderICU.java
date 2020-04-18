/**
*******************************************************************************
* Copyright (C) 1996-2006, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
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
import libcore.icu.ICU;
import libcore.icu.NativeConverter;
import libcore.util.EmptyArray;

final class CharsetDecoderICU extends CharsetDecoder {
    private static final int MAX_CHARS_PER_BYTE = 2;

    private static final int INPUT_OFFSET = 0;
    private static final int OUTPUT_OFFSET = 1;
    private static final int INVALID_BYTE_COUNT = 2;
    /*
     * data[INPUT_OFFSET]   = on input contains the start of input and on output the number of input bytes consumed
     * data[OUTPUT_OFFSET]  = on input contains the start of output and on output the number of output chars written
     * data[INVALID_BYTE_COUNT]  = number of invalid bytes
     */
    private final int[] data = new int[3];

    /* handle to the ICU converter that is opened */
    private long converterHandle = 0;

    private byte[] input = null;
    private char[] output= null;

    private byte[] allocatedInput = null;
    private char[] allocatedOutput = null;

    // These instance variables are always assigned in the methods before being used. This class
    // is inherently thread-unsafe so we don't have to worry about synchronization.
    private int inEnd;
    private int outEnd;

    public static CharsetDecoderICU newInstance(Charset cs, String icuCanonicalName) {
        // This complexity is necessary to ensure that even if the constructor, superclass
        // constructor, or call to updateCallback throw, we still free the native peer.
        long address = 0;
        try {
            address = NativeConverter.openConverter(icuCanonicalName);
            float averageCharsPerByte = NativeConverter.getAveCharsPerByte(address);
            CharsetDecoderICU result = new CharsetDecoderICU(cs, averageCharsPerByte, address);
            address = 0; // CharsetDecoderICU has taken ownership; its finalizer will do the free.
            result.updateCallback();
            return result;
        } finally {
            if (address != 0) {
                NativeConverter.closeConverter(address);
            }
        }
    }

    private CharsetDecoderICU(Charset cs, float averageCharsPerByte, long address) {
        super(cs, averageCharsPerByte, MAX_CHARS_PER_BYTE);
        this.converterHandle = address;
        NativeConverter.registerConverter(this, converterHandle);
    }

    @Override protected void implReplaceWith(String newReplacement) {
        updateCallback();
     }

    @Override protected final void implOnMalformedInput(CodingErrorAction newAction) {
        updateCallback();
    }

    @Override protected final void implOnUnmappableCharacter(CodingErrorAction newAction) {
        updateCallback();
    }

    private void updateCallback() {
        NativeConverter.setCallbackDecode(converterHandle, this);
    }

    @Override protected void implReset() {
        NativeConverter.resetByteToChar(converterHandle);
        data[INPUT_OFFSET] = 0;
        data[OUTPUT_OFFSET] = 0;
        data[INVALID_BYTE_COUNT] = 0;
        output = null;
        input = null;
        allocatedInput = null;
        allocatedOutput = null;
        inEnd = 0;
        outEnd = 0;
    }

    @Override protected final CoderResult implFlush(CharBuffer out) {
        try {
            // ICU needs to see an empty input.
            input = EmptyArray.BYTE;
            inEnd = 0;
            data[INPUT_OFFSET] = 0;

            data[OUTPUT_OFFSET] = getArray(out);
            data[INVALID_BYTE_COUNT] = 0; // Make sure we don't see earlier errors.

            int error = NativeConverter.decode(converterHandle, input, inEnd, output, outEnd, data, true);
            if (ICU.U_FAILURE(error)) {
                if (error == ICU.U_BUFFER_OVERFLOW_ERROR) {
                    return CoderResult.OVERFLOW;
                } else if (error == ICU.U_TRUNCATED_CHAR_FOUND) {
                    if (data[INVALID_BYTE_COUNT] > 0) {
                        return CoderResult.malformedForLength(data[INVALID_BYTE_COUNT]);
                    }
                }
            }
            return CoderResult.UNDERFLOW;
       } finally {
            setPosition(out);
            implReset();
       }
    }

    @Override protected CoderResult decodeLoop(ByteBuffer in, CharBuffer out) {
        if (!in.hasRemaining()) {
            return CoderResult.UNDERFLOW;
        }

        data[INPUT_OFFSET] = getArray(in);
        data[OUTPUT_OFFSET]= getArray(out);

        try {
            int error = NativeConverter.decode(converterHandle, input, inEnd, output, outEnd, data, false);
            if (ICU.U_FAILURE(error)) {
                if (error == ICU.U_BUFFER_OVERFLOW_ERROR) {
                    return CoderResult.OVERFLOW;
                } else if (error == ICU.U_INVALID_CHAR_FOUND) {
                    return CoderResult.unmappableForLength(data[INVALID_BYTE_COUNT]);
                } else if (error == ICU.U_ILLEGAL_CHAR_FOUND) {
                    return CoderResult.malformedForLength(data[INVALID_BYTE_COUNT]);
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


    private int getArray(CharBuffer out) {
        if (out.hasArray()) {
            output = out.array();
            outEnd = out.arrayOffset() + out.limit();
            return out.arrayOffset() + out.position();
        } else {
            outEnd = out.remaining();
            if (allocatedOutput == null || outEnd > allocatedOutput.length) {
                allocatedOutput = new char[outEnd];
            }
            // The array's start position is 0.
            output = allocatedOutput;
            return 0;
        }
    }

    private  int getArray(ByteBuffer in) {
        if (in.hasArray()) {
            input = in.array();
            inEnd = in.arrayOffset() + in.limit();
            return in.arrayOffset() + in.position();
        } else {
            inEnd = in.remaining();
            if (allocatedInput == null || inEnd > allocatedInput.length) {
                allocatedInput = new byte[inEnd];
            }
            // Copy the input buffer into the allocated array.
            int pos = in.position();
            in.get(allocatedInput, 0, inEnd);
            in.position(pos);
            // The array's start position is 0.
            input = allocatedInput;
            return 0;
        }
    }

    private void setPosition(CharBuffer out) {
        if (out.hasArray()) {
            out.position(out.position() + data[OUTPUT_OFFSET]);
        } else {
            out.put(output, 0, data[OUTPUT_OFFSET]);
        }
        // release reference to output array, which may not be ours
        output = null;
    }

    private void setPosition(ByteBuffer in) {
        in.position(in.position() + data[INPUT_OFFSET]);
        // release reference to input array, which may not be ours
        input = null;
    }
}
