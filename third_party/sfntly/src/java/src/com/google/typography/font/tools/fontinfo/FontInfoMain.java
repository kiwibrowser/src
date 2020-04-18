// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.typography.font.tools.fontinfo;

import com.google.typography.font.sfntly.Font;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.ParameterException;

import java.io.IOException;

/**
 * This is the main class for the command-line version of the font info tool
 *
 * @author Han-Wen Yeh
 *
 */
public class FontInfoMain {
  private static final String PROGRAM_NAME = "java -jar fontinfo.jar";

  public static void main(String[] args) {
    CommandOptions options = new CommandOptions();
    JCommander commander = null;
    try {
      commander = new JCommander(options, args);
    } catch (ParameterException e) {
      System.out.println(e.getMessage());
      commander = new JCommander(options, "--help");
    }

    // Display help
    if (options.help) {
      commander.setProgramName(PROGRAM_NAME);
      commander.usage();
      return;
    }

    // No font loaded
    if (options.files.size() != 1) {
      System.out.println(
          "Please specify a single font. Try '" + PROGRAM_NAME + " --help' for more information.");
      return;
    }

    // Default option
    if (!(options.metrics || options.general || options.cmap || options.chars || options.blocks
        || options.scripts || options.glyphs || options.all)) {
      options.general = true;
    }

    // Obtain file name
    String fileName = options.files.get(0);

    // Load font
    Font[] fonts = null;
    try {
      fonts = FontUtils.getFonts(fileName);
    } catch (IOException e) {
      System.out.println("Unable to load font " + fileName);
      return;
    }

    for (int i = 0; i < fonts.length; i++) {
      Font font = fonts[i];

      if (fonts.length > 1 && !options.csv) {
        System.out.println("==== Information for font index " + i + " ====\n");
      }

      // Print general information
      if (options.general || options.all) {
        if (options.csv) {
          System.out.println(String.format("sfnt version: %s", FontInfo.sfntVersion(font)));
          System.out.println();
          System.out.println("Font Tables");
          System.out.println(
              prependDataAndBuildCsv(FontInfo.listTables(font).csvStringArray(), fileName, i));
          System.out.println();
          System.out.println("Name Table Entries:");
          System.out.println(
              prependDataAndBuildCsv(FontInfo.listNameEntries(font).csvStringArray(), fileName, i));
          System.out.println();
        } else {
          System.out.println(String.format("sfnt version: %s", FontInfo.sfntVersion(font)));
          System.out.println();
          System.out.println("Font Tables:");
          FontInfo.listTables(font).prettyPrint();
          System.out.println();
          System.out.println("Name Table Entries:");
          FontInfo.listNameEntries(font).prettyPrint();
          System.out.println();
        }
      }

      // Print metrics
      if (options.metrics || options.all) {
        if (options.csv) {
          System.out.println("Font Metrics:");
          System.out.println(
              prependDataAndBuildCsv(FontInfo.listFontMetrics(font).csvStringArray(), fileName, i));
          System.out.println();
        } else {
          System.out.println("Font Metrics:");
          FontInfo.listFontMetrics(font).prettyPrint();
          System.out.println();
        }
      }

      // Print glyph metrics
      if (options.metrics || options.glyphs || options.all) {
        if (options.csv) {
          System.out.println("Glyph Metrics:");
          System.out.println(prependDataAndBuildCsv(
              FontInfo.listGlyphDimensionBounds(font).csvStringArray(), fileName, i));
          System.out.println();
        } else {
          System.out.println("Glyph Metrics:");
          FontInfo.listGlyphDimensionBounds(font).prettyPrint();
          System.out.println();
        }
      }

      // Print cmap list
      if (options.cmap || options.all) {
        if (options.csv) {
          System.out.println("Cmaps in the font:");
          System.out.println(
              prependDataAndBuildCsv(FontInfo.listCmaps(font).csvStringArray(), fileName, i));
          System.out.println();
        } else {
          System.out.println("Cmaps in the font:");
          FontInfo.listCmaps(font).prettyPrint();
          System.out.println();
        }
      }

      // Print blocks
      if (options.blocks || options.all) {
        if (options.csv) {
          System.out.println("Unicode block coverage:");
          System.out.println(prependDataAndBuildCsv(
              FontInfo.listCharBlockCoverage(font).csvStringArray(), fileName, i));
          System.out.println();
        } else {
          System.out.println("Unicode block coverage:");
          FontInfo.listCharBlockCoverage(font).prettyPrint();
          System.out.println();
        }
      }

      // Print scripts
      if (options.scripts || options.all) {
        if (options.csv) {
          System.out.println("Unicode script coverage:");
          System.out.println(prependDataAndBuildCsv(
              FontInfo.listScriptCoverage(font).csvStringArray(), fileName, i));
          System.out.println();
          if (options.detailed) {
            System.out.println("Uncovered code points in partially-covered scripts:");
            System.out.println(prependDataAndBuildCsv(
                FontInfo.listCharsNeededToCoverScript(font).csvStringArray(), fileName, i));
            System.out.println();
          }
        } else {
          System.out.println("Unicode script coverage:");
          FontInfo.listScriptCoverage(font).prettyPrint();
          System.out.println();
          if (options.detailed) {
            System.out.println("Uncovered code points in partially-covered scripts:");
            FontInfo.listCharsNeededToCoverScript(font).prettyPrint();
            System.out.println();
          }
        }
      }

      // Print char list
      if (options.chars || options.all) {
        if (options.csv) {
          System.out.println("Characters with valid glyphs:");
          System.out.println(
              prependDataAndBuildCsv(FontInfo.listChars(font).csvStringArray(), fileName, i));
          System.out.println();
        } else {
          System.out.println("Characters with valid glyphs:");
          FontInfo.listChars(font).prettyPrint();
          System.out.println();
          System.out.println(String.format(
              "Total number of characters with valid glyphs: %d", FontInfo.numChars(font)));
          System.out.println();
        }
      }

      // Print glyph information
      if (options.glyphs || options.all) {
        DataDisplayTable unmappedGlyphs = FontInfo.listUnmappedGlyphs(font);
        if (options.csv) {
          System.out.println(String.format("Total hinting size: %s", FontInfo.hintingSize(font)));
          System.out.println(String.format(
              "Number of unmapped glyphs: %d / %d", unmappedGlyphs.getNumRows(),
              FontInfo.numGlyphs(font)));
          System.out.println();
          if (options.detailed) {
            System.out.println("Unmapped glyphs:");
            System.out.println(
                prependDataAndBuildCsv(unmappedGlyphs.csvStringArray(), fileName, i));
            System.out.println();
          }
          System.out.println("Subglyphs used by characters in the font:");
          System.out.println(prependDataAndBuildCsv(
              FontInfo.listSubglyphFrequency(font).csvStringArray(), fileName, i));
          System.out.println();
        } else {
          System.out.println(String.format("Total hinting size: %s", FontInfo.hintingSize(font)));
          System.out.println(String.format(
              "Number of unmapped glyphs: %d / %d", unmappedGlyphs.getNumRows(),
              FontInfo.numGlyphs(font)));
          System.out.println();
          if (options.detailed) {
            System.out.println("Unmapped glyphs:");
            unmappedGlyphs.prettyPrint();
            System.out.println();
          }
          System.out.println("Subglyphs used by characters in the font:");
          FontInfo.listSubglyphFrequency(font).prettyPrint();
          System.out.println();
        }
      }
    }
  }

  private static String prependDataAndBuildCsv(String[] arr, String fontName, int fontIndex) {
    StringBuilder output = new StringBuilder("Font,font index,").append(arr[0]).append('\n');
    for (int i = 1; i < arr.length; i++) {
      String row = arr[i];
      output.append(fontName)
          .append(',')
          .append("font index ")
          .append(fontIndex)
          .append(',')
          .append(row)
          .append('\n');
    }
    return output.toString();
  }
}
