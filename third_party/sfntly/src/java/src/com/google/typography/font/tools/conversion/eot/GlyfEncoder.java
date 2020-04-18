/*
 * Copyright 2011 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.typography.font.tools.conversion.eot;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.truetype.CompositeGlyph;
import com.google.typography.font.sfntly.table.truetype.Glyph;
import com.google.typography.font.sfntly.table.truetype.GlyphTable;
import com.google.typography.font.sfntly.table.truetype.LocaTable;
import com.google.typography.font.sfntly.table.truetype.SimpleGlyph;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * @author Raph Levien
 *
 * Implementation of compression of CTF glyph data, as per sections 5.6-5.10 and 6 of the spec.
 */
public class GlyfEncoder {

  private final boolean doPush;
  
  private final ByteArrayOutputStream glyfStream;
  private final ByteArrayOutputStream pushStream;
  private final ByteArrayOutputStream codeStream;

  public GlyfEncoder(boolean doPush) {
    this.doPush = doPush;
    glyfStream = new ByteArrayOutputStream();
    pushStream = new ByteArrayOutputStream();
    codeStream = new ByteArrayOutputStream();
  }

  public GlyfEncoder() {
    this(true);
  }

  public void encode(Font sourceFont) {
    LocaTable loca = sourceFont.getTable(Tag.loca);
    int nGlyphs = loca.numGlyphs();
    GlyphTable glyf = sourceFont.getTable(Tag.glyf);

    for (int glyphId = 0; glyphId < nGlyphs; glyphId++) {
      int sourceOffset = loca.glyphOffset(glyphId);
      int length = loca.glyphLength(glyphId);
      Glyph glyph = glyf.glyph(sourceOffset, length);
      writeGlyph(glyph);
    }
  }

  private void writeGlyph(Glyph glyph) {
    try {
      if (glyph == null || glyph.dataLength() == 0) {
        writeUShort(0);
      } else if (glyph instanceof SimpleGlyph) {
        writeSimpleGlyph((SimpleGlyph)glyph);
      } else if (glyph instanceof CompositeGlyph) {
        writeCompositeGlyph((CompositeGlyph)glyph);
      }
    } catch (IOException e) {
      throw new RuntimeException("unexpected IOException writing glyph data", e);
    }
  }
  
  private void writeInstructions(Glyph glyph) throws IOException{
    if (doPush) {
      splitPush(glyph);
    } else {
      int pushCount = 0;
      int codeSize = glyph.instructionSize();
      write255UShort(glyfStream, pushCount);
      write255UShort(glyfStream, codeSize);
      if (codeSize > 0) {
        glyph.instructions().copyTo(codeStream);
      }
    }
  }

  private void writeSimpleGlyph(SimpleGlyph glyph) throws IOException {
    int numContours = glyph.numberOfContours();
      writeUShort(numContours);
      for (int i = 0; i < numContours; i++) {
        write255UShort(glyfStream, glyph.numberOfPoints(i) - (i == 0 ? 1 : 0));
      }
      ByteArrayOutputStream os = new ByteArrayOutputStream();
      int lastX = 0;
      int lastY = 0;
      for (int i = 0; i < numContours; i++) {
        int numPoints = glyph.numberOfPoints(i);
        for (int j = 0; j < numPoints; j++) {
          int x = glyph.xCoordinate(i, j);
          int y = glyph.yCoordinate(i, j);
          int dx = x - lastX;
          int dy = y - lastY;
          writeTriplet(os, glyph.onCurve(i, j), dx, dy);
          lastX = x;
          lastY = y;
        }
      }
      os.writeTo(glyfStream);
      if (numContours > 0) {
        writeInstructions(glyph);
      }
  }
  
  private void writeCompositeGlyph(CompositeGlyph glyph) throws IOException {
    boolean haveInstructions = false;
    writeUShort(-1);
    writeUShort(glyph.xMin());
    writeUShort(glyph.yMin());
    writeUShort(glyph.xMax());
    writeUShort(glyph.yMax());
    for (int i = 0; i < glyph.numGlyphs(); i++) {
      int flags = glyph.flags(i);
      writeUShort(flags);
      haveInstructions = (flags & CompositeGlyph.FLAG_WE_HAVE_INSTRUCTIONS) != 0;
      writeUShort(glyph.glyphIndex(i));
      if ((flags & CompositeGlyph.FLAG_ARG_1_AND_2_ARE_WORDS) == 0) {
        glyfStream.write(glyph.argument1(i));
        glyfStream.write(glyph.argument2(i));
      } else {
        writeUShort(glyph.argument1(i));
        writeUShort(glyph.argument2(i));
      }
      if (glyph.transformationSize(i) != 0) {
        try {
          glyfStream.write(glyph.transformation(i));
        } catch (IOException e) {
        }
      }
    }
    if (haveInstructions) {
      writeInstructions(glyph);
    }
  }

  private void writeUShort(int value) {
    glyfStream.write(value >> 8);
    glyfStream.write(value & 255);
  }
  
  // As per 6.1.1 of spec
  // visible for testing
  static void write255UShort(OutputStream os, int value) throws IOException {
    if (value < 0) {
      throw new IllegalArgumentException();
    }
    if (value < 253) {
      os.write((byte)value);
    } else if (value < 506) {
      os.write(255);
      os.write((byte)(value - 253));
    } else if (value < 762) {
      os.write(254);
      os.write((byte)(value - 506));
    } else {
      os.write(253);
      os.write((byte)(value >> 8));
      os.write((byte)(value & 0xff));
    }
  }
  
  // As per 6.1.1 of spec
  // visible for testing
  static void write255Short(OutputStream os, int value) throws IOException {
    int absValue = Math.abs(value);
    if (value < 0) {
      // spec is unclear about whether words should be signed. This code is conservative, but we
      // can test once the implementation is working.
      os.write(250);
    }
    if (absValue < 250) {
      os.write((byte)absValue);
    } else if (absValue < 500) {
      os.write(255);
      os.write((byte)(absValue - 250));
    } else if (absValue < 756) {
      os.write(254);
      os.write((byte)(absValue - 500));
    } else {
      os.write(253);
      os.write((byte)(absValue >> 8));
      os.write((byte)(absValue & 0xff));
    }
  }
  
  // As in section 5.11 of the spec
  // visible for testing
  void writeTriplet(OutputStream os, boolean onCurve, int x, int y) throws IOException {
    int absX = Math.abs(x);
    int absY = Math.abs(y);
    int onCurveBit = onCurve ? 0 : 128;
    int xSignBit = (x < 0) ? 0 : 1;
    int ySignBit = (y < 0) ? 0 : 1;
    int xySignBits = xSignBit + 2 * ySignBit;
    
    if (x == 0 && absY < 1280) {
      glyfStream.write(onCurveBit + ((absY & 0xf00) >> 7) + ySignBit);
      os.write(absY & 0xff);
    } else if (y == 0 && absX < 1280) {
      glyfStream.write(onCurveBit + 10 + ((absX & 0xf00) >> 7) + xSignBit);
      os.write(absX & 0xff);
    } else if (absX < 65 && absY < 65) {
      glyfStream.write(onCurveBit + 20 + ((absX - 1) & 0x30) + (((absY - 1) & 0x30) >> 2) +
          xySignBits);
      os.write((((absX - 1) & 0xf) << 4) | ((absY - 1) & 0xf));
    } else if (absX < 769 && absY < 769) {
      glyfStream.write(onCurveBit + 84 + 12 * (((absX - 1) & 0x300) >> 8) +
          (((absY - 1) & 0x300) >> 6) + xySignBits);
      os.write((absX - 1) & 0xff);
      os.write((absY - 1) & 0xff);
    } else if (absX < 4096 && absY < 4096) {
      glyfStream.write(onCurveBit + 120 + xySignBits);
      os.write(absX >> 4);
      os.write(((absX & 0xf) << 4) | (absY >> 8));
      os.write(absY & 0xff);
    } else {
      glyfStream.write(onCurveBit + 124 + xySignBits);
      os.write(absX >> 8);
      os.write(absX & 0xff);
      os.write(absY >> 8);
      os.write(absY & 0xff);
    }
  }

  /**
   * Split the instructions into a push sequence and the remainder of the instructions.
   * Writes both streams, and the counts to the glyfStream.
   * 
   * As per section 6.2.1 of the spec.
   * 
   * @param glyph the glyph to split
   */
  private void splitPush(Glyph glyph) throws IOException {
    int instrSize = glyph.instructionSize();
    ReadableFontData data = glyph.instructions();
    int i = 0;
    List<Integer> result = new ArrayList<Integer>();
    // All push sequences are at least two bytes, make sure there's enough room
    while (i + 1 < instrSize) {
      int ix = i;
      int instr = data.readUByte(ix++);
      int n = 0;
      int size = 0;
      if (instr == 0x40 || instr == 0x41) {
        // NPUSHB, NPUSHW
        n = data.readUByte(ix++);
        size = (instr & 1) + 1;
      } else if (instr >= 0xB0 && instr < 0xC0) {
        // PUSHB, PUSHW
        n = 1 + (instr & 7);
        size = ((instr & 8) >> 3) + 1;
      } else {
        break;
      }
      if (i + size * n > instrSize) {
        // This is a broken font, and a potential buffer overflow, but in the interest
        // of preserving the original, we just put the broken instruction sequence in
        // the stream.
        break;
      }
      for (int j = 0; j < n; j++) {
        if (size == 1) {
          result.add(data.readUByte(ix));
        } else {
          result.add(data.readShort(ix));
        }
        ix += size;
      }
      i = ix;
    }
    int pushCount = result.size();
    int codeSize = instrSize - i;
    write255UShort(glyfStream, pushCount);
    write255UShort(glyfStream, codeSize);
    encodePushSequence(pushStream, result);
    if (codeSize > 0) {
      data.slice(i).copyTo(codeStream);
    }
  }
  
  // As per section 6.2.2 of the spec.
  private void encodePushSequence(OutputStream os, List<Integer> data) throws IOException {
    int n = data.size();
    int hopSkip = 0;
    for (int i = 0; i < n; i++) {
      if ((hopSkip & 1) == 0) {
        int val = data.get(i);
        if (hopSkip == 0 && i >= 2 &&
            i + 2 < n && val == data.get(i - 2) && val == data.get(i + 2)) {
          if (i + 4 < n && val == data.get(i + 4)) {
            // Hop4 code
            os.write(252);
            hopSkip = 0x14;
          } else {
            // Hop3 code
            os.write(251);
            hopSkip = 4;
          }
        } else {
          write255Short(os, data.get(i));
        }
      }
      hopSkip >>= 1;
    }
  }

  public byte[] getGlyfBytes() {
    return glyfStream.toByteArray();
  }

  public byte[] getPushBytes() {
    return pushStream.toByteArray();
  }

  public byte[] getCodeBytes() {
    return codeStream.toByteArray();
  }
}
