package com.google.typography.font.sfntly.sample.sfview;

import com.google.typography.font.sfntly.Font;
import com.google.typography.font.sfntly.FontFactory;
import com.google.typography.font.sfntly.Tag;
import com.google.typography.font.sfntly.table.opentype.GSubTable;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.UnsupportedEncodingException;

public class HtmlViewer {
//  private static final String fileName = "/home/build/google3/googledata/third_party/" +
//      "fonts/ascender/arial.ttf";

  public static void main(String[] args) throws IOException {

    Font[] fonts = loadFont(new File(args[0]));
    GSubTable gsub = fonts[0].getTable(Tag.GSUB);
    tag(gsub, args[1]);

  }
  public static void tag(GSubTable gsub, String outFileName) throws FileNotFoundException, UnsupportedEncodingException {
    PrintWriter writer = new PrintWriter(outFileName, "UTF-8");
    writer.println("<html>");
    writer.println("  <head>");
    writer.println("    <link href=special.css rel=stylesheet type=text/css>");
    writer.println("  </head>");
    writer.println("  <body>");
//    writer.println(gsub.scriptList().toHtml());
//    writer.println(gsub.featureList().toHtml());
//    writer.println(gsub.lookupList().toHtml());
    writer.println("  </body>");
    writer.println("</html>");
    writer.close();
  }

  public static Font[] loadFont(File file) throws IOException {
    FontFactory fontFactory = FontFactory.getInstance();
    fontFactory.fingerprintFont(true);
    FileInputStream is = null;
    try {
      is = new FileInputStream(file);
      return fontFactory.loadFonts(is);
    } finally {
      if (is != null) {
        is.close();
      }
    }
  }
}
