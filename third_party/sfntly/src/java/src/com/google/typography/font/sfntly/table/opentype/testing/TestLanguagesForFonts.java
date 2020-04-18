package com.google.typography.font.sfntly.table.opentype.testing;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

public class TestLanguagesForFonts {
  private static final String FONTS_ROOT = "/usr/local/google/home/cibu/sfntly/fonts";
  private static final String WORDS_DIR = "/usr/local/google/home/cibu/sfntly/adv_layout/data/testdata/wiki_words";
  private static final String OUTPUT_FILE = "/tmp/font-languages.txt";

  private static final FontLanguages fontLanguages = new FontLanguages(availableLangs(WORDS_DIR));

  public static void main(String[] args) throws IOException {
    List<File> fontFiles = FontLoader.getFontFiles(FONTS_ROOT);
    PrintWriter writer = new PrintWriter(OUTPUT_FILE);
    for (File fontFile : fontFiles) {
      writer.print(fontFile.getPath());
      Set<String> langs = fontLanguages.get(FontLoader.getFont(fontFile));
      if (langs.isEmpty()) {
        langs.add("en");
      }
      for (String lang : langs) {
        writer.print("," + lang);
      }
      writer.println();
    }
    writer.close();
  }

  private static List<String> availableLangs(String wordsDir) {
    List<String> langs = new ArrayList<String>();
    File[] wordFiles = new File(wordsDir).listFiles();
    for (File file : wordFiles) {
      String lang = file.getName();
      if (lang.startsWith(".")) {
        continue;
      }
      langs.add(lang);
    }
    return langs;
  }
}
