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
/**
 * 
 */
package com.google.typography.font.sfntly.sample.sfntdump;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.FontFactory;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.data.ReadableFontData;
import com.google.typography.font.sfntly.table.Table;
import com.google.typography.font.sfntly.table.bitmap.EblcTable;
import com.google.typography.font.sfntly.table.core.CMap;
import com.google.typography.font.sfntly.table.core.CMapTable;
import com.google.typography.font.sfntly.table.core.CMapTable.CMapId;
import com.google.typography.font.sfntly.table.core.NameTable;
import com.google.typography.font.sfntly.table.core.NameTable.NameEntry;
import com.google.typography.font.sfntly.table.core.PostScriptTable;
import com.google.typography.font.sfntly.table.truetype.Glyph;
import com.google.typography.font.sfntly.table.truetype.GlyphTable;
import com.google.typography.font.sfntly.table.truetype.LocaTable;
import com.google.typography.font.sfntly.table.truetype.SimpleGlyph;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class SfntDump {
  private boolean countSpecialGlyphs;
  private boolean dumpTableHeadersInFont;
  private boolean dumpNameList;
  private boolean dumpCmapList;
  private boolean cmapMapping;
  private boolean dumpPost;
  private boolean dumpEblc;
  @SuppressWarnings("unused")
  private boolean dumpAllGlyphs;
  private List<String> tablesToBinaryDump = new ArrayList<String>();
  private BitSet glyphSet;
  private boolean dumpAllChars;
  private BitSet charSet;
  private CMapId cmapId = CMapId.WINDOWS_BMP;
  private FontFactory fontFactory;

  /**
   * Dump a font with various options based on the command line.
   *
   * @param args command line arguments
   * @throws IOException 
   */
  public static void main(String[] args) throws IOException {
    SfntDump dumper = new SfntDump();
    File fontFile = null;
    int optionCount = 0;

    if (args.length > 0
        && !(args[0].equals("-h") || args[0].equals("-help") || args[0].equals("-?"))) {
      fontFile = new File(args[args.length - 1]);
      args = Arrays.copyOfRange(args, 0, args.length - 1);
    } else {
      printUsage();
      System.exit(0);
    }
    
    for (int i = 0; i < args.length; i++) {
      String option = null;
      if (args[i].charAt(0) == '-') {
        option = args[i].substring(1);
      }

      if (option != null) {
        optionCount++;
        
        if (option.equals("count")) {
          dumper.countSpecialGlyphs(true);
          continue;
        }

        if (option.equals("t")) {
          if (i + 1 < args.length) {
            dumper.dumpTablesAsBinary(args[++i]);
          }
          continue;
        }

        if (option.equals("cm")) {
          if (i + 1 < args.length) {
            dumper.useCMap(args[++i]);
          }
          continue;
        }

        if (option.equals("table")) {
          dumper.dumpTableList(true);
          continue;
        }

        if (option.startsWith("name")) {
          dumper.dumpNames(true);
          continue;
        }

        if (option.startsWith("cmap")) {
          dumper.dumpCMaps(true);
          if (i + 1 < args.length && !args[i + 1].startsWith("-")) {
            dumper.dumpCMaps(args[++i]);
          }          
          continue;
        }

        if (option.startsWith("post")) {
          dumper.dumpPost(true);
          continue;
        }

        if (option.startsWith("eblc")) {
          dumper.dumpEblc(true);
          continue;
        }
        
        if (option.equals("glyph") || option.equals("g")) {
          BitSet glyphSet = null;
          // if there's only one argument left (the filename), then i + 1 == args.length - 1
          if (i + 1 >= args.length || args[i + 1].startsWith("-")) {
            dumper.dumpAllGlyphs(true);
            continue;
          }
          if (i + 1 < args.length ) {
            i++;
            glyphSet = parseRange(args[i]);
            if (glyphSet == null) {
              glyphSet = parseList(args[i]);
            }
            if (glyphSet != null) {
              dumper.dumpGlyphs(glyphSet);
            }
          }
          if (glyphSet == null) {
            System.out.println("glyph dump option requires a glyph range or list");
            System.exit(0);
          }
        }

        if (option.equals("char") || option.equals("c")) {
          BitSet charSet = null;
          if (i + 1 >= args.length || args[i + 1].startsWith("-")) {
            dumper.dumpAllChars(true);
            continue;
          }
          if (i + 1 < args.length) {
            i++;
            charSet = parseRange(args[i]);
            if (charSet == null) {
              charSet = parseList(args[i]);
            }
            if (charSet != null) {
              dumper.dumpChars(charSet);
            }
          }
          if (charSet == null) {
            System.out.println("character dump option requires a glyph range or list");
            System.exit(0);
          }
        }

        if (option.equals("all") || option.equals("a")) {
          dumper.dumpAll(true);
        }
      }
    }

    if (optionCount == 0) {
      dumper.dumpTableList(true);
    }

    if (fontFile != null) {
      if (fontFile.isDirectory()) {
        File[] files = fontFile.listFiles();
        for (File file : files) {
          if (file.isFile() && !file.isHidden()) {
            try {
            dumper.dumpFont(file);
            System.out.println();
            } catch (Throwable t) {
              System.out.printf("Error processing file: %s\n", file);
            }
          }
        }
      } else {
        try {
          dumper.dumpFont(fontFile);
        } catch (Throwable t) {
          System.out.printf("Error processing file: %s\n", fontFile);
        }
      }
    } else {
      printUsage();
      System.exit(0);      
    }
  }

  private static final void printUsage() {
    System.out.println("FontDumper [-all|-a] [-table] [-t tag] [-name] [-cmap] [-g|-glyph range|li"
        + "st] [-c|-char range|list] [-?|-h|-help] fontfile | directory");
    System.out.println("dump information about the font file or all fonts in a directory");
    System.out.println("\t-all,-a\t\tdump all information");
    System.out.println("\t-table\t\tdump all table indexes");
    System.out.println(
        "\t-t tag\t\tbinary dump the table with the tag specified if it exists in the font");
    System.out.println("\t-name\t\tdump all name entries");
    System.out.println("\t-cmap [mapping]\t\tdump all cmap subtables");
    System.out.println(
        "\tif 'mapping' specified then dump the character to glyph mapping for the cmap(s)");
    // stupid string creation because the autoformatter and the 100 character
    // line length interactions
    String temp1 = "\t-cm pid,eid\t\tuse the cmap with the given platform id and";
    String temp2 = "encoding id when looking for glyphs from character ids";
    System.out.println(temp1 + temp2);
    System.out.println("\t-post\t\tdump the PostScript name table");
    System.out.println("\t-eblc\t\tdump the EBLC table - bitmap location");
    System.out.println("\t-g,-glyph\t\tdump the glyphs specified");
    System.out.println("\t-c,-char\t\tdump the characters specified using the Windows En" + 
    "glish Unicode cmap or the cmap specified with the -cm option");
    System.out.println("\trange\t\ttwo 1 to 4 digit numbers seperated by a hyphen that are opti" +
    "onally preceded by an x indicating hex - e.g. x12-234");
    System.out.println("\tlist\t\tone or more 1 to 4 digit numbers seperated commas that are op" +
    "tionally preceded by an x indicating hex - e.g. x12,234,666,x1234");
    System.out.println("\t-?,-h,-help\tprint this help information");
  }

  private static final Pattern RANGE_PATTERN =
    Pattern.compile("(x?)([\\da-fA-F]{1,5})-(x?)([\\da-fA-F]{1,5})");
  private static final BitSet parseRange(String range) {
    BitSet set = null;

    Matcher m = RANGE_PATTERN.matcher(range);
    if (m.matches()) {
      int low = Integer.parseInt(m.group(2), m.group(1).equals("") ? 10 : 16);
      int high = Integer.parseInt(m.group(4), m.group(3).equals("") ? 10 : 16);
      set = new BitSet();
      set.set(low, high + 1);
    }
    return set;
  }

  private static final Pattern NUMBER_PATTERN = Pattern.compile("(x?)([\\da-fA-F]{1,5})");
  private static final BitSet parseList(String list) {
    BitSet set = null;
    String[] items = list.split(",");

    if (items.length == 0) {
      return null;
    }

    set = new BitSet();
    for (String item : items) {
      Matcher m = NUMBER_PATTERN.matcher(item);
      if (!m.matches()) {
        return null;
      }
      int itemNumber = Integer.parseInt(m.group(2), m.group(1).equals("") ? 10 : 16);
      set.set(itemNumber);
    }
    return set;
  }

  public SfntDump() {
    this.fontFactory = FontFactory.getInstance();
  }

  public void countSpecialGlyphs(boolean count) {
    this.countSpecialGlyphs = count;
  }

  public void dumpTableList(boolean dumpTableList) {
    this.dumpTableHeadersInFont = dumpTableList;
  }

  public void dumpCMaps(boolean dumpCMaps) {
    this.dumpCmapList = dumpCMaps;
  }

  public void dumpCMaps(String option) {
    if (option.equals("mapping")) {
      this.cmapMapping = true;
    }
  }

  public void dumpNames(boolean dumpNames) {
    this.dumpNameList = dumpNames;
  }

  public void dumpPost(boolean dumpPost) {
    this.dumpPost = dumpPost;
  }

  public void dumpEblc(boolean dumpEblc) {
    this.dumpEblc = dumpEblc;
  }
  
  public void dumpAll(boolean dumpAll) {
    this.dumpCMaps(dumpAll);
    this.dumpNames(dumpAll);
    this.dumpPost(dumpAll);
    this.dumpTableList(dumpAll);
  }

  public void dumpGlyphs(BitSet set) {
    this.glyphSet = set;
  }

  public void dumpAllGlyphs(boolean dumpAllGlyphs) {
    this.dumpAllGlyphs = dumpAllGlyphs;
  }
  
  public void dumpAllChars(boolean dumpAll) {
    this.dumpAllChars = dumpAll;
  }

  public void dumpChars(BitSet set) {
    this.charSet = set;
  }

  public void dumpTablesAsBinary(String tableTag) {
    this.tablesToBinaryDump.add(tableTag);
  }

  public void useCMap(String cmap) {
    String[] cmapParams = cmap.split("\\D");
    this.cmapId =
      CMapId.getInstance(Integer.parseInt(cmapParams[0]), Integer.parseInt(cmapParams[1]));
  }

  public void dumpFont(File fontFile) throws IOException {
    boolean canDumpGlyphs = true;

    FileInputStream fis = null;
    System.out.println(fontFile + " ============================");
    try {
      fis = new FileInputStream(fontFile);
      Font[] fontArray = fontFactory.loadFonts(fis);

      for (int fontNumber = 0; fontNumber < fontArray.length; fontNumber++) {
        Font font = fontArray[fontNumber];
        if (fontArray.length > 1) {
          System.out.println("\n======= TTC Entry #" + fontNumber + "\n");
        }
        if (this.dumpTableHeadersInFont) {
          for (Map.Entry<Integer, ? extends Table> entry : font.tableMap().entrySet()) {
            System.out.println(entry.getValue().header());
          }
        }

        if (this.countSpecialGlyphs) {
          this.countSpecialGlyphs(font);
        }

        if (this.dumpNameList) {
          System.out.println("\n----- Name Tables");
          NameTable name = (NameTable) font.getTable(Tag.name);
          for (NameEntry entry : name) {
            System.out.println(entry);
          }
        }

        if (this.dumpCmapList) {
          System.out.println("\n------ CMap Tables");
          CMapTable cmapTable = (CMapTable) font.getTable(Tag.cmap);
          for (CMap cmap : cmapTable) {
            System.out.println(cmap);
            if (this.cmapMapping) {
              dumpCMapMapping(cmap);
            }
          }
        }

        if (this.dumpPost) {
          System.out.println("\n------ Post Table");
          PostScriptTable post = font.getTable(Tag.post);
          int nGlyphs = post.numberOfGlyphs();
          for (int glyphId = 0; glyphId < nGlyphs; glyphId++) {
            System.out.printf("%d: %s\n", glyphId, post.glyphName(glyphId));
          }
        }

        if (this.dumpEblc) {
          System.out.println("\n------ EBLC Table");
          EblcTable eblcTable = font.getTable(Tag.EBLC);
          System.out.println(eblcTable.toString());
        }

        if (this.tablesToBinaryDump.size() > 0) {
          for (String tag : this.tablesToBinaryDump) {
            int tableTag = Tag.intValue(tag);
            Table table = font.getTable(tableTag);
            if (table != null) {
              System.out.println("\n------ Dump Data - Table = " + tag + ", length = "
                  + Integer.toHexString(table.dataLength()));
              ReadableFontData data = table.readFontData();
              for (int i = 0; i < data.length(); i += 16) {
                System.out.printf("%08x: ", i);
                for (int j = i; (j < i + 16) && j < data.length(); j++) {
                  System.out.printf("%02x ", data.readUByte(j));
                }
                System.out.println();
              }
              System.out.println();
            }
          }
        }

        LocaTable locaTable = font.getTable(Tag.loca);
        GlyphTable glyphTable = font.getTable(Tag.glyf);
        if (locaTable == null) {
          canDumpGlyphs = false;
          System.out.println("PROBLEM: font has no 'loca' table.");
        }
        if (glyphTable == null) {
          canDumpGlyphs = false;
          System.out.println("PROBLEM: font has no 'glyf' table.");
        }

        if (canDumpGlyphs && this.glyphSet != null) {
          System.out.println("\n------ Glyphs");
          for (int glyphId = this.glyphSet.nextSetBit(0); 
          glyphId >= 0; glyphId = this.glyphSet.nextSetBit(glyphId+1)) {
            int offset = locaTable.glyphOffset(glyphId);
            int length = locaTable.glyphLength(glyphId);
            Glyph glyph = glyphTable.glyph(offset, length);
            System.out.println("glyph id = " + glyphId);
            if (glyph != null) {
              System.out.println(glyph);
            }
          }
        }

        if (canDumpGlyphs && this.charSet != null) {
          dumpChars(font, locaTable, glyphTable);
        }
      }
    } finally {
      if (fis != null) {
        fis.close();
      }
    }
  }

  private void dumpChars(Font font, LocaTable locaTable, GlyphTable glyphTable) {
    CMapTable cmapTable = font.getTable(Tag.cmap);
    if (cmapTable == null) {
      System.out.println("PROBLEM: font has no 'cmap' table.");
      return;
    }
    CMap cmap = cmapTable.cmap(this.cmapId);
    // if (cmap == null) {
    // cmap = cmapTable.cmap(
    // Font.PlatformId.Windows.value(),
    // Font.WindowsEncodingId.UnicodeUCS4.value());
    // }
    if (cmap == null) {
      System.out.println("PROBLEM: required cmap subtable not available.");
      return;
    }

    System.out.println("\n=============\n" + cmap);

    if (this.dumpAllChars) {
      for (int charId : cmap) {
        dumpChar(charId, cmap, locaTable, glyphTable);
      }
    } else if (this.charSet != null) {
      System.out.println("\n------ Characters");
      for (int charId = this.charSet.nextSetBit(0); 
      charId >= 0; charId = this.charSet.nextSetBit(charId+1)) {
        dumpChar(charId, cmap, locaTable, glyphTable);
      }
    }
  }

  private void dumpChar(int charId, CMap cmap, LocaTable locaTable, GlyphTable glyphTable) {
    int glyphId = cmap.glyphId(charId);
    int offset = locaTable.glyphOffset(glyphId);
    int length = locaTable.glyphLength(glyphId);
    Glyph glyph = glyphTable.glyph(offset, length);
    System.out.println("char = 0x" + Integer.toHexString(charId) + ", glyph id = 0x"
        + Integer.toHexString(glyphId));
    if (glyph != null) {
      System.out.println(glyph);
    } else {
      System.out.println();
    }
  }

  private void countSpecialGlyphs(Font font) {
    LocaTable locaTable = font.getTable(Tag.loca);
    GlyphTable glyphTable = font.getTable(Tag.glyf);

    int count = 0;
    for (int glyphId = 0; glyphId < locaTable.numGlyphs(); glyphId++) {
      int offset = locaTable.glyphOffset(glyphId);
      int length = locaTable.glyphLength(glyphId);
      Glyph glyph = glyphTable.glyph(offset, length);
      if (glyph instanceof SimpleGlyph) {
        SimpleGlyph simple = (SimpleGlyph) glyph;
        if (simple.numberOfContours() != 2) {
          continue;
        }
        if ((simple.numberOfPoints(0) != 1) && (simple.numberOfPoints(1) != 1)) {
          continue;
        }
        count++;
      }
    }
    System.out.println("\n------ Special Glyph Count");
    System.out.println("\ttotal glyphs = " + locaTable.numGlyphs());
    System.out.println("\tspecial glyphs = " + count);    
  }
  
  private void dumpCMapMapping(CMap cmap) {
    Iterator<Integer> iter = cmap.iterator();
    while (iter.hasNext()) {
      int c = iter.next();
      int g = cmap.glyphId(c);
      if (g != CMapTable.NOTDEF) {
        System.out.printf("%x -> %x\n", c, g);
      }
    }
  }
}
