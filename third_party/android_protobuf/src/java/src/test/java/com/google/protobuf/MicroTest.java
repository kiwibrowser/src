// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

package com.google.protobuf;

import com.google.protobuf.micro.ByteStringMicro;
import com.google.protobuf.micro.CodedInputStreamMicro;
import com.google.protobuf.micro.FileScopeEnumRefMicro;
import com.google.protobuf.micro.MessageScopeEnumRefMicro;
import com.google.protobuf.micro.MicroOuterClass;
import com.google.protobuf.micro.MicroOuterClass.TestAllTypesMicro;
import com.google.protobuf.micro.MultipleImportingNonMultipleMicro1;
import com.google.protobuf.micro.MultipleImportingNonMultipleMicro2;
import com.google.protobuf.micro.MultipleNameClashMicro;
import com.google.protobuf.micro.UnittestImportMicro;
import com.google.protobuf.micro.UnittestMultipleMicro;
import com.google.protobuf.micro.UnittestRecursiveMicro.RecursiveMessageMicro;
import com.google.protobuf.micro.UnittestSimpleMicro.SimpleMessageMicro;
import com.google.protobuf.micro.UnittestSingleMicro.SingleMessageMicro;
import com.google.protobuf.micro.UnittestStringutf8Micro.StringUtf8;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.FilterInputStream;
import java.io.InputStream;
import java.io.IOException;

/**
 * Test micro runtime.
 *
 * @author wink@google.com Wink Saville
 */
public class MicroTest extends TestCase {
  public void setUp() throws Exception {
  }

  public void testSimpleMessageMicro() throws Exception {
    SimpleMessageMicro msg = new SimpleMessageMicro();
    assertFalse(msg.hasD());
    assertEquals(123, msg.getD());
    assertFalse(msg.hasNestedMsg());
    assertEquals(null, msg.getNestedMsg());
    assertFalse(msg.hasDefaultNestedEnum());
    assertEquals(SimpleMessageMicro.BAZ, msg.getDefaultNestedEnum());

    msg.setD(456);
    assertTrue(msg.hasD());
    assertEquals(456, msg.getD());
    msg.clearD()
       .setD(456);
    assertTrue(msg.hasD());

    SimpleMessageMicro.NestedMessage nestedMsg = new SimpleMessageMicro.NestedMessage()
      .setBb(2);
    assertTrue(nestedMsg.hasBb());
    assertEquals(2, nestedMsg.getBb());
    msg.setNestedMsg(nestedMsg);
    assertTrue(msg.hasNestedMsg());
    assertEquals(2, msg.getNestedMsg().getBb());

    msg.setDefaultNestedEnum(SimpleMessageMicro.BAR);
    assertTrue(msg.hasDefaultNestedEnum());
    assertEquals(SimpleMessageMicro.BAR, msg.getDefaultNestedEnum());

    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    SimpleMessageMicro newMsg = SimpleMessageMicro.parseFrom(result);
    assertTrue(newMsg.hasD());
    assertTrue(newMsg.hasNestedMsg());
    assertTrue(newMsg.hasDefaultNestedEnum());
    assertEquals(456, newMsg.getD());
    assertEquals(2, msg.getNestedMsg().getBb());
    assertEquals(SimpleMessageMicro.BAR, msg.getDefaultNestedEnum());
  }

  public void testRecursiveMessageMicro() throws Exception {
    RecursiveMessageMicro msg = new RecursiveMessageMicro();
    assertFalse(msg.hasId());
    assertFalse(msg.hasNestedMessage());
    assertFalse(msg.hasOptionalRecursiveMessageMicro());
    assertEquals(0, msg.getRepeatedRecursiveMessageMicroCount());

    RecursiveMessageMicro msg1 = new RecursiveMessageMicro();
    msg1.setId(1);
    assertEquals(1, msg1.getId());
    RecursiveMessageMicro msg2 = new RecursiveMessageMicro();
    msg2.setId(2);
    RecursiveMessageMicro msg3 = new RecursiveMessageMicro();
    msg3.setId(3);

    RecursiveMessageMicro.NestedMessage nestedMsg = new RecursiveMessageMicro.NestedMessage();
    nestedMsg.setA(msg1);
    assertEquals(1, nestedMsg.getA().getId());

    msg.setId(0);
    msg.setNestedMessage(nestedMsg);
    msg.setOptionalRecursiveMessageMicro(msg2);
    msg.addRepeatedRecursiveMessageMicro(msg3);

    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 16);
    assertEquals(result.length, msgSerializedSize);

    RecursiveMessageMicro newMsg = RecursiveMessageMicro.parseFrom(result);
    assertTrue(newMsg.hasId());
    assertTrue(newMsg.hasNestedMessage());
    assertTrue(newMsg.hasOptionalRecursiveMessageMicro());
    assertEquals(1, newMsg.getRepeatedRecursiveMessageMicroCount());

    assertEquals(0, newMsg.getId());
    assertEquals(1, newMsg.getNestedMessage().getA().getId());
    assertEquals(2, newMsg.getOptionalRecursiveMessageMicro().getId());
    assertEquals(3, newMsg.getRepeatedRecursiveMessageMicro(0).getId());
  }

  public void testMicroRequiredInt32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasId());
    assertFalse(msg.isInitialized());
    msg.setId(123);
    assertTrue(msg.hasId());
    assertTrue(msg.isInitialized());
    assertEquals(123, msg.getId());
    msg.clearId();
    assertFalse(msg.hasId());
    assertFalse(msg.isInitialized());
    msg.clearId()
       .setId(456);
    assertTrue(msg.hasId());
    msg.clear();
    assertFalse(msg.hasId());
    assertFalse(msg.isInitialized());

    msg.setId(123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasId());
    assertTrue(newMsg.isInitialized());
    assertEquals(123, newMsg.getId());
  }

  public void testMicroOptionalInt32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalInt32());
    msg.setOptionalInt32(123);
    assertTrue(msg.hasOptionalInt32());
    assertEquals(123, msg.getOptionalInt32());
    msg.clearOptionalInt32();
    assertFalse(msg.hasOptionalInt32());
    msg.clearOptionalInt32()
       .setOptionalInt32(456);
    assertTrue(msg.hasOptionalInt32());
    msg.clear();
    assertFalse(msg.hasOptionalInt32());

    msg.setOptionalInt32(123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 2);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalInt32());
    assertEquals(123, newMsg.getOptionalInt32());
  }

  public void testMicroOptionalInt64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalInt64());
    msg.setOptionalInt64(123);
    assertTrue(msg.hasOptionalInt64());
    assertEquals(123, msg.getOptionalInt64());
    msg.clearOptionalInt64();
    assertFalse(msg.hasOptionalInt64());
    msg.clearOptionalInt64()
       .setOptionalInt64(456);
    assertTrue(msg.hasOptionalInt64());
    msg.clear();
    assertFalse(msg.hasOptionalInt64());

    msg.setOptionalInt64(123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 2);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalInt64());
    assertEquals(123, newMsg.getOptionalInt64());
  }

  public void testMicroOptionalUint32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalUint32());
    msg.setOptionalUint32(123);
    assertTrue(msg.hasOptionalUint32());
    assertEquals(123, msg.getOptionalUint32());
    msg.clearOptionalUint32();
    assertFalse(msg.hasOptionalUint32());
    msg.clearOptionalUint32()
       .setOptionalUint32(456);
    assertTrue(msg.hasOptionalUint32());
    msg.clear();
    assertFalse(msg.hasOptionalUint32());

    msg.setOptionalUint32(123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 2);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalUint32());
    assertEquals(123, newMsg.getOptionalUint32());
  }

  public void testMicroOptionalUint64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalUint64());
    msg.setOptionalUint64(123);
    assertTrue(msg.hasOptionalUint64());
    assertEquals(123, msg.getOptionalUint64());
    msg.clearOptionalUint64();
    assertFalse(msg.hasOptionalUint64());
    msg.clearOptionalUint64()
       .setOptionalUint64(456);
    assertTrue(msg.hasOptionalUint64());
    msg.clear();
    assertFalse(msg.hasOptionalUint64());

    msg.setOptionalUint64(123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 2);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalUint64());
    assertEquals(123, newMsg.getOptionalUint64());
  }

  public void testMicroOptionalSint32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalSint32());
    msg.setOptionalSint32(123);
    assertTrue(msg.hasOptionalSint32());
    assertEquals(123, msg.getOptionalSint32());
    msg.clearOptionalSint32();
    assertFalse(msg.hasOptionalSint32());
    msg.clearOptionalSint32()
       .setOptionalSint32(456);
    assertTrue(msg.hasOptionalSint32());
    msg.clear();
    assertFalse(msg.hasOptionalSint32());

    msg.setOptionalSint32(-123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalSint32());
    assertEquals(-123, newMsg.getOptionalSint32());
  }

  public void testMicroOptionalSint64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalSint64());
    msg.setOptionalSint64(123);
    assertTrue(msg.hasOptionalSint64());
    assertEquals(123, msg.getOptionalSint64());
    msg.clearOptionalSint64();
    assertFalse(msg.hasOptionalSint64());
    msg.clearOptionalSint64()
       .setOptionalSint64(456);
    assertTrue(msg.hasOptionalSint64());
    msg.clear();
    assertFalse(msg.hasOptionalSint64());

    msg.setOptionalSint64(-123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalSint64());
    assertEquals(-123, newMsg.getOptionalSint64());
  }

  public void testMicroOptionalFixed32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalFixed32());
    msg.setOptionalFixed32(123);
    assertTrue(msg.hasOptionalFixed32());
    assertEquals(123, msg.getOptionalFixed32());
    msg.clearOptionalFixed32();
    assertFalse(msg.hasOptionalFixed32());
    msg.clearOptionalFixed32()
       .setOptionalFixed32(456);
    assertTrue(msg.hasOptionalFixed32());
    msg.clear();
    assertFalse(msg.hasOptionalFixed32());

    msg.setOptionalFixed32(123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalFixed32());
    assertEquals(123, newMsg.getOptionalFixed32());
  }

  public void testMicroOptionalFixed64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalFixed64());
    msg.setOptionalFixed64(123);
    assertTrue(msg.hasOptionalFixed64());
    assertEquals(123, msg.getOptionalFixed64());
    msg.clearOptionalFixed64();
    assertFalse(msg.hasOptionalFixed64());
    msg.clearOptionalFixed64()
       .setOptionalFixed64(456);
    assertTrue(msg.hasOptionalFixed64());
    msg.clear();
    assertFalse(msg.hasOptionalFixed64());

    msg.setOptionalFixed64(123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalFixed64());
    assertEquals(123, newMsg.getOptionalFixed64());
  }
  public void testMicroOptionalSfixed32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalSfixed32());
    msg.setOptionalSfixed32(123);
    assertTrue(msg.hasOptionalSfixed32());
    assertEquals(123, msg.getOptionalSfixed32());
    msg.clearOptionalSfixed32();
    assertFalse(msg.hasOptionalSfixed32());
    msg.clearOptionalSfixed32()
       .setOptionalSfixed32(456);
    assertTrue(msg.hasOptionalSfixed32());
    msg.clear();
    assertFalse(msg.hasOptionalSfixed32());

    msg.setOptionalSfixed32(123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalSfixed32());
    assertEquals(123, newMsg.getOptionalSfixed32());
  }

  public void testMicroOptionalSfixed64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalSfixed64());
    msg.setOptionalSfixed64(123);
    assertTrue(msg.hasOptionalSfixed64());
    assertEquals(123, msg.getOptionalSfixed64());
    msg.clearOptionalSfixed64();
    assertFalse(msg.hasOptionalSfixed64());
    msg.clearOptionalSfixed64()
       .setOptionalSfixed64(456);
    assertTrue(msg.hasOptionalSfixed64());
    msg.clear();
    assertFalse(msg.hasOptionalSfixed64());

    msg.setOptionalSfixed64(-123);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalSfixed64());
    assertEquals(-123, newMsg.getOptionalSfixed64());
  }

  public void testMicroOptionalFloat() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalFloat());
    msg.setOptionalFloat(123f);
    assertTrue(msg.hasOptionalFloat());
    assertTrue(123.0f == msg.getOptionalFloat());
    msg.clearOptionalFloat();
    assertFalse(msg.hasOptionalFloat());
    msg.clearOptionalFloat()
       .setOptionalFloat(456.0f);
    assertTrue(msg.hasOptionalFloat());
    msg.clear();
    assertFalse(msg.hasOptionalFloat());

    msg.setOptionalFloat(-123.456f);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalFloat());
    assertTrue(-123.456f == newMsg.getOptionalFloat());
  }

  public void testMicroOptionalDouble() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalDouble());
    msg.setOptionalDouble(123);
    assertTrue(msg.hasOptionalDouble());
    assertTrue(123.0 == msg.getOptionalDouble());
    msg.clearOptionalDouble();
    assertFalse(msg.hasOptionalDouble());
    msg.clearOptionalDouble()
       .setOptionalDouble(456.0);
    assertTrue(msg.hasOptionalDouble());
    msg.clear();
    assertFalse(msg.hasOptionalDouble());

    msg.setOptionalDouble(-123.456);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalDouble());
    assertTrue(-123.456 == newMsg.getOptionalDouble());
  }

  public void testMicroOptionalBool() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalBool());
    msg.setOptionalBool(true);
    assertTrue(msg.hasOptionalBool());
    assertEquals(true, msg.getOptionalBool());
    msg.clearOptionalBool();
    assertFalse(msg.hasOptionalBool());
    msg.clearOptionalBool()
       .setOptionalBool(true);
    assertTrue(msg.hasOptionalBool());
    msg.clear();
    assertFalse(msg.hasOptionalBool());

    msg.setOptionalBool(false);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 2);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalBool());
    assertEquals(false, newMsg.getOptionalBool());
  }

  public void testMicroOptionalString() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalString());
    msg.setOptionalString("hello");
    assertTrue(msg.hasOptionalString());
    assertEquals("hello", msg.getOptionalString());
    msg.clearOptionalString();
    assertFalse(msg.hasOptionalString());
    msg.clearOptionalString()
       .setOptionalString("hello");
    assertTrue(msg.hasOptionalString());
    msg.clear();
    assertFalse(msg.hasOptionalString());

    msg.setOptionalString("bye");
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalString());
    assertEquals("bye", newMsg.getOptionalString());
  }

  public void testMicroOptionalBytes() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalBytes());
    msg.setOptionalBytes(ByteStringMicro.copyFromUtf8("hello"));
    assertTrue(msg.hasOptionalBytes());
    assertEquals("hello", msg.getOptionalBytes().toStringUtf8());
    msg.clearOptionalBytes();
    assertFalse(msg.hasOptionalBytes());
    msg.clearOptionalBytes()
       .setOptionalBytes(ByteStringMicro.copyFromUtf8("hello"));
    assertTrue(msg.hasOptionalBytes());
    msg.clear();
    assertFalse(msg.hasOptionalBytes());

    msg.setOptionalBytes(ByteStringMicro.copyFromUtf8("bye"));
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalBytes());
    assertEquals("bye", newMsg.getOptionalBytes().toStringUtf8());
  }

  public void testMicroOptionalGroup() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    TestAllTypesMicro.OptionalGroup grp = new TestAllTypesMicro.OptionalGroup();
    grp.setA(1);
    assertFalse(msg.hasOptionalGroup());
    msg.setOptionalGroup(grp);
    assertTrue(msg.hasOptionalGroup());
    assertEquals(1, msg.getOptionalGroup().getA());
    msg.clearOptionalGroup();
    assertFalse(msg.hasOptionalGroup());
    msg.clearOptionalGroup()
       .setOptionalGroup(new TestAllTypesMicro.OptionalGroup().setA(2));
    assertTrue(msg.hasOptionalGroup());
    msg.clear();
    assertFalse(msg.hasOptionalGroup());

    msg.setOptionalGroup(grp);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalGroup());
    assertEquals(1, newMsg.getOptionalGroup().getA());
  }

  public void testMicroOptionalNestedMessage() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    TestAllTypesMicro.NestedMessage nestedMsg = new TestAllTypesMicro.NestedMessage();
    nestedMsg.setBb(1);
    assertFalse(msg.hasOptionalNestedMessage());
    msg.setOptionalNestedMessage(nestedMsg);
    assertTrue(msg.hasOptionalNestedMessage());
    assertEquals(1, msg.getOptionalNestedMessage().getBb());
    msg.clearOptionalNestedMessage();
    assertFalse(msg.hasOptionalNestedMessage());
    msg.clearOptionalNestedMessage()
       .setOptionalNestedMessage(new TestAllTypesMicro.NestedMessage().setBb(2));
    assertTrue(msg.hasOptionalNestedMessage());
    msg.clear();
    assertFalse(msg.hasOptionalNestedMessage());

    msg.setOptionalNestedMessage(nestedMsg);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalNestedMessage());
    assertEquals(1, newMsg.getOptionalNestedMessage().getBb());
  }

  public void testMicroOptionalForeignMessage() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    MicroOuterClass.ForeignMessageMicro foreignMsg =
        new MicroOuterClass.ForeignMessageMicro();
    assertFalse(foreignMsg.hasC());
    foreignMsg.setC(1);
    assertTrue(foreignMsg.hasC());
    assertFalse(msg.hasOptionalForeignMessage());
    msg.setOptionalForeignMessage(foreignMsg);
    assertTrue(msg.hasOptionalForeignMessage());
    assertEquals(1, msg.getOptionalForeignMessage().getC());
    msg.clearOptionalForeignMessage();
    assertFalse(msg.hasOptionalForeignMessage());
    msg.clearOptionalForeignMessage()
       .setOptionalForeignMessage(new MicroOuterClass.ForeignMessageMicro().setC(2));
    assertTrue(msg.hasOptionalForeignMessage());
    msg.clear();
    assertFalse(msg.hasOptionalForeignMessage());

    msg.setOptionalForeignMessage(foreignMsg);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalForeignMessage());
    assertEquals(1, newMsg.getOptionalForeignMessage().getC());
  }

  public void testMicroOptionalImportMessage() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    UnittestImportMicro.ImportMessageMicro importMsg =
        new UnittestImportMicro.ImportMessageMicro();
    assertFalse(importMsg.hasD());
    importMsg.setD(1);
    assertTrue(importMsg.hasD());
    assertFalse(msg.hasOptionalImportMessage());
    msg.setOptionalImportMessage(importMsg);
    assertTrue(msg.hasOptionalImportMessage());
    assertEquals(1, msg.getOptionalImportMessage().getD());
    msg.clearOptionalImportMessage();
    assertFalse(msg.hasOptionalImportMessage());
    msg.clearOptionalImportMessage()
       .setOptionalImportMessage(new UnittestImportMicro.ImportMessageMicro().setD(2));
    assertTrue(msg.hasOptionalImportMessage());
    msg.clear();
    assertFalse(msg.hasOptionalImportMessage());

    msg.setOptionalImportMessage(importMsg);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalImportMessage());
    assertEquals(1, newMsg.getOptionalImportMessage().getD());
  }

  public void testMicroOptionalNestedEnum() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    msg.setOptionalNestedEnum(TestAllTypesMicro.BAR);
    assertTrue(msg.hasOptionalNestedEnum());
    assertEquals(TestAllTypesMicro.BAR, msg.getOptionalNestedEnum());
    msg.clearOptionalNestedEnum();
    assertFalse(msg.hasOptionalNestedEnum());
    msg.clearOptionalNestedEnum()
       .setOptionalNestedEnum(TestAllTypesMicro.BAZ);
    assertTrue(msg.hasOptionalNestedEnum());
    msg.clear();
    assertFalse(msg.hasOptionalNestedEnum());

    msg.setOptionalNestedEnum(TestAllTypesMicro.BAR);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalNestedEnum());
    assertEquals(TestAllTypesMicro.BAR, newMsg.getOptionalNestedEnum());
  }

  public void testMicroOptionalForeignEnum() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    msg.setOptionalForeignEnum(MicroOuterClass.FOREIGN_MICRO_BAR);
    assertTrue(msg.hasOptionalForeignEnum());
    assertEquals(MicroOuterClass.FOREIGN_MICRO_BAR,
        msg.getOptionalForeignEnum());
    msg.clearOptionalForeignEnum();
    assertFalse(msg.hasOptionalForeignEnum());
    msg.clearOptionalForeignEnum()
       .setOptionalForeignEnum(MicroOuterClass.FOREIGN_MICRO_BAZ);
    assertTrue(msg.hasOptionalForeignEnum());
    msg.clear();
    assertFalse(msg.hasOptionalForeignEnum());

    msg.setOptionalForeignEnum(MicroOuterClass.FOREIGN_MICRO_BAR);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalForeignEnum());
    assertEquals(MicroOuterClass.FOREIGN_MICRO_BAR,
        newMsg.getOptionalForeignEnum());
  }

  public void testMicroOptionalImportEnum() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    msg.setOptionalImportEnum(UnittestImportMicro.IMPORT_MICRO_BAR);
    assertTrue(msg.hasOptionalImportEnum());
    assertEquals(UnittestImportMicro.IMPORT_MICRO_BAR,
        msg.getOptionalImportEnum());
    msg.clearOptionalImportEnum();
    assertFalse(msg.hasOptionalImportEnum());
    msg.clearOptionalImportEnum()
       .setOptionalImportEnum(UnittestImportMicro.IMPORT_MICRO_BAZ);
    assertTrue(msg.hasOptionalImportEnum());
    msg.clear();
    assertFalse(msg.hasOptionalImportEnum());

    msg.setOptionalImportEnum(UnittestImportMicro.IMPORT_MICRO_BAR);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalImportEnum());
    assertEquals(UnittestImportMicro.IMPORT_MICRO_BAR,
        newMsg.getOptionalImportEnum());
  }

  public void testMicroOptionalStringPiece() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalStringPiece());
    msg.setOptionalStringPiece("hello");
    assertTrue(msg.hasOptionalStringPiece());
    assertEquals("hello", msg.getOptionalStringPiece());
    msg.clearOptionalStringPiece();
    assertFalse(msg.hasOptionalStringPiece());
    msg.clearOptionalStringPiece()
       .setOptionalStringPiece("hello");
    assertTrue(msg.hasOptionalStringPiece());
    msg.clear();
    assertFalse(msg.hasOptionalStringPiece());

    msg.setOptionalStringPiece("bye");
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalStringPiece());
    assertEquals("bye", newMsg.getOptionalStringPiece());
  }

  public void testMicroOptionalCord() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasOptionalCord());
    msg.setOptionalCord("hello");
    assertTrue(msg.hasOptionalCord());
    assertEquals("hello", msg.getOptionalCord());
    msg.clearOptionalCord();
    assertFalse(msg.hasOptionalCord());
    msg.clearOptionalCord()
      .setOptionalCord("hello");
    assertTrue(msg.hasOptionalCord());
    msg.clear();
    assertFalse(msg.hasOptionalCord());

    msg.setOptionalCord("bye");
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertTrue(newMsg.hasOptionalCord());
    assertEquals("bye", newMsg.getOptionalCord());
  }

  public void testMicroRepeatedInt32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedInt32Count());
    msg.addRepeatedInt32(123);
    assertEquals(1, msg.getRepeatedInt32Count());
    assertEquals(123, msg.getRepeatedInt32(0));
    msg.addRepeatedInt32(456);
    assertEquals(2, msg.getRepeatedInt32Count());
    assertEquals(123, msg.getRepeatedInt32(0));
    assertEquals(456, msg.getRepeatedInt32(1));
    msg.setRepeatedInt32(0, 789);
    assertEquals(2, msg.getRepeatedInt32Count());
    assertEquals(789, msg.getRepeatedInt32(0));
    assertEquals(456, msg.getRepeatedInt32(1));
    msg.clearRepeatedInt32();
    assertEquals(0, msg.getRepeatedInt32Count());
    msg.clearRepeatedInt32()
       .addRepeatedInt32(456);
    assertEquals(1, msg.getRepeatedInt32Count());
    assertEquals(456, msg.getRepeatedInt32(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedInt32Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedInt32(123);
    assertEquals(1, msg.getRepeatedInt32Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedInt32Count());
    assertEquals(123, newMsg.getRepeatedInt32(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedInt32(123)
       .addRepeatedInt32(456);
    assertEquals(2, msg.getRepeatedInt32Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedInt32Count());
    assertEquals(123, newMsg.getRepeatedInt32(0));
    assertEquals(456, newMsg.getRepeatedInt32(1));
  }

  public void testMicroRepeatedInt64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedInt64Count());
    msg.addRepeatedInt64(123);
    assertEquals(1, msg.getRepeatedInt64Count());
    assertEquals(123, msg.getRepeatedInt64(0));
    msg.addRepeatedInt64(456);
    assertEquals(2, msg.getRepeatedInt64Count());
    assertEquals(123, msg.getRepeatedInt64(0));
    assertEquals(456, msg.getRepeatedInt64(1));
    msg.setRepeatedInt64(0, 789);
    assertEquals(2, msg.getRepeatedInt64Count());
    assertEquals(789, msg.getRepeatedInt64(0));
    assertEquals(456, msg.getRepeatedInt64(1));
    msg.clearRepeatedInt64();
    assertEquals(0, msg.getRepeatedInt64Count());
    msg.clearRepeatedInt64()
       .addRepeatedInt64(456);
    assertEquals(1, msg.getRepeatedInt64Count());
    assertEquals(456, msg.getRepeatedInt64(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedInt64Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedInt64(123);
    assertEquals(1, msg.getRepeatedInt64Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedInt64Count());
    assertEquals(123, newMsg.getRepeatedInt64(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedInt64(123)
       .addRepeatedInt64(456);
    assertEquals(2, msg.getRepeatedInt64Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedInt64Count());
    assertEquals(123, newMsg.getRepeatedInt64(0));
    assertEquals(456, newMsg.getRepeatedInt64(1));
  }

  public void testMicroRepeatedUint32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedUint32Count());
    msg.addRepeatedUint32(123);
    assertEquals(1, msg.getRepeatedUint32Count());
    assertEquals(123, msg.getRepeatedUint32(0));
    msg.addRepeatedUint32(456);
    assertEquals(2, msg.getRepeatedUint32Count());
    assertEquals(123, msg.getRepeatedUint32(0));
    assertEquals(456, msg.getRepeatedUint32(1));
    msg.setRepeatedUint32(0, 789);
    assertEquals(2, msg.getRepeatedUint32Count());
    assertEquals(789, msg.getRepeatedUint32(0));
    assertEquals(456, msg.getRepeatedUint32(1));
    msg.clearRepeatedUint32();
    assertEquals(0, msg.getRepeatedUint32Count());
    msg.clearRepeatedUint32()
       .addRepeatedUint32(456);
    assertEquals(1, msg.getRepeatedUint32Count());
    assertEquals(456, msg.getRepeatedUint32(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedUint32Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedUint32(123);
    assertEquals(1, msg.getRepeatedUint32Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedUint32Count());
    assertEquals(123, newMsg.getRepeatedUint32(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedUint32(123)
       .addRepeatedUint32(456);
    assertEquals(2, msg.getRepeatedUint32Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedUint32Count());
    assertEquals(123, newMsg.getRepeatedUint32(0));
    assertEquals(456, newMsg.getRepeatedUint32(1));
  }

  public void testMicroRepeatedUint64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedUint64Count());
    msg.addRepeatedUint64(123);
    assertEquals(1, msg.getRepeatedUint64Count());
    assertEquals(123, msg.getRepeatedUint64(0));
    msg.addRepeatedUint64(456);
    assertEquals(2, msg.getRepeatedUint64Count());
    assertEquals(123, msg.getRepeatedUint64(0));
    assertEquals(456, msg.getRepeatedUint64(1));
    msg.setRepeatedUint64(0, 789);
    assertEquals(2, msg.getRepeatedUint64Count());
    assertEquals(789, msg.getRepeatedUint64(0));
    assertEquals(456, msg.getRepeatedUint64(1));
    msg.clearRepeatedUint64();
    assertEquals(0, msg.getRepeatedUint64Count());
    msg.clearRepeatedUint64()
       .addRepeatedUint64(456);
    assertEquals(1, msg.getRepeatedUint64Count());
    assertEquals(456, msg.getRepeatedUint64(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedUint64Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedUint64(123);
    assertEquals(1, msg.getRepeatedUint64Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedUint64Count());
    assertEquals(123, newMsg.getRepeatedUint64(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedUint64(123)
       .addRepeatedUint64(456);
    assertEquals(2, msg.getRepeatedUint64Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedUint64Count());
    assertEquals(123, newMsg.getRepeatedUint64(0));
    assertEquals(456, newMsg.getRepeatedUint64(1));
  }

  public void testMicroRepeatedSint32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedSint32Count());
    msg.addRepeatedSint32(123);
    assertEquals(1, msg.getRepeatedSint32Count());
    assertEquals(123, msg.getRepeatedSint32(0));
    msg.addRepeatedSint32(456);
    assertEquals(2, msg.getRepeatedSint32Count());
    assertEquals(123, msg.getRepeatedSint32(0));
    assertEquals(456, msg.getRepeatedSint32(1));
    msg.setRepeatedSint32(0, 789);
    assertEquals(2, msg.getRepeatedSint32Count());
    assertEquals(789, msg.getRepeatedSint32(0));
    assertEquals(456, msg.getRepeatedSint32(1));
    msg.clearRepeatedSint32();
    assertEquals(0, msg.getRepeatedSint32Count());
    msg.clearRepeatedSint32()
       .addRepeatedSint32(456);
    assertEquals(1, msg.getRepeatedSint32Count());
    assertEquals(456, msg.getRepeatedSint32(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedSint32Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedSint32(123);
    assertEquals(1, msg.getRepeatedSint32Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 4);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedSint32Count());
    assertEquals(123, newMsg.getRepeatedSint32(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedSint32(123)
       .addRepeatedSint32(456);
    assertEquals(2, msg.getRepeatedSint32Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedSint32Count());
    assertEquals(123, newMsg.getRepeatedSint32(0));
    assertEquals(456, newMsg.getRepeatedSint32(1));
  }

  public void testMicroRepeatedSint64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedSint64Count());
    msg.addRepeatedSint64(123);
    assertEquals(1, msg.getRepeatedSint64Count());
    assertEquals(123, msg.getRepeatedSint64(0));
    msg.addRepeatedSint64(456);
    assertEquals(2, msg.getRepeatedSint64Count());
    assertEquals(123, msg.getRepeatedSint64(0));
    assertEquals(456, msg.getRepeatedSint64(1));
    msg.setRepeatedSint64(0, 789);
    assertEquals(2, msg.getRepeatedSint64Count());
    assertEquals(789, msg.getRepeatedSint64(0));
    assertEquals(456, msg.getRepeatedSint64(1));
    msg.clearRepeatedSint64();
    assertEquals(0, msg.getRepeatedSint64Count());
    msg.clearRepeatedSint64()
       .addRepeatedSint64(456);
    assertEquals(1, msg.getRepeatedSint64Count());
    assertEquals(456, msg.getRepeatedSint64(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedSint64Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedSint64(123);
    assertEquals(1, msg.getRepeatedSint64Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 4);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedSint64Count());
    assertEquals(123, newMsg.getRepeatedSint64(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedSint64(123)
       .addRepeatedSint64(456);
    assertEquals(2, msg.getRepeatedSint64Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedSint64Count());
    assertEquals(123, newMsg.getRepeatedSint64(0));
    assertEquals(456, newMsg.getRepeatedSint64(1));
  }

  public void testMicroRepeatedFixed32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedFixed32Count());
    msg.addRepeatedFixed32(123);
    assertEquals(1, msg.getRepeatedFixed32Count());
    assertEquals(123, msg.getRepeatedFixed32(0));
    msg.addRepeatedFixed32(456);
    assertEquals(2, msg.getRepeatedFixed32Count());
    assertEquals(123, msg.getRepeatedFixed32(0));
    assertEquals(456, msg.getRepeatedFixed32(1));
    msg.setRepeatedFixed32(0, 789);
    assertEquals(2, msg.getRepeatedFixed32Count());
    assertEquals(789, msg.getRepeatedFixed32(0));
    assertEquals(456, msg.getRepeatedFixed32(1));
    msg.clearRepeatedFixed32();
    assertEquals(0, msg.getRepeatedFixed32Count());
    msg.clearRepeatedFixed32()
       .addRepeatedFixed32(456);
    assertEquals(1, msg.getRepeatedFixed32Count());
    assertEquals(456, msg.getRepeatedFixed32(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedFixed32Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedFixed32(123);
    assertEquals(1, msg.getRepeatedFixed32Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedFixed32Count());
    assertEquals(123, newMsg.getRepeatedFixed32(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedFixed32(123)
       .addRepeatedFixed32(456);
    assertEquals(2, msg.getRepeatedFixed32Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 12);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedFixed32Count());
    assertEquals(123, newMsg.getRepeatedFixed32(0));
    assertEquals(456, newMsg.getRepeatedFixed32(1));
  }

  public void testMicroRepeatedFixed64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedFixed64Count());
    msg.addRepeatedFixed64(123);
    assertEquals(1, msg.getRepeatedFixed64Count());
    assertEquals(123, msg.getRepeatedFixed64(0));
    msg.addRepeatedFixed64(456);
    assertEquals(2, msg.getRepeatedFixed64Count());
    assertEquals(123, msg.getRepeatedFixed64(0));
    assertEquals(456, msg.getRepeatedFixed64(1));
    msg.setRepeatedFixed64(0, 789);
    assertEquals(2, msg.getRepeatedFixed64Count());
    assertEquals(789, msg.getRepeatedFixed64(0));
    assertEquals(456, msg.getRepeatedFixed64(1));
    msg.clearRepeatedFixed64();
    assertEquals(0, msg.getRepeatedFixed64Count());
    msg.clearRepeatedFixed64()
       .addRepeatedFixed64(456);
    assertEquals(1, msg.getRepeatedFixed64Count());
    assertEquals(456, msg.getRepeatedFixed64(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedFixed64Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedFixed64(123);
    assertEquals(1, msg.getRepeatedFixed64Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedFixed64Count());
    assertEquals(123, newMsg.getRepeatedFixed64(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedFixed64(123)
       .addRepeatedFixed64(456);
    assertEquals(2, msg.getRepeatedFixed64Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 20);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedFixed64Count());
    assertEquals(123, newMsg.getRepeatedFixed64(0));
    assertEquals(456, newMsg.getRepeatedFixed64(1));
  }

  public void testMicroRepeatedSfixed32() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedSfixed32Count());
    msg.addRepeatedSfixed32(123);
    assertEquals(1, msg.getRepeatedSfixed32Count());
    assertEquals(123, msg.getRepeatedSfixed32(0));
    msg.addRepeatedSfixed32(456);
    assertEquals(2, msg.getRepeatedSfixed32Count());
    assertEquals(123, msg.getRepeatedSfixed32(0));
    assertEquals(456, msg.getRepeatedSfixed32(1));
    msg.setRepeatedSfixed32(0, 789);
    assertEquals(2, msg.getRepeatedSfixed32Count());
    assertEquals(789, msg.getRepeatedSfixed32(0));
    assertEquals(456, msg.getRepeatedSfixed32(1));
    msg.clearRepeatedSfixed32();
    assertEquals(0, msg.getRepeatedSfixed32Count());
    msg.clearRepeatedSfixed32()
       .addRepeatedSfixed32(456);
    assertEquals(1, msg.getRepeatedSfixed32Count());
    assertEquals(456, msg.getRepeatedSfixed32(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedSfixed32Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedSfixed32(123);
    assertEquals(1, msg.getRepeatedSfixed32Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedSfixed32Count());
    assertEquals(123, newMsg.getRepeatedSfixed32(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedSfixed32(123)
       .addRepeatedSfixed32(456);
    assertEquals(2, msg.getRepeatedSfixed32Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 12);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedSfixed32Count());
    assertEquals(123, newMsg.getRepeatedSfixed32(0));
    assertEquals(456, newMsg.getRepeatedSfixed32(1));
  }

  public void testMicroRepeatedSfixed64() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedSfixed64Count());
    msg.addRepeatedSfixed64(123);
    assertEquals(1, msg.getRepeatedSfixed64Count());
    assertEquals(123, msg.getRepeatedSfixed64(0));
    msg.addRepeatedSfixed64(456);
    assertEquals(2, msg.getRepeatedSfixed64Count());
    assertEquals(123, msg.getRepeatedSfixed64(0));
    assertEquals(456, msg.getRepeatedSfixed64(1));
    msg.setRepeatedSfixed64(0, 789);
    assertEquals(2, msg.getRepeatedSfixed64Count());
    assertEquals(789, msg.getRepeatedSfixed64(0));
    assertEquals(456, msg.getRepeatedSfixed64(1));
    msg.clearRepeatedSfixed64();
    assertEquals(0, msg.getRepeatedSfixed64Count());
    msg.clearRepeatedSfixed64()
       .addRepeatedSfixed64(456);
    assertEquals(1, msg.getRepeatedSfixed64Count());
    assertEquals(456, msg.getRepeatedSfixed64(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedSfixed64Count());

    // Test 1 entry
    msg.clear()
       .addRepeatedSfixed64(123);
    assertEquals(1, msg.getRepeatedSfixed64Count());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedSfixed64Count());
    assertEquals(123, newMsg.getRepeatedSfixed64(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedSfixed64(123)
       .addRepeatedSfixed64(456);
    assertEquals(2, msg.getRepeatedSfixed64Count());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 20);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedSfixed64Count());
    assertEquals(123, newMsg.getRepeatedSfixed64(0));
    assertEquals(456, newMsg.getRepeatedSfixed64(1));
  }

  public void testMicroRepeatedFloat() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedFloatCount());
    msg.addRepeatedFloat(123f);
    assertEquals(1, msg.getRepeatedFloatCount());
    assertTrue(123f == msg.getRepeatedFloat(0));
    msg.addRepeatedFloat(456f);
    assertEquals(2, msg.getRepeatedFloatCount());
    assertTrue(123f == msg.getRepeatedFloat(0));
    assertTrue(456f == msg.getRepeatedFloat(1));
    msg.setRepeatedFloat(0, 789f);
    assertEquals(2, msg.getRepeatedFloatCount());
    assertTrue(789f == msg.getRepeatedFloat(0));
    assertTrue(456f == msg.getRepeatedFloat(1));
    msg.clearRepeatedFloat();
    assertEquals(0, msg.getRepeatedFloatCount());
    msg.clearRepeatedFloat()
       .addRepeatedFloat(456f);
    assertEquals(1, msg.getRepeatedFloatCount());
    assertTrue(456f == msg.getRepeatedFloat(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedFloatCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedFloat(123f);
    assertEquals(1, msg.getRepeatedFloatCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedFloatCount());
    assertTrue(123f == newMsg.getRepeatedFloat(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedFloat(123f)
       .addRepeatedFloat(456f);
    assertEquals(2, msg.getRepeatedFloatCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 12);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedFloatCount());
    assertTrue(123f == newMsg.getRepeatedFloat(0));
    assertTrue(456f == newMsg.getRepeatedFloat(1));
  }

  public void testMicroRepeatedDouble() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedDoubleCount());
    msg.addRepeatedDouble(123.0);
    assertEquals(1, msg.getRepeatedDoubleCount());
    assertTrue(123.0 == msg.getRepeatedDouble(0));
    msg.addRepeatedDouble(456.0);
    assertEquals(2, msg.getRepeatedDoubleCount());
    assertTrue(123.0 == msg.getRepeatedDouble(0));
    assertTrue(456.0 == msg.getRepeatedDouble(1));
    msg.setRepeatedDouble(0, 789.0);
    assertEquals(2, msg.getRepeatedDoubleCount());
    assertTrue(789.0 == msg.getRepeatedDouble(0));
    assertTrue(456.0 == msg.getRepeatedDouble(1));
    msg.clearRepeatedDouble();
    assertEquals(0, msg.getRepeatedDoubleCount());
    msg.clearRepeatedDouble()
       .addRepeatedDouble(456.0);
    assertEquals(1, msg.getRepeatedDoubleCount());
    assertTrue(456.0 == msg.getRepeatedDouble(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedDoubleCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedDouble(123.0);
    assertEquals(1, msg.getRepeatedDoubleCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedDoubleCount());
    assertTrue(123.0 == newMsg.getRepeatedDouble(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedDouble(123.0)
       .addRepeatedDouble(456.0);
    assertEquals(2, msg.getRepeatedDoubleCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 20);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedDoubleCount());
    assertTrue(123.0 == newMsg.getRepeatedDouble(0));
    assertTrue(456.0 == newMsg.getRepeatedDouble(1));
  }

  public void testMicroRepeatedBool() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedBoolCount());
    msg.addRepeatedBool(true);
    assertEquals(1, msg.getRepeatedBoolCount());
    assertEquals(true, msg.getRepeatedBool(0));
    msg.addRepeatedBool(false);
    assertEquals(2, msg.getRepeatedBoolCount());
    assertEquals(true, msg.getRepeatedBool(0));
    assertEquals(false, msg.getRepeatedBool(1));
    msg.setRepeatedBool(0, false);
    assertEquals(2, msg.getRepeatedBoolCount());
    assertEquals(false, msg.getRepeatedBool(0));
    assertEquals(false, msg.getRepeatedBool(1));
    msg.clearRepeatedBool();
    assertEquals(0, msg.getRepeatedBoolCount());
    msg.clearRepeatedBool()
       .addRepeatedBool(true);
    assertEquals(1, msg.getRepeatedBoolCount());
    assertEquals(true, msg.getRepeatedBool(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedBoolCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedBool(false);
    assertEquals(1, msg.getRepeatedBoolCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedBoolCount());
    assertEquals(false, newMsg.getRepeatedBool(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedBool(true)
       .addRepeatedBool(false);
    assertEquals(2, msg.getRepeatedBoolCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedBoolCount());
    assertEquals(true, newMsg.getRepeatedBool(0));
    assertEquals(false, newMsg.getRepeatedBool(1));
  }

  public void testMicroRepeatedString() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedStringCount());
    msg.addRepeatedString("hello");
    assertEquals(1, msg.getRepeatedStringCount());
    assertEquals("hello", msg.getRepeatedString(0));
    msg.addRepeatedString("bye");
    assertEquals(2, msg.getRepeatedStringCount());
    assertEquals("hello", msg.getRepeatedString(0));
    assertEquals("bye", msg.getRepeatedString(1));
    msg.setRepeatedString(0, "boo");
    assertEquals(2, msg.getRepeatedStringCount());
    assertEquals("boo", msg.getRepeatedString(0));
    assertEquals("bye", msg.getRepeatedString(1));
    msg.clearRepeatedString();
    assertEquals(0, msg.getRepeatedStringCount());
    msg.clearRepeatedString()
       .addRepeatedString("hello");
    assertEquals(1, msg.getRepeatedStringCount());
    assertEquals("hello", msg.getRepeatedString(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedStringCount());

    // Test 1 entry and an empty string
    msg.clear()
       .addRepeatedString("");
    assertEquals(1, msg.getRepeatedStringCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedStringCount());
    assertEquals("", newMsg.getRepeatedString(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedString("hello")
       .addRepeatedString("world");
    assertEquals(2, msg.getRepeatedStringCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 16);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedStringCount());
    assertEquals("hello", newMsg.getRepeatedString(0));
    assertEquals("world", newMsg.getRepeatedString(1));
  }

  public void testMicroRepeatedBytes() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedBytesCount());
    msg.addRepeatedBytes(ByteStringMicro.copyFromUtf8("hello"));
    assertEquals(1, msg.getRepeatedBytesCount());
    assertEquals("hello", msg.getRepeatedBytes(0).toStringUtf8());
    msg.addRepeatedBytes(ByteStringMicro.copyFromUtf8("bye"));
    assertEquals(2, msg.getRepeatedBytesCount());
    assertEquals("hello", msg.getRepeatedBytes(0).toStringUtf8());
    assertEquals("bye", msg.getRepeatedBytes(1).toStringUtf8());
    msg.setRepeatedBytes(0, ByteStringMicro.copyFromUtf8("boo"));
    assertEquals(2, msg.getRepeatedBytesCount());
    assertEquals("boo", msg.getRepeatedBytes(0).toStringUtf8());
    assertEquals("bye", msg.getRepeatedBytes(1).toStringUtf8());
    msg.clearRepeatedBytes();
    assertEquals(0, msg.getRepeatedBytesCount());
    msg.clearRepeatedBytes()
       .addRepeatedBytes(ByteStringMicro.copyFromUtf8("hello"));
    assertEquals(1, msg.getRepeatedBytesCount());
    assertEquals("hello", msg.getRepeatedBytes(0).toStringUtf8());
    msg.clear();
    assertEquals(0, msg.getRepeatedBytesCount());

    // Test 1 entry and an empty byte array can be serialized
    msg.clear()
       .addRepeatedBytes(ByteStringMicro.copyFromUtf8(""));
    assertEquals(1, msg.getRepeatedBytesCount());
    assertEquals("", msg.getRepeatedBytes(0).toStringUtf8());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedBytesCount());
    assertEquals("", newMsg.getRepeatedBytes(0).toStringUtf8());

    // Test 2 entries
    msg.clear()
       .addRepeatedBytes(ByteStringMicro.copyFromUtf8("hello"))
       .addRepeatedBytes(ByteStringMicro.copyFromUtf8("world"));
    assertEquals(2, msg.getRepeatedBytesCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 16);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedBytesCount());
    assertEquals("hello", newMsg.getRepeatedBytes(0).toStringUtf8());
    assertEquals("world", newMsg.getRepeatedBytes(1).toStringUtf8());
  }

  public void testMicroRepeatedGroup() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    TestAllTypesMicro.RepeatedGroup group0 =
      new TestAllTypesMicro.RepeatedGroup().setA(0);
    TestAllTypesMicro.RepeatedGroup group1 =
      new TestAllTypesMicro.RepeatedGroup().setA(1);
    TestAllTypesMicro.RepeatedGroup group2 =
      new TestAllTypesMicro.RepeatedGroup().setA(2);

    msg.addRepeatedGroup(group0);
    assertEquals(1, msg.getRepeatedGroupCount());
    assertEquals(0, msg.getRepeatedGroup(0).getA());
    msg.addRepeatedGroup(group1);
    assertEquals(2, msg.getRepeatedGroupCount());
    assertEquals(0, msg.getRepeatedGroup(0).getA());
    assertEquals(1, msg.getRepeatedGroup(1).getA());
    msg.setRepeatedGroup(0, group2);
    assertEquals(2, msg.getRepeatedGroupCount());
    assertEquals(2, msg.getRepeatedGroup(0).getA());
    assertEquals(1, msg.getRepeatedGroup(1).getA());
    msg.clearRepeatedGroup();
    assertEquals(0, msg.getRepeatedGroupCount());
    msg.clearRepeatedGroup()
       .addRepeatedGroup(group1);
    assertEquals(1, msg.getRepeatedGroupCount());
    assertEquals(1, msg.getRepeatedGroup(0).getA());
    msg.clear();
    assertEquals(0, msg.getRepeatedGroupCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedGroup(group0);
    assertEquals(1, msg.getRepeatedGroupCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedGroupCount());
    assertEquals(0, newMsg.getRepeatedGroup(0).getA());

    // Test 2 entries
    msg.clear()
       .addRepeatedGroup(group0)
       .addRepeatedGroup(group1);
    assertEquals(2, msg.getRepeatedGroupCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 14);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedGroupCount());
    assertEquals(0, newMsg.getRepeatedGroup(0).getA());
    assertEquals(1, newMsg.getRepeatedGroup(1).getA());
  }


  public void testMicroRepeatedNestedMessage() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    TestAllTypesMicro.NestedMessage nestedMsg0 =
      new TestAllTypesMicro.NestedMessage().setBb(0);
    TestAllTypesMicro.NestedMessage nestedMsg1 =
      new TestAllTypesMicro.NestedMessage().setBb(1);
    TestAllTypesMicro.NestedMessage nestedMsg2 =
      new TestAllTypesMicro.NestedMessage().setBb(2);

    msg.addRepeatedNestedMessage(nestedMsg0);
    assertEquals(1, msg.getRepeatedNestedMessageCount());
    assertEquals(0, msg.getRepeatedNestedMessage(0).getBb());
    msg.addRepeatedNestedMessage(nestedMsg1);
    assertEquals(2, msg.getRepeatedNestedMessageCount());
    assertEquals(0, msg.getRepeatedNestedMessage(0).getBb());
    assertEquals(1, msg.getRepeatedNestedMessage(1).getBb());
    msg.setRepeatedNestedMessage(0, nestedMsg2);
    assertEquals(2, msg.getRepeatedNestedMessageCount());
    assertEquals(2, msg.getRepeatedNestedMessage(0).getBb());
    assertEquals(1, msg.getRepeatedNestedMessage(1).getBb());
    msg.clearRepeatedNestedMessage();
    assertEquals(0, msg.getRepeatedNestedMessageCount());
    msg.clearRepeatedNestedMessage()
       .addRepeatedNestedMessage(nestedMsg1);
    assertEquals(1, msg.getRepeatedNestedMessageCount());
    assertEquals(1, msg.getRepeatedNestedMessage(0).getBb());
    msg.clear();
    assertEquals(0, msg.getRepeatedNestedMessageCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedNestedMessage(nestedMsg0);
    assertEquals(1, msg.getRepeatedNestedMessageCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedNestedMessageCount());
    assertEquals(0, newMsg.getRepeatedNestedMessage(0).getBb());

    // Test 2 entries
    msg.clear()
       .addRepeatedNestedMessage(nestedMsg0)
       .addRepeatedNestedMessage(nestedMsg1);
    assertEquals(2, msg.getRepeatedNestedMessageCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedNestedMessageCount());
    assertEquals(0, newMsg.getRepeatedNestedMessage(0).getBb());
    assertEquals(1, newMsg.getRepeatedNestedMessage(1).getBb());
  }

  public void testMicroRepeatedForeignMessage() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    MicroOuterClass.ForeignMessageMicro foreignMsg0 =
      new MicroOuterClass.ForeignMessageMicro().setC(0);
    MicroOuterClass.ForeignMessageMicro foreignMsg1 =
      new MicroOuterClass.ForeignMessageMicro().setC(1);
    MicroOuterClass.ForeignMessageMicro foreignMsg2 =
      new MicroOuterClass.ForeignMessageMicro().setC(2);

    msg.addRepeatedForeignMessage(foreignMsg0);
    assertEquals(1, msg.getRepeatedForeignMessageCount());
    assertEquals(0, msg.getRepeatedForeignMessage(0).getC());
    msg.addRepeatedForeignMessage(foreignMsg1);
    assertEquals(2, msg.getRepeatedForeignMessageCount());
    assertEquals(0, msg.getRepeatedForeignMessage(0).getC());
    assertEquals(1, msg.getRepeatedForeignMessage(1).getC());
    msg.setRepeatedForeignMessage(0, foreignMsg2);
    assertEquals(2, msg.getRepeatedForeignMessageCount());
    assertEquals(2, msg.getRepeatedForeignMessage(0).getC());
    assertEquals(1, msg.getRepeatedForeignMessage(1).getC());
    msg.clearRepeatedForeignMessage();
    assertEquals(0, msg.getRepeatedForeignMessageCount());
    msg.clearRepeatedForeignMessage()
       .addRepeatedForeignMessage(foreignMsg1);
    assertEquals(1, msg.getRepeatedForeignMessageCount());
    assertEquals(1, msg.getRepeatedForeignMessage(0).getC());
    msg.clear();
    assertEquals(0, msg.getRepeatedForeignMessageCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedForeignMessage(foreignMsg0);
    assertEquals(1, msg.getRepeatedForeignMessageCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedForeignMessageCount());
    assertEquals(0, newMsg.getRepeatedForeignMessage(0).getC());

    // Test 2 entries
    msg.clear()
       .addRepeatedForeignMessage(foreignMsg0)
       .addRepeatedForeignMessage(foreignMsg1);
    assertEquals(2, msg.getRepeatedForeignMessageCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedForeignMessageCount());
    assertEquals(0, newMsg.getRepeatedForeignMessage(0).getC());
    assertEquals(1, newMsg.getRepeatedForeignMessage(1).getC());
  }

  public void testMicroRepeatedImportMessage() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    UnittestImportMicro.ImportMessageMicro importMsg0 =
      new UnittestImportMicro.ImportMessageMicro().setD(0);
    UnittestImportMicro.ImportMessageMicro importMsg1 =
      new UnittestImportMicro.ImportMessageMicro().setD(1);
    UnittestImportMicro.ImportMessageMicro importMsg2 =
      new UnittestImportMicro.ImportMessageMicro().setD(2);

    msg.addRepeatedImportMessage(importMsg0);
    assertEquals(1, msg.getRepeatedImportMessageCount());
    assertEquals(0, msg.getRepeatedImportMessage(0).getD());
    msg.addRepeatedImportMessage(importMsg1);
    assertEquals(2, msg.getRepeatedImportMessageCount());
    assertEquals(0, msg.getRepeatedImportMessage(0).getD());
    assertEquals(1, msg.getRepeatedImportMessage(1).getD());
    msg.setRepeatedImportMessage(0, importMsg2);
    assertEquals(2, msg.getRepeatedImportMessageCount());
    assertEquals(2, msg.getRepeatedImportMessage(0).getD());
    assertEquals(1, msg.getRepeatedImportMessage(1).getD());
    msg.clearRepeatedImportMessage();
    assertEquals(0, msg.getRepeatedImportMessageCount());
    msg.clearRepeatedImportMessage()
       .addRepeatedImportMessage(importMsg1);
    assertEquals(1, msg.getRepeatedImportMessageCount());
    assertEquals(1, msg.getRepeatedImportMessage(0).getD());
    msg.clear();
    assertEquals(0, msg.getRepeatedImportMessageCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedImportMessage(importMsg0);
    assertEquals(1, msg.getRepeatedImportMessageCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedImportMessageCount());
    assertEquals(0, newMsg.getRepeatedImportMessage(0).getD());

    // Test 2 entries
    msg.clear()
       .addRepeatedImportMessage(importMsg0)
       .addRepeatedImportMessage(importMsg1);
    assertEquals(2, msg.getRepeatedImportMessageCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedImportMessageCount());
    assertEquals(0, newMsg.getRepeatedImportMessage(0).getD());
    assertEquals(1, newMsg.getRepeatedImportMessage(1).getD());
  }

  public void testMicroRepeatedNestedEnum() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    msg.addRepeatedNestedEnum(TestAllTypesMicro.FOO);
    assertEquals(1, msg.getRepeatedNestedEnumCount());
    assertEquals(TestAllTypesMicro.FOO, msg.getRepeatedNestedEnum(0));
    msg.addRepeatedNestedEnum(TestAllTypesMicro.BAR);
    assertEquals(2, msg.getRepeatedNestedEnumCount());
    assertEquals(TestAllTypesMicro.FOO, msg.getRepeatedNestedEnum(0));
    assertEquals(TestAllTypesMicro.BAR, msg.getRepeatedNestedEnum(1));
    msg.setRepeatedNestedEnum(0, TestAllTypesMicro.BAZ);
    assertEquals(2, msg.getRepeatedNestedEnumCount());
    assertEquals(TestAllTypesMicro.BAZ, msg.getRepeatedNestedEnum(0));
    assertEquals(TestAllTypesMicro.BAR, msg.getRepeatedNestedEnum(1));
    msg.clearRepeatedNestedEnum();
    assertEquals(0, msg.getRepeatedNestedEnumCount());
    msg.clearRepeatedNestedEnum()
       .addRepeatedNestedEnum(TestAllTypesMicro.BAR);
    assertEquals(1, msg.getRepeatedNestedEnumCount());
    assertEquals(TestAllTypesMicro.BAR, msg.getRepeatedNestedEnum(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedNestedEnumCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedNestedEnum(TestAllTypesMicro.FOO);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedNestedEnumCount());
    assertEquals(TestAllTypesMicro.FOO, msg.getRepeatedNestedEnum(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedNestedEnum(TestAllTypesMicro.FOO)
       .addRepeatedNestedEnum(TestAllTypesMicro.BAR);
    assertEquals(2, msg.getRepeatedNestedEnumCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedNestedEnumCount());
    assertEquals(TestAllTypesMicro.FOO, msg.getRepeatedNestedEnum(0));
    assertEquals(TestAllTypesMicro.BAR, msg.getRepeatedNestedEnum(1));
  }

  public void testMicroRepeatedForeignEnum() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    msg.addRepeatedForeignEnum(MicroOuterClass.FOREIGN_MICRO_FOO);
    assertEquals(1, msg.getRepeatedForeignEnumCount());
    assertEquals(MicroOuterClass.FOREIGN_MICRO_FOO, msg.getRepeatedForeignEnum(0));
    msg.addRepeatedForeignEnum(MicroOuterClass.FOREIGN_MICRO_BAR);
    assertEquals(2, msg.getRepeatedForeignEnumCount());
    assertEquals(MicroOuterClass.FOREIGN_MICRO_FOO, msg.getRepeatedForeignEnum(0));
    assertEquals(MicroOuterClass.FOREIGN_MICRO_BAR, msg.getRepeatedForeignEnum(1));
    msg.setRepeatedForeignEnum(0, MicroOuterClass.FOREIGN_MICRO_BAZ);
    assertEquals(2, msg.getRepeatedForeignEnumCount());
    assertEquals(MicroOuterClass.FOREIGN_MICRO_BAZ, msg.getRepeatedForeignEnum(0));
    assertEquals(MicroOuterClass.FOREIGN_MICRO_BAR, msg.getRepeatedForeignEnum(1));
    msg.clearRepeatedForeignEnum();
    assertEquals(0, msg.getRepeatedForeignEnumCount());
    msg.clearRepeatedForeignEnum()
       .addRepeatedForeignEnum(MicroOuterClass.FOREIGN_MICRO_BAR);
    assertEquals(1, msg.getRepeatedForeignEnumCount());
    assertEquals(MicroOuterClass.FOREIGN_MICRO_BAR, msg.getRepeatedForeignEnum(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedForeignEnumCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedForeignEnum(MicroOuterClass.FOREIGN_MICRO_FOO);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedForeignEnumCount());
    assertEquals(MicroOuterClass.FOREIGN_MICRO_FOO, msg.getRepeatedForeignEnum(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedForeignEnum(MicroOuterClass.FOREIGN_MICRO_FOO)
       .addRepeatedForeignEnum(MicroOuterClass.FOREIGN_MICRO_BAR);
    assertEquals(2, msg.getRepeatedForeignEnumCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedForeignEnumCount());
    assertEquals(MicroOuterClass.FOREIGN_MICRO_FOO, msg.getRepeatedForeignEnum(0));
    assertEquals(MicroOuterClass.FOREIGN_MICRO_BAR, msg.getRepeatedForeignEnum(1));
  }

  public void testMicroRepeatedImportEnum() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    msg.addRepeatedImportEnum(UnittestImportMicro.IMPORT_MICRO_FOO);
    assertEquals(1, msg.getRepeatedImportEnumCount());
    assertEquals(UnittestImportMicro.IMPORT_MICRO_FOO, msg.getRepeatedImportEnum(0));
    msg.addRepeatedImportEnum(UnittestImportMicro.IMPORT_MICRO_BAR);
    assertEquals(2, msg.getRepeatedImportEnumCount());
    assertEquals(UnittestImportMicro.IMPORT_MICRO_FOO, msg.getRepeatedImportEnum(0));
    assertEquals(UnittestImportMicro.IMPORT_MICRO_BAR, msg.getRepeatedImportEnum(1));
    msg.setRepeatedImportEnum(0, UnittestImportMicro.IMPORT_MICRO_BAZ);
    assertEquals(2, msg.getRepeatedImportEnumCount());
    assertEquals(UnittestImportMicro.IMPORT_MICRO_BAZ, msg.getRepeatedImportEnum(0));
    assertEquals(UnittestImportMicro.IMPORT_MICRO_BAR, msg.getRepeatedImportEnum(1));
    msg.clearRepeatedImportEnum();
    assertEquals(0, msg.getRepeatedImportEnumCount());
    msg.clearRepeatedImportEnum()
       .addRepeatedImportEnum(UnittestImportMicro.IMPORT_MICRO_BAR);
    assertEquals(1, msg.getRepeatedImportEnumCount());
    assertEquals(UnittestImportMicro.IMPORT_MICRO_BAR, msg.getRepeatedImportEnum(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedImportEnumCount());

    // Test 1 entry
    msg.clear()
       .addRepeatedImportEnum(UnittestImportMicro.IMPORT_MICRO_FOO);
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedImportEnumCount());
    assertEquals(UnittestImportMicro.IMPORT_MICRO_FOO, msg.getRepeatedImportEnum(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedImportEnum(UnittestImportMicro.IMPORT_MICRO_FOO)
       .addRepeatedImportEnum(UnittestImportMicro.IMPORT_MICRO_BAR);
    assertEquals(2, msg.getRepeatedImportEnumCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedImportEnumCount());
    assertEquals(UnittestImportMicro.IMPORT_MICRO_FOO, msg.getRepeatedImportEnum(0));
    assertEquals(UnittestImportMicro.IMPORT_MICRO_BAR, msg.getRepeatedImportEnum(1));
  }

  public void testMicroRepeatedStringPiece() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedStringPieceCount());
    msg.addRepeatedStringPiece("hello");
    assertEquals(1, msg.getRepeatedStringPieceCount());
    assertEquals("hello", msg.getRepeatedStringPiece(0));
    msg.addRepeatedStringPiece("bye");
    assertEquals(2, msg.getRepeatedStringPieceCount());
    assertEquals("hello", msg.getRepeatedStringPiece(0));
    assertEquals("bye", msg.getRepeatedStringPiece(1));
    msg.setRepeatedStringPiece(0, "boo");
    assertEquals(2, msg.getRepeatedStringPieceCount());
    assertEquals("boo", msg.getRepeatedStringPiece(0));
    assertEquals("bye", msg.getRepeatedStringPiece(1));
    msg.clearRepeatedStringPiece();
    assertEquals(0, msg.getRepeatedStringPieceCount());
    msg.clearRepeatedStringPiece()
       .addRepeatedStringPiece("hello");
    assertEquals(1, msg.getRepeatedStringPieceCount());
    assertEquals("hello", msg.getRepeatedStringPiece(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedStringPieceCount());

    // Test 1 entry and an empty string
    msg.clear()
       .addRepeatedStringPiece("");
    assertEquals(1, msg.getRepeatedStringPieceCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedStringPieceCount());
    assertEquals("", newMsg.getRepeatedStringPiece(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedStringPiece("hello")
       .addRepeatedStringPiece("world");
    assertEquals(2, msg.getRepeatedStringPieceCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 16);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedStringPieceCount());
    assertEquals("hello", newMsg.getRepeatedStringPiece(0));
    assertEquals("world", newMsg.getRepeatedStringPiece(1));
  }

  public void testMicroRepeatedCord() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertEquals(0, msg.getRepeatedCordCount());
    msg.addRepeatedCord("hello");
    assertEquals(1, msg.getRepeatedCordCount());
    assertEquals("hello", msg.getRepeatedCord(0));
    msg.addRepeatedCord("bye");
    assertEquals(2, msg.getRepeatedCordCount());
    assertEquals("hello", msg.getRepeatedCord(0));
    assertEquals("bye", msg.getRepeatedCord(1));
    msg.setRepeatedCord(0, "boo");
    assertEquals(2, msg.getRepeatedCordCount());
    assertEquals("boo", msg.getRepeatedCord(0));
    assertEquals("bye", msg.getRepeatedCord(1));
    msg.clearRepeatedCord();
    assertEquals(0, msg.getRepeatedCordCount());
    msg.clearRepeatedCord()
       .addRepeatedCord("hello");
    assertEquals(1, msg.getRepeatedCordCount());
    assertEquals("hello", msg.getRepeatedCord(0));
    msg.clear();
    assertEquals(0, msg.getRepeatedCordCount());

    // Test 1 entry and an empty string
    msg.clear()
       .addRepeatedCord("");
    assertEquals(1, msg.getRepeatedCordCount());
    byte [] result = msg.toByteArray();
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesMicro newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(1, newMsg.getRepeatedCordCount());
    assertEquals("", newMsg.getRepeatedCord(0));

    // Test 2 entries
    msg.clear()
       .addRepeatedCord("hello")
       .addRepeatedCord("world");
    assertEquals(2, msg.getRepeatedCordCount());
    result = msg.toByteArray();
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 16);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesMicro.parseFrom(result);
    assertEquals(2, newMsg.getRepeatedCordCount());
    assertEquals("hello", newMsg.getRepeatedCord(0));
    assertEquals("world", newMsg.getRepeatedCord(1));
  }

  /**
   * Tests that code generation correctly wraps a single message into its outer
   * class. The class {@code SingleMessageMicro} is imported from the outer
   * class {@code UnittestSingleMicro}, whose name is implicit. Any error would
   * cause this method to fail compilation.
   */
  public void testMicroSingle() throws Exception {
    SingleMessageMicro msg = new SingleMessageMicro();
  }

  /**
   * Tests that code generation correctly skips generating the outer class if
   * unnecessary, letting a file-scope entity have the same name. The class
   * {@code MultipleNameClashMicro} shares the same name with the file's outer
   * class defined explicitly, but the file contains no other entities and has
   * java_multiple_files set. Any error would cause this method to fail
   * compilation.
   */
  public void testMicroMultipleNameClash() throws Exception {
    MultipleNameClashMicro msg = new MultipleNameClashMicro();
    msg.setField(0);
  }

  /**
   * Tests that code generation correctly handles enums in different scopes in
   * a source file with the option java_multiple_files set to true. Any error
   * would cause this method to fail compilation.
   */
  public void testMicroMultipleEnumScoping() throws Exception {
    FileScopeEnumRefMicro msg1 = new FileScopeEnumRefMicro();
    msg1.setEnumField(UnittestMultipleMicro.ONE);
    MessageScopeEnumRefMicro msg2 = new MessageScopeEnumRefMicro();
    msg2.setEnumField(MessageScopeEnumRefMicro.TWO);
  }

  /**
   * Tests that code generation with mixed values of the java_multiple_files
   * options between the main source file and the imported source files would
   * generate correct references. Any error would cause this method to fail
   * compilation.
   */
  public void testMicroMultipleImportingNonMultiple() throws Exception {
    UnittestImportMicro.ImportMessageMicro importMsg =
        new UnittestImportMicro.ImportMessageMicro();
    MultipleImportingNonMultipleMicro1 micro1 = new MultipleImportingNonMultipleMicro1();
    micro1.setField(importMsg);
    MultipleImportingNonMultipleMicro2 micro2 = new MultipleImportingNonMultipleMicro2();
    micro2.setMicro1(micro1);
  }

  public void testMicroDefaults() throws Exception {
    TestAllTypesMicro msg = new TestAllTypesMicro();
    assertFalse(msg.hasDefaultInt32());
    assertEquals(41, msg.getDefaultInt32());
    assertFalse(msg.hasDefaultInt64());
    assertEquals(42, msg.getDefaultInt64());
    assertFalse(msg.hasDefaultUint32());
    assertEquals(43, msg.getDefaultUint32());
    assertFalse(msg.hasDefaultUint64());
    assertEquals(44, msg.getDefaultUint64());
    assertFalse(msg.hasDefaultSint32());
    assertEquals(-45, msg.getDefaultSint32());
    assertFalse(msg.hasDefaultSint64());
    assertEquals(46, msg.getDefaultSint64());
    assertFalse(msg.hasDefaultFixed32());
    assertEquals(47, msg.getDefaultFixed32());
    assertFalse(msg.hasDefaultFixed64());
    assertEquals(48, msg.getDefaultFixed64());
    assertFalse(msg.hasDefaultSfixed32());
    assertEquals(49, msg.getDefaultSfixed32());
    assertFalse(msg.hasDefaultSfixed64());
    assertEquals(-50, msg.getDefaultSfixed64());
    assertFalse(msg.hasDefaultFloat());
    assertTrue(51.5f == msg.getDefaultFloat());
    assertFalse(msg.hasDefaultDouble());
    assertTrue(52.0e3 == msg.getDefaultDouble());
    assertFalse(msg.hasDefaultBool());
    assertEquals(true, msg.getDefaultBool());
    assertFalse(msg.hasDefaultString());
    assertEquals("hello", msg.getDefaultString());
    assertFalse(msg.hasDefaultBytes());
    assertEquals("world", msg.getDefaultBytes().toStringUtf8());
    assertFalse(msg.hasDefaultNestedEnum());
    assertEquals(TestAllTypesMicro.BAR, msg.getDefaultNestedEnum());
    assertFalse(msg.hasDefaultForeignEnum());
    assertEquals(MicroOuterClass.FOREIGN_MICRO_BAR, msg.getDefaultForeignEnum());
    assertFalse(msg.hasDefaultImportEnum());
    assertEquals(UnittestImportMicro.IMPORT_MICRO_BAR, msg.getDefaultImportEnum());
    assertFalse(msg.hasDefaultFloatInf());
    assertEquals(Float.POSITIVE_INFINITY, msg.getDefaultFloatInf());
    assertFalse(msg.hasDefaultFloatNegInf());
    assertEquals(Float.NEGATIVE_INFINITY, msg.getDefaultFloatNegInf());
    assertFalse(msg.hasDefaultFloatNan());
    assertEquals(Float.NaN, msg.getDefaultFloatNan());
    assertFalse(msg.hasDefaultDoubleInf());
    assertEquals(Double.POSITIVE_INFINITY, msg.getDefaultDoubleInf());
    assertFalse(msg.hasDefaultDoubleNegInf());
    assertEquals(Double.NEGATIVE_INFINITY, msg.getDefaultDoubleNegInf());
    assertFalse(msg.hasDefaultDoubleNan());
    assertEquals(Double.NaN, msg.getDefaultDoubleNan());
  }

  /**
   * Test that a bug in skipRawBytes() has been fixed:  if the skip skips
   * exactly up to a limit, this should not break things.
   */
  public void testSkipRawBytesBug() throws Exception {
    byte[] rawBytes = new byte[] { 1, 2 };
    CodedInputStreamMicro input = CodedInputStreamMicro.newInstance(rawBytes);

    int limit = input.pushLimit(1);
    input.skipRawBytes(1);
    input.popLimit(limit);
    assertEquals(2, input.readRawByte());
  }

  /**
   * Test that a bug in skipRawBytes() has been fixed:  if the skip skips
   * past the end of a buffer with a limit that has been set past the end of
   * that buffer, this should not break things.
   */
  public void testSkipRawBytesPastEndOfBufferWithLimit() throws Exception {
    byte[] rawBytes = new byte[] { 1, 2, 3, 4, 5 };
    CodedInputStreamMicro input = CodedInputStreamMicro.newInstance(
        new SmallBlockInputStream(rawBytes, 3));

    int limit = input.pushLimit(4);
    // In order to expose the bug we need to read at least one byte to prime the
    // buffer inside the CodedInputStream.
    assertEquals(1, input.readRawByte());
    // Skip to the end of the limit.
    input.skipRawBytes(3);
    assertTrue(input.isAtEnd());
    input.popLimit(limit);
    assertEquals(5, input.readRawByte());
  }

  /**
   * An InputStream which limits the number of bytes it reads at a time.
   * We use this to make sure that CodedInputStream doesn't screw up when
   * reading in small blocks.
   */
  private static final class SmallBlockInputStream extends FilterInputStream {
    private final int blockSize;

    public SmallBlockInputStream(byte[] data, int blockSize) {
      this(new ByteArrayInputStream(data), blockSize);
    }

    public SmallBlockInputStream(InputStream in, int blockSize) {
      super(in);
      this.blockSize = blockSize;
    }

    public int read(byte[] b) throws IOException {
      return super.read(b, 0, Math.min(b.length, blockSize));
    }

    public int read(byte[] b, int off, int len) throws IOException {
      return super.read(b, off, Math.min(len, blockSize));
    }
  }
}
