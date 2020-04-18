// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf.nano;

import android.os.Parcel;

import com.google.protobuf.nano.UnittestSimpleNano.SimpleMessageNano;
import com.google.protobuf.nano.UnittestSimpleNano.SimpleMessageNano.NestedMessage;
import com.google.protobuf.nano.testext.Extensions.ContainerMessage;
import com.google.protobuf.nano.testext.Extensions.ExtendableMessage;

import junit.framework.TestCase;

public class NanoAndroidTest extends TestCase {
    public void testParceling() {
        SimpleMessageNano message = new SimpleMessageNano();
        message.d = 54321;
        message.nestedMsg = new NestedMessage();
        message.nestedMsg.bb = 12345;
        message.defaultNestedEnum = SimpleMessageNano.FOO;

        Parcel parcel = null;
        try {
            parcel = Parcel.obtain();
            parcel.writeParcelable(message, 0);
            parcel.setDataPosition(0);
            message = parcel.readParcelable(getClass().getClassLoader());
        } finally {
            if (parcel != null) {
                parcel.recycle();
            }
        }

        assertEquals(54321, message.d);
        assertEquals(12345, message.nestedMsg.bb);
        assertEquals(SimpleMessageNano.FOO, message.defaultNestedEnum);
    }

    public void testExtendableParceling() {
        ExtendableMessage message = new ExtendableMessage();
        message.field = 12345;
        message.setExtension(ContainerMessage.anotherThing, true);

        Parcel parcel = null;
        try {
            parcel = Parcel.obtain();
            parcel.writeParcelable(message, 0);
            parcel.setDataPosition(0);
            message = parcel.readParcelable(getClass().getClassLoader());
        } finally {
            if (parcel != null) {
                parcel.recycle();
            }
        }

        assertEquals(12345, message.field);
        assertTrue((boolean) message.getExtension(ContainerMessage.anotherThing));
    }
}
