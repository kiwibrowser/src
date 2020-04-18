/*
 * Copyright 2010 Google Inc. All Rights Reserved.
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

package com.google.typography.font.sfntly;

import java.io.UnsupportedEncodingException;


/**
 * Font identification tags used for tables, features, etc.
 *
 * Tag names are consistent with the OpenType and sfnt specs.
 *
 * @author Stuart Gill
 */
public final class Tag {
  public static final int ttcf = Tag.intValue(new byte[]{'t', 't', 'c', 'f'});

  /***********************************************************************************
   *
   * Table Type Tags
   *
   ***********************************************************************************/

  // required tables
  public static final int cmap = Tag.intValue(new byte[]{'c', 'm', 'a', 'p'});
  public static final int head = Tag.intValue(new byte[]{'h', 'e', 'a', 'd'});
  public static final int hhea = Tag.intValue(new byte[]{'h', 'h', 'e', 'a'});
  public static final int hmtx = Tag.intValue(new byte[]{'h', 'm', 't', 'x'});
  public static final int maxp = Tag.intValue(new byte[]{'m', 'a', 'x', 'p'});
  public static final int name = Tag.intValue(new byte[]{'n', 'a', 'm', 'e'});
  public static final int OS_2 = Tag.intValue(new byte[]{'O', 'S', '/', '2'});
  public static final int post = Tag.intValue(new byte[]{'p', 'o', 's', 't'});

  // truetype outline tables
  public static final int cvt = Tag.intValue(new byte[]{'c', 'v', 't', ' '});
  public static final int fpgm = Tag.intValue(new byte[]{'f', 'p', 'g', 'm'});
  public static final int glyf = Tag.intValue(new byte[]{'g', 'l', 'y', 'f'});
  public static final int loca = Tag.intValue(new byte[]{'l', 'o', 'c', 'a'});
  public static final int prep = Tag.intValue(new byte[]{'p', 'r', 'e', 'p'});

  // postscript outline tables
  public static final int CFF = Tag.intValue(new byte[]{'C', 'F', 'F', ' '});
  public static final int VORG = Tag.intValue(new byte[]{'V', 'O', 'R', 'G'});

  // opentype bitmap glyph outlines
  public static final int EBDT = Tag.intValue(new byte[]{'E', 'B', 'D', 'T'});
  public static final int EBLC = Tag.intValue(new byte[]{'E', 'B', 'L', 'C'});
  public static final int EBSC = Tag.intValue(new byte[]{'E', 'B', 'S', 'C'});

  // advanced typographic features
  public static final int BASE = Tag.intValue(new byte[]{'B', 'A', 'S', 'E'});
  public static final int GDEF = Tag.intValue(new byte[]{'G', 'D', 'E', 'F'});
  public static final int GPOS = Tag.intValue(new byte[]{'G', 'P', 'O', 'S'});
  public static final int GSUB = Tag.intValue(new byte[]{'G', 'S', 'U', 'B'});
  public static final int JSTF = Tag.intValue(new byte[]{'J', 'S', 'T', 'F'});

  // other
  public static final int DSIG = Tag.intValue(new byte[]{'D', 'S', 'I', 'G'});
  public static final int gasp = Tag.intValue(new byte[]{'g', 'a', 's', 'p'});
  public static final int hdmx = Tag.intValue(new byte[]{'h', 'd', 'm', 'x'});
  public static final int kern = Tag.intValue(new byte[]{'k', 'e', 'r', 'n'});
  public static final int LTSH = Tag.intValue(new byte[]{'L', 'T', 'S', 'H'});
  public static final int PCLT = Tag.intValue(new byte[]{'P', 'C', 'L', 'T'});
  public static final int VDMX = Tag.intValue(new byte[]{'V', 'D', 'M', 'X'});
  public static final int vhea = Tag.intValue(new byte[]{'v', 'h', 'e', 'a'});
  public static final int vmtx = Tag.intValue(new byte[]{'v', 'm', 't', 'x'});

  // AAT Tables
  // TODO(stuartg): some tables may be missing from this list
  public static final int bsln = Tag.intValue(new byte[]{'b', 's', 'l', 'n'});
  public static final int feat = Tag.intValue(new byte[]{'f', 'e', 'a', 't'});
  public static final int lcar = Tag.intValue(new byte[]{'l', 'c', 'a', 'r'});
  public static final int morx = Tag.intValue(new byte[]{'m', 'o', 'r', 'x'});
  public static final int opbd = Tag.intValue(new byte[]{'o', 'p', 'b', 'd'});
  public static final int prop = Tag.intValue(new byte[]{'p', 'r', 'o', 'p'});

  // Graphite tables
  public static final int Feat = Tag.intValue(new byte[]{'F', 'e', 'a', 't'});
  public static final int Glat = Tag.intValue(new byte[]{'G', 'l', 'a', 't'});
  public static final int Gloc = Tag.intValue(new byte[]{'G', 'l', 'o', 'c'});
  public static final int Sile = Tag.intValue(new byte[]{'S', 'i', 'l', 'e'});
  public static final int Silf = Tag.intValue(new byte[]{'S', 'i', 'l', 'f'});

  // truetype bitmap font tables
  public static final int bhed = Tag.intValue(new byte[]{'b', 'h', 'e', 'd'});
  public static final int bdat = Tag.intValue(new byte[]{'b', 'd', 'a', 't'});
  public static final int bloc = Tag.intValue(new byte[]{'b', 'l', 'o', 'c'});

  private Tag() {
    // Prevent construction.
  }

  public static int intValue(byte[] tag) {
    return tag[0] << 24 | tag[1] << 16 | tag[2] << 8 | tag[3];
  }

  public static byte[] byteValue(int tag) {
    byte[] b = new byte[4];
    b[0] = (byte) (0xff & (tag >> 24));
    b[1] = (byte) (0xff & (tag >> 16));
    b[2] = (byte) (0xff & (tag >> 8));
    b[3] = (byte) (0xff & tag);
    return b;
  }

  public static String stringValue(int tag) {
    String s;
    try {
      s = new String(Tag.byteValue(tag), "US-ASCII");
    } catch (UnsupportedEncodingException e) {
      // should never happen since US-ASCII is a guaranteed character set but...
      return "";
    }
    return s;
  }

  public static int intValue(String s) {
    byte[] b = null;
    try {
      b = s.substring(0, 4).getBytes("US-ASCII");
    } catch (UnsupportedEncodingException e) {
      // should never happen since US-ASCII is a guaranteed character set but...
      return 0;
    }
    return intValue(b);
  }

  /**
   * Determines whether the tag is that for the header table.
   * @param tag table tag
   * @return true if the tag represents the font header table
   */
  public static boolean isHeaderTable(int tag) {
    if (tag == Tag.head || tag == Tag.bhed) {
      return true;
    }
    return false;
  }
}
