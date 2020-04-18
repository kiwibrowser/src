package com.google.typography.font.sfntly.sample.sfview;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.FontFactory;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import javax.swing.JFrame;
import javax.swing.JScrollPane;

public class SFView {
  public static void main(String[] args) throws IOException {
    for (String fontName : args) {
      System.out.println("Displaying font: " + fontName);
      Font[] fonts = loadFont(new File(fontName));
      if (fonts == null) {
        continue;
      }
      for (Font font : fonts) {
        JFrame jf = new JFrame("Sfntly Table Viewer");
        jf.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        SFFontView view = new SFFontView(font);
        JScrollPane sp = new JScrollPane(view);
        jf.add(sp);
        jf.pack();
        jf.setVisible(true);
      }
    }
  }

  private static Font[] loadFont(File file) throws IOException {
    FontFactory fontFactory = FontFactory.getInstance();
    fontFactory.fingerprintFont(true);
    FileInputStream is = null;
    try {
      is = new FileInputStream(file);
      return fontFactory.loadFonts(is);
    } catch (FileNotFoundException e) {
      System.err.println("Could not load the font: " + file.getName());
      return null;
    } finally {
      if (is != null) {
        is.close();
      }
    }
  }
}
